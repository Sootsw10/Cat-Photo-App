#include "ShelfCharacter.h"
#include "ShelfAIController.h"
#include "ShelfWorld.h"
#include "ShelfGameInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"

namespace
{
	bool Is(FName A, const TCHAR* B) { return A == FName(B); }
	const FColor ColRot(183, 211, 106), ColBond(242, 239, 230), ColHeal(168, 224, 138), ColGold(232, 194, 124), ColBlue(168, 212, 255), ColRegard(243, 220, 160);
}

AShelfCharacter::AShelfCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 720.f, 0.f);
	Move->MaxWalkSpeed = 430.f;
	Move->BrakingDecelerationWalking = 2400.f;
	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(GetCapsuleComponent());
	VisualRoot->SetRelativeLocation(FVector(0.f, 0.f, -88.f));
	AIControllerClass = AShelfAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::Spawned;
}

// ===================================================================== setup

void AShelfCharacter::InitMember(const FShelfMember& M)
{
	const FHouseDef& H = ShelfData::House(M.House);
	const FShelfStats S = ShelfData::Stats(M);
	MemberId = M.Id; DisplayName = M.Name; House = M.House; Team = EShelfTeam::Party;
	MaxHP = S.MaxHP; HP = FMath::Clamp(M.HP, 1, MaxHP); Atk = S.Atk; Def = S.Def; Spd = S.Spd;
	AttackRange = H.AttackRange;
	Abilities.Reset();
	for (const FName& A : H.Abilities)
	{
		const FAbilityDef* D = ShelfData::Ability(A);
		if (D && M.Level >= D->Unlock) Abilities.Add(A);
	}
	bCompanion = M.Id != FName(TEXT("you"));
	FLinearColor Cloak = H.Cloak, Tunic = H.Tunic;
	if (M.Id == FName(TEXT("nim"))) { Cloak = FLinearColor(FColor::FromHex(TEXT("3F3046"))); Tunic = FLinearColor(FColor::FromHex(TEXT("55603A"))); }
	if (M.Id == FName(TEXT("oda"))) { Tunic = FLinearColor(FColor::FromHex(TEXT("5B6E7C"))); }
	BuildHumanoid(M.House, Cloak, Tunic, 1.f, false, M.House == EShelfHouse::Mirrowen || M.House == EShelfHouse::Thessal);
	GetCharacterMovement()->MaxWalkSpeed = 430.f + Spd * 6.f;
}

void AShelfCharacter::InitEnemy(FName Key)
{
	const FEnemyDef* E = ShelfData::Enemy(Key);
	if (!E) return;
	EnemyKey = Key; DisplayName = E->Name; House = E->Kind; Team = EShelfTeam::Enemy; bBoss = E->bBoss;
	MaxHP = HP = E->Hp; Atk = E->Atk; Def = E->Def; Spd = E->Spd; AttackRange = E->Range; Skills = E->Skills;
	XpValue = E->Xp; SaltValue = E->Salt; Shape = E->Shape;
	if (Is(Shape, TEXT("hound"))) BuildHound();
	else if (Is(Shape, TEXT("swine"))) BuildSwine();
	else if (Is(Shape, TEXT("moths"))) BuildMoths();
	else
	{
		const bool bSellim = Is(Key, TEXT("sellim"));
		FLinearColor Cloak = bSellim ? FLinearColor(0.02f, 0.02f, 0.025f) : Is(Key, TEXT("broker")) ? FLinearColor(0.07f, 0.025f, 0.025f) : FLinearColor(0.04f, 0.035f, 0.02f);
		FLinearColor Tunic = bSellim ? FLinearColor(0.05f, 0.055f, 0.06f) : ShelfData::House(E->Kind).Tunic;
		BuildHumanoid(E->Kind, Cloak, Tunic, bSellim ? 1.12f : 1.f, bSellim, Is(Key, TEXT("cutpurse")));
	}
	GetCharacterMovement()->MaxWalkSpeed = 300.f + Spd * 12.f;
}

void AShelfCharacter::InitNPC(EShelfHouse InHouse, const FString& Name, FLinearColor Cloak, FLinearColor Tunic)
{
	DisplayName = Name; House = InHouse; Team = EShelfTeam::Neutral;
	BuildHumanoid(InHouse, Cloak, Tunic, 1.f, false, InHouse == EShelfHouse::Mirrowen || InHouse == EShelfHouse::Thessal);
}

void AShelfCharacter::Revive(float Fraction)
{
	bDead = false;
	HP = FMath::Max(1, FMath::CeilToInt(MaxHP * Fraction));
	St = FShelfStatus();
	CastTotal = 0.f;
	VisualRoot->SetRelativeRotation(FRotator::ZeroRotator);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SetActorHiddenInGame(false);
}

// ===================================================================== queries

bool AShelfCharacter::IsHostileTo(const AShelfCharacter* O) const
{
	if (!O || O == this || !O->IsAlive()) return false;
	if (Team == EShelfTeam::Party) return O->Team == EShelfTeam::Enemy && O->bHostile;
	if (Team == EShelfTeam::Enemy) return bHostile && O->Team == EShelfTeam::Party;
	return false;
}

float AShelfCharacter::Dist2D(const AActor* O) const
{
	return O ? FVector::Dist2D(GetActorLocation(), O->GetActorLocation()) : TNumericLimits<float>::Max();
}

float AShelfCharacter::AttackInterval() const
{
	float I = FMath::Clamp(2.6f - Spd * 0.12f, 1.1f, 2.6f);
	if (St.Heavy > 0) I *= 1.5f;
	if (St.Green > 0) I /= 1.6f;
	return I;
}

TArray<AShelfCharacter*> AShelfCharacter::Allies() const
{
	TArray<AShelfCharacter*> Out;
	if (AShelfWorld* W = AShelfWorld::Get(this))
		for (AShelfCharacter* C : W->AllCharacters()) if (C && C->IsAlive() && C->Team == Team) Out.Add(C);
	return Out;
}

TArray<AShelfCharacter*> AShelfCharacter::Foes() const
{
	TArray<AShelfCharacter*> Out;
	if (AShelfWorld* W = AShelfWorld::Get(this))
		for (AShelfCharacter* C : W->AllCharacters()) if (IsHostileTo(C)) Out.Add(C);
	return Out;
}

AShelfCharacter* AShelfCharacter::PickAllyLowest(bool bExcludeSelf) const
{
	AShelfCharacter* Best = nullptr; float BestPct = 2.f;
	for (AShelfCharacter* C : Allies())
	{
		if (bExcludeSelf && C == this) continue;
		const float Pct = float(C->HP) / float(C->MaxHP);
		if (Pct < BestPct) { BestPct = Pct; Best = C; }
	}
	return Best ? Best : const_cast<AShelfCharacter*>(this);
}

AShelfCharacter* AShelfCharacter::NearestHostile(float MaxDist) const
{
	AShelfCharacter* Best = nullptr; float BestD = MaxDist;
	for (AShelfCharacter* C : Foes())
	{
		if (C->Team == EShelfTeam::Party && C->St.Unreg > 0) continue;
		const float D = Dist2D(C);
		if (D < BestD) { BestD = D; Best = C; }
	}
	return Best;
}

void AShelfCharacter::FaceTowards(const FVector& Loc)
{
	FVector Dir = Loc - GetActorLocation(); Dir.Z = 0;
	if (!Dir.IsNearlyZero()) SetActorRotation(Dir.Rotation());
}

// ===================================================================== damage

int32 AShelfCharacter::CalcDamage(const AShelfCharacter* Src, float Mult, bool bPierce) const
{
	float D = Src->Atk * Mult * FMath::FRandRange(0.9f, 1.1f);
	if (Src->St.Green > 0) D *= 1.6f;
	if (Src->Team == EShelfTeam::Party)
		if (AShelfWorld* W = AShelfWorld::Get(this)) if (W->IsSleepless()) D *= 0.85f;
	if (!bPierce) D -= Def * 0.6f;
	D = FMath::Max(1.f, D);
	if (St.Light > 0) D *= 1.5f;
	return FMath::RoundToInt(D);
}

int32 AShelfCharacter::TakeHit(AShelfCharacter* Src, int32 Damage, bool bCanBeRedirected, FColor Color)
{
	if (bDead) return 0;
	AShelfWorld* W = AShelfWorld::Get(this);
	if (Src && Src->St.bWeak && bCanBeRedirected) { Damage = FMath::Max(1, Damage / 2); Src->St.bWeak = false; }
	if (bCanBeRedirected && St.bEvade)
	{
		St.bEvade = false;
		if (W) W->Float(GetActorLocation(), TEXT("Evaded"), ColBlue);
		return 0;
	}
	if (bCanBeRedirected && St.Guard > 0 && St.GuardBy.IsValid() && St.GuardBy->IsAlive() && St.GuardBy.Get() != this)
	{
		AShelfCharacter* G = St.GuardBy.Get();
		const int32 Half = Damage / 2;
		Damage -= Half;
		G->TakeHit(Src, Half, false, Color);
		G->St.Load = FMath::Min(G->St.Load + 1, 8);
	}
	if (St.Stay > 0 && HP - Damage < 1) Damage = FMath::Max(0, HP - 1);
	HP = FMath::Max(0, HP - Damage);
	FlashT = 0.15f;
	if (W) W->Float(GetActorLocation(), FString::Printf(TEXT("-%d"), Damage), Color);
	if (Src && Team == EShelfTeam::Enemy && !bHostile) bHostile = true;
	if (Src && Team == EShelfTeam::Enemy && !Target.IsValid()) Target = Src;
	if (HP <= 0) Die();
	return Damage;
}

void AShelfCharacter::Heal(int32 Amount)
{
	if (bDead) return;
	Amount = FMath::Min(Amount, MaxHP - HP);
	HP += Amount;
	if (AShelfWorld* W = AShelfWorld::Get(this)) W->Float(GetActorLocation(), FString::Printf(TEXT("+%d"), Amount), ColHeal);
}

void AShelfCharacter::AddRot(int32 N)
{
	if (bDead) return;
	St.Rot = FMath::Min(St.Rot + N, 8);
}

void AShelfCharacter::Die()
{
	bDead = true;
	Target = nullptr;
	Path.Reset();
	CastTotal = 0.f;
	GetCharacterMovement()->StopMovementImmediately();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualRoot->SetRelativeRotation(FRotator(0.f, 0.f, 80.f));
	if (AShelfWorld* W = AShelfWorld::Get(this)) W->OnCharacterDown(this);
}

// ===================================================================== tick

void AShelfCharacter::Tick(float Dt)
{
	Super::Tick(Dt);
	TickVisuals(Dt);
	if (bDead) return;
	AShelfWorld* W = AShelfWorld::Get(this);
	if (W && W->IsCombatPaused()) return;
	TickStatus(Dt);
	if (bDead) return;
	TickMovement(Dt);
	TickCombat(Dt);
}

void AShelfCharacter::TickStatus(float Dt)
{
	auto Dec = [Dt](float& V) { V = FMath::Max(0.f, V - Dt); };
	Dec(St.Heavy); Dec(St.Light); Dec(St.Set); Dec(St.Stare); Dec(St.Stay); Dec(St.Unreg); Dec(St.Green); Dec(St.Guard); Dec(GCD);
	for (auto& Pair : Cooldowns) Pair.Value = FMath::Max(0.f, Pair.Value - Dt);
	if (Team == EShelfTeam::Party) Focus = FMath::Min(MaxFocus, Focus + Dt / 3.f);

	St.DotTimer += Dt;
	if (St.DotTimer >= 2.f)
	{
		St.DotTimer = 0.f;
		if (St.Rot > 0 && St.Stay <= 0) { TakeHit(nullptr, St.Rot * 3, false, ColRot); St.Rot--; }
		if (!bDead && St.Bond > 0) TakeHit(nullptr, St.Bond * 2, false, ColBond);
	}
	if (St.DebtAmount > 0)
	{
		St.DebtTimer -= Dt;
		if (St.DebtTimer <= 0.f) { TakeHit(nullptr, St.DebtAmount, false, FColor(201, 167, 255)); St.DebtAmount = 0; }
	}
}

void AShelfCharacter::TickMovement(float Dt)
{
	if (St.IsStunned() || IsCasting()) return;
	while (Path.Num() && FVector::Dist2D(GetActorLocation(), Path[0]) < 45.f) Path.RemoveAt(0);
	if (Path.Num())
	{
		FVector Dir = Path[0] - GetActorLocation(); Dir.Z = 0;
		AddMovementInput(Dir.GetSafeNormal(), St.Heavy > 0 ? 0.5f : 1.f);
	}
}

void AShelfCharacter::TickCombat(float Dt)
{
	if (Target.IsValid() && !Target->IsAlive()) Target = nullptr;

	if (IsCasting())
	{
		if (St.IsStunned())
		{
			CastTotal = 0.f;
			if (AShelfWorld* W = AShelfWorld::Get(this)) W->Toast(FString::Printf(TEXT("%s's focus breaks. The charge is lost."), *DisplayName));
			return;
		}
		CastTime += Dt;
		if (CastTime >= CastTotal)
		{
			const FName S = CastSkill;
			CastTotal = 0.f;
			DoEnemySkill(S, Target.Get());
		}
		return;
	}
	if (St.IsStunned()) return;

	AShelfCharacter* T = Target.Get();
	if (!T || !IsHostileTo(T)) return;
	if (Team == EShelfTeam::Enemy && T->St.Unreg > 0) { Target = nullptr; return; }
	AttackTimer -= Dt;
	if (Dist2D(T) <= AttackRange + 40.f)
	{
		Path.Reset();
		FaceTowards(T->GetActorLocation());
		if (AttackTimer <= 0.f)
		{
			AttackTimer = AttackInterval();
			AutoAttack();
		}
	}
}

void AShelfCharacter::AutoAttack()
{
	AShelfCharacter* T = Target.Get();
	if (!T) return;
	if (Team == EShelfTeam::Party)
	{
		const FName Basic = Abilities.Num() ? Abilities[0] : NAME_None;
		const float Mult = (Is(Basic, TEXT("touch")) || Is(Basic, TEXT("glare"))) ? 0.8f : 1.f;
		T->TakeHit(this, T->CalcDamage(this, Mult, false));
		if (Is(Basic, TEXT("touch"))) T->AddRot(1);
		if (Is(Basic, TEXT("glare"))) St.Regard = FMath::Min(St.Regard + 1, 8);
	}
	else
	{
		const FName Basic = Skills.Num() ? Skills[0] : FName(TEXT("stab"));
		DoEnemySkill(Basic, T);
	}
}

void AShelfCharacter::TickVisuals(float Dt)
{
	AnimT += Dt;
	FlashT = FMath::Max(0.f, FlashT - Dt);
	const float Speed = GetVelocity().Size2D();
	const float Swing = Speed > 20.f && !bDead ? FMath::Sin(AnimT * 10.f) * 30.f : 0.f;
	for (int32 i = 0; i < Legs.Num(); ++i)
		if (Legs[i]) Legs[i]->SetRelativeRotation(FRotator((i % 2 ? 1.f : -1.f) * (i >= 2 ? -1.f : 1.f) * Swing, 0.f, 0.f));
	for (int32 i = 0; i < Arms.Num(); ++i)
		if (Arms[i]) Arms[i]->SetRelativeRotation(FRotator((i % 2 ? -1.f : 1.f) * Swing * 0.6f, 0.f, 0.f));
	for (int32 i = 0; i < Orbiters.Num(); ++i)
	{
		const float A = AnimT * 2.4f + i * 1.05f;
		if (Orbiters[i]) Orbiters[i]->SetRelativeLocation(FVector(FMath::Cos(A) * 45.f, FMath::Sin(A) * 45.f, 110.f + FMath::Sin(A * 1.7f) * 30.f));
	}
	if (!bDead) VisualRoot->SetRelativeLocation(FVector(0.f, 0.f, -88.f + (FlashT > 0 ? 6.f : 0.f)));
}

// ===================================================================== abilities

FString AShelfCharacter::AbilityBlockReason(FName Id) const
{
	const FAbilityDef* D = ShelfData::Ability(Id);
	if (!D || !Abilities.Contains(Id)) return TEXT("Not learned yet");
	if (bDead) return TEXT("Down");
	if (St.IsStunned()) return TEXT("Can't act");
	if (GCD > 0) return TEXT("Not ready");
	if (const float* Cd = Cooldowns.Find(Id)) if (*Cd > 0) return TEXT("Not ready");
	if (Focus < D->Cost) return TEXT("Not enough Focus");
	AShelfCharacter* T = Target.Get();
	if (D->Target == EAbilityTarget::Enemy)
	{
		if (!T || !IsHostileTo(T)) return TEXT("No target");
		if (Dist2D(T) > D->Range + 60.f) return TEXT("Too far away");
	}
	if (Is(Id, TEXT("overburden")) && St.Load <= 0) return TEXT("No Load stored");
	if (Is(Id, TEXT("distraint")) && (!T || T->St.Bond <= 0)) return TEXT("Target has no Bond");
	if (Is(Id, TEXT("scrutiny")) && St.Regard <= 0) return TEXT("No Regard");
	if (Is(Id, TEXT("persist")) && St.Regard < 3) return TEXT("Needs 3 Regard");
	if (Is(Id, TEXT("assign")))
	{
		bool bAny = false;
		for (AShelfCharacter* F : Foes()) if (F->St.Bond > 0) bAny = true;
		if (!bAny) return TEXT("No bonded enemy");
	}
	return FString();
}

bool AShelfCharacter::TryAbility(FName Id)
{
	const FString Why = AbilityBlockReason(Id);
	if (!Why.IsEmpty())
	{
		if (Team == EShelfTeam::Party && !bCompanion)
			if (AShelfWorld* W = AShelfWorld::Get(this)) W->Toast(Why, 1.2f);
		return false;
	}
	const FAbilityDef* D = ShelfData::Ability(Id);
	Focus -= D->Cost;
	Cooldowns.Add(Id, D->Cooldown);
	GCD = 1.0f;
	AShelfCharacter* T = nullptr;
	switch (D->Target)
	{
	case EAbilityTarget::Enemy: T = Target.Get(); break;
	case EAbilityTarget::Ally: T = PickAllyLowest(Is(Id, TEXT("shoulder"))); break;
	default: T = this; break;
	}
	if (T && T != this) FaceTowards(T->GetActorLocation());
	ApplyAbility(Id, T);
	if (AShelfWorld* W = AShelfWorld::Get(this)) W->Float(GetActorLocation() + FVector(0, 0, 60), D->Name, FColor(240, 220, 160));
	return true;
}

void AShelfCharacter::ApplyAbility(FName Id, AShelfCharacter* T)
{
	AShelfWorld* W = AShelfWorld::Get(this);
	auto Fl = [W](AShelfCharacter* C, const TCHAR* Txt, FColor Col) { if (W && C) W->Float(C->GetActorLocation(), Txt, Col); };

	// ---- Undrum ----
	if (Is(Id, TEXT("shoulder"))) { T->St.Guard = 8.f; T->St.GuardBy = this; Fl(T, TEXT("Shouldered"), ColGold); }
	else if (Is(Id, TEXT("plumb"))) { T->TakeHit(this, T->CalcDamage(this, 1.3f, false)); T->St.Heavy = 6.f; }
	else if (Is(Id, TEXT("set"))) { T->St.Set = 3.f; St.Light = 3.f; Fl(T, TEXT("Set"), ColGold); }
	else if (Is(Id, TEXT("overburden"))) { const int32 L = St.Load; St.Load = 0; T->TakeHit(this, T->CalcDamage(this, 1.f + L, false)); }
	// ---- Saldric ----
	else if (Is(Id, TEXT("bind"))) { T->St.Bond = FMath::Min(6, T->St.Bond + 2); if (W) W->Float(T->GetActorLocation(), FString::Printf(TEXT("Bond %d"), T->St.Bond), ColBond); }
	else if (Is(Id, TEXT("distraint"))) { const int32 B = T->St.Bond; T->St.Bond = 0; const int32 Dm = T->TakeHit(this, T->CalcDamage(this, 0.9f * B, true)); Heal(Dm / 2); }
	else if (Is(Id, TEXT("assign")))
	{
		AShelfCharacter* F = nullptr;
		for (AShelfCharacter* C : Foes()) if (!F || C->St.Bond > F->St.Bond) F = C;
		const int32 Amt = FMath::RoundToInt(T->MaxHP * 0.35f);
		T->Heal(Amt); T->St.Rot = 0;
		if (F) F->TakeHit(this, Amt, false, ColBond);
	}
	else if (Is(Id, TEXT("forgive"))) { for (AShelfCharacter* F : Foes()) if (F->St.Bond > 0) { const int32 B = F->St.Bond; F->St.Bond = 0; F->TakeHit(this, B * 7, false, ColBond); } }
	// ---- Mirrowen ----
	else if (Is(Id, TEXT("quicken"))) { const int32 R = T->St.Rot; T->St.Rot = 0; T->TakeHit(this, T->CalcDamage(this, 1.f, false) + R * (R + 1) / 2 * 3, true, ColRot); }
	else if (Is(Id, TEXT("stay"))) { T->St.Stay = 6.f; Fl(T, TEXT("Stayed"), ColRot); }
	else if (Is(Id, TEXT("draught"))) { T->Heal(FMath::RoundToInt(T->MaxHP * 0.4f)); T->St.DebtAmount = FMath::RoundToInt(T->MaxHP * 0.1f); T->St.DebtTimer = 8.f; }
	else if (Is(Id, TEXT("longgreen")))
	{
		const int32 Cost = FMath::RoundToInt(MaxHP * 0.2f);
		MaxHP -= Cost; HP = FMath::Max(1, HP - Cost); St.Green = 12.f;
		if (W) W->Float(GetActorLocation(), FString::Printf(TEXT("-%d years"), Cost), FColor(127, 227, 106));
	}
	// ---- Ikorran ----
	else if (Is(Id, TEXT("handthrough"))) { T->TakeHit(this, T->CalcDamage(this, 1.6f, true), true, ColBlue); }
	else if (Is(Id, TEXT("step")))
	{
		FVector Away = GetActorForwardVector() * -1.f;
		if (Target.IsValid()) { Away = GetActorLocation() - Target->GetActorLocation(); Away.Z = 0; Away.Normalize(); }
		const FVector Dest = GetActorLocation() + Away * 400.f;
		if (W && W->IsWalkable(W->WorldToTile(Dest))) SetActorLocation(Dest, false, nullptr, ETeleportType::TeleportPhysics);
		St.bEvade = true; Fl(this, TEXT("Step"), ColBlue);
	}
	else if (Is(Id, TEXT("stretch"))) { T->TakeHit(this, T->CalcDamage(this, 0.6f, false)); T->St.Heavy = 6.f; T->St.bWeak = true; Fl(T, TEXT("Stretched"), ColBlue); }
	else if (Is(Id, TEXT("attenuate"))) { T->TakeHit(this, T->CalcDamage(this, 1.2f, true), true, ColBlue); T->AddRot(3); }
	// ---- Thessal ----
	else if (Is(Id, TEXT("scrutiny"))) { const int32 R = St.Regard; St.Regard = 0; T->TakeHit(this, T->CalcDamage(this, 0.75f * R, true), true, ColRegard); }
	else if (Is(Id, TEXT("fix")))
	{
		const float Chance = T->bBoss ? 0.3f : 0.75f;
		if (FMath::FRand() < Chance) { T->St.Stare = 3.f; Fl(T, TEXT("Fixed"), ColRegard); }
		else Fl(T, TEXT("Resisted"), FColor::White);
	}
	else if (Is(Id, TEXT("unregard"))) { T->St.Unreg = 5.f; Fl(T, TEXT("Unregarded"), ColRegard); for (AShelfCharacter* F : Foes()) if (F->Target.Get() == T) F->Target = nullptr; }
	else if (Is(Id, TEXT("persist"))) { St.Regard -= 3; T->Heal(FMath::RoundToInt(T->MaxHP * 0.6f)); T->St.Rot = 0; T->St.Stay = 3.f; }
}

// ===================================================================== enemy skills

void AShelfCharacter::StartCast(FName Skill, float Duration, const FString& LabelText)
{
	CastSkill = Skill; CastTime = 0.f; CastTotal = Duration; CastLabel = LabelText;
	Path.Reset();
	if (AShelfWorld* W = AShelfWorld::Get(this)) W->Toast(LabelText, 3.5f);
}

void AShelfCharacter::DoEnemySkill(FName Skill, AShelfCharacter* T)
{
	AShelfWorld* W = AShelfWorld::Get(this);
	TArray<AShelfCharacter*> Party = Foes();
	if (Is(Skill, TEXT("shear")))
	{
		for (AShelfCharacter* P : Party) if (Dist2D(P) < 1600.f) P->TakeHit(this, P->CalcDamage(this, 1.45f, false), true, FColor(255, 179, 107));
		if (W) W->Toast(TEXT("Sellim releases it all. SHEAR."), 2.f);
		return;
	}
	if (Is(Skill, TEXT("trample"))) { for (AShelfCharacter* P : Party) if (Dist2D(P) < 450.f) P->TakeHit(this, P->CalcDamage(this, 0.65f, false)); return; }
	if (Is(Skill, TEXT("spores"))) { for (AShelfCharacter* P : Party) if (Dist2D(P) < 700.f) { P->TakeHit(this, P->CalcDamage(this, 0.45f, false), true, ColRot); P->AddRot(1); } return; }
	if (!T || !T->IsAlive()) return;
	FaceTowards(T->GetActorLocation());
	if (Is(Skill, TEXT("stab")) || Is(Skill, TEXT("maul")) || Is(Skill, TEXT("gore"))) T->TakeHit(this, T->CalcDamage(this, 1.f, false));
	else if (Is(Skill, TEXT("cable"))) { T->TakeHit(this, T->CalcDamage(this, 1.15f, false)); if (FMath::FRand() < 0.5f) T->AddRot(1); }
	else if (Is(Skill, TEXT("bite"))) { if (T->TakeHit(this, T->CalcDamage(this, 0.9f, false)) > 0) T->AddRot(1); }
	else if (Is(Skill, TEXT("hthru"))) T->TakeHit(this, T->CalcDamage(this, 1.35f, true), true, ColBlue);
	else if (Is(Skill, TEXT("bindE"))) { T->St.Bond = FMath::Min(6, T->St.Bond + 2); if (W) W->Float(T->GetActorLocation(), FString::Printf(TEXT("Bond %d"), T->St.Bond), ColBond); }
	else if (Is(Skill, TEXT("collect")))
	{
		AShelfCharacter* B = nullptr;
		for (AShelfCharacter* P : Party) if (P->St.Bond > 0 && (!B || P->St.Bond > B->St.Bond)) B = P;
		if (!B) { T->TakeHit(this, T->CalcDamage(this, 1.f, false)); return; }
		const int32 Dm = B->TakeHit(this, FMath::RoundToInt(Atk * 0.8f * B->St.Bond), false, ColBond);
		B->St.Bond = 0; Heal(Dm / 2);
	}
	else if (Is(Skill, TEXT("plumbE"))) { T->TakeHit(this, T->CalcDamage(this, 1.2f, false)); T->St.Heavy = 6.f; }
	else T->TakeHit(this, T->CalcDamage(this, 1.f, false));
}

// ===================================================================== visuals

UStaticMeshComponent* AShelfCharacter::AddPart(UStaticMesh* Mesh, FVector Loc, FVector Scale, FLinearColor Color, FRotator Rot, USceneComponent* Parent)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetStaticMesh(Mesh);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCastShadow(true);
	C->RegisterComponent();
	C->AttachToComponent(Parent ? Parent : VisualRoot.Get(), FAttachmentTransformRules::KeepRelativeTransform);
	C->SetRelativeLocationAndRotation(Loc, Rot);
	C->SetRelativeScale3D(Scale);
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(ShelfAssets::BaseMaterial(), this);
	MID->SetVectorParameterValue(TEXT("Color"), Color);
	C->SetMaterial(0, MID);
	Parts.Add(C);
	return C;
}

USceneComponent* AShelfCharacter::AddPivot(FVector Loc, USceneComponent* Parent)
{
	USceneComponent* P = NewObject<USceneComponent>(this);
	P->RegisterComponent();
	P->AttachToComponent(Parent ? Parent : VisualRoot.Get(), FAttachmentTransformRules::KeepRelativeTransform);
	P->SetRelativeLocation(Loc);
	return P;
}

void AShelfCharacter::ClearVisuals()
{
	for (UStaticMeshComponent* P : Parts) if (P) P->DestroyComponent();
	for (USceneComponent* P : Legs) if (P) P->DestroyComponent();
	for (USceneComponent* P : Arms) if (P) P->DestroyComponent();
	for (USceneComponent* P : Orbiters) if (P) P->DestroyComponent();
	Parts.Reset(); Legs.Reset(); Arms.Reset(); Orbiters.Reset();
}

// Engine basic shapes are 100 units across; Cylinder/Cube are centred, Cone points up.
void AShelfCharacter::BuildHumanoid(EShelfHouse InHouse, FLinearColor Cloak, FLinearColor Tunic, float S, bool bBlade, bool bHood)
{
	ClearVisuals();
	const FLinearColor Skin = FLinearColor(FColor::FromHex(TEXT("D9A982"))) * FMath::FRandRange(0.6f, 1.05f);
	const FLinearColor Pants(0.03f, 0.025f, 0.02f), Boot(0.015f, 0.012f, 0.01f);
	const float Bulk = InHouse == EShelfHouse::Undrum ? 1.25f : 1.f;
	VisualRoot->SetRelativeScale3D(FVector(S));
	for (int32 i = 0; i < 2; ++i)
	{
		const float X = (i ? 1.f : -1.f) * 11.f * Bulk;
		USceneComponent* Hip = AddPivot(FVector(0.f, X, 90.f));
		AddPart(ShelfAssets::Cylinder(), FVector(0, 0, -42.f), FVector(0.15f, 0.15f, 0.84f), Pants, FRotator::ZeroRotator, Hip);
		AddPart(ShelfAssets::Cube(), FVector(5.f, 0, -86.f), FVector(0.24f, 0.14f, 0.1f), Boot, FRotator::ZeroRotator, Hip);
		Legs.Add(Hip);
	}
	AddPart(ShelfAssets::Cylinder(), FVector(0, 0, 122.f), FVector(0.38f * Bulk, 0.42f * Bulk, 0.66f), Tunic);
	AddPart(ShelfAssets::Cone(), FVector(-4.f, 0, 112.f), FVector(0.62f * Bulk, 0.62f * Bulk, 1.05f), Cloak);
	AddPart(ShelfAssets::Sphere(), FVector(0, 0, 172.f), FVector(0.3f), Skin);
	if (bHood) AddPart(ShelfAssets::Sphere(), FVector(-3.f, 0, 176.f), FVector(0.36f, 0.36f, 0.36f), Cloak);
	else AddPart(ShelfAssets::Sphere(), FVector(-3.f, 0, 178.f), FVector(0.31f, 0.31f, 0.24f), FLinearColor(0.03f, 0.02f, 0.015f));
	for (int32 i = 0; i < 2; ++i)
	{
		const float Y = (i ? 1.f : -1.f) * 26.f * Bulk;
		USceneComponent* Sh = AddPivot(FVector(0.f, Y, 148.f));
		AddPart(ShelfAssets::Cylinder(), FVector(0, 0, -26.f), FVector(0.11f, 0.11f, 0.5f), Tunic * 0.9f, FRotator::ZeroRotator, Sh);
		AddPart(ShelfAssets::Sphere(), FVector(0, 0, -54.f), FVector(0.1f), Skin, FRotator::ZeroRotator, Sh);
		Arms.Add(Sh);
	}
	USceneComponent* Hand = Arms.Num() > 1 ? Arms[1].Get() : VisualRoot.Get();
	switch (InHouse)
	{
	case EShelfHouse::Undrum:
		for (int32 i = 0; i < 2; ++i) AddPart(ShelfAssets::Sphere(), FVector(0, (i ? 1.f : -1.f) * 30.f, 152.f), FVector(0.2f), FLinearColor(0.25f, 0.16f, 0.06f));
		if (bBlade) AddPart(ShelfAssets::Cube(), FVector(20.f, 0, -60.f), FVector(0.6f, 0.03f, 0.06f), FLinearColor(0.7f, 0.72f, 0.75f), FRotator::ZeroRotator, Hand);
		else { AddPart(ShelfAssets::Cylinder(), FVector(10.f, 0, -70.f), FVector(0.05f, 0.05f, 0.9f), FLinearColor(0.1f, 0.06f, 0.03f), FRotator(70.f, 0, 0), Hand);
			AddPart(ShelfAssets::Cube(), FVector(48.f, 0, -84.f), FVector(0.22f, 0.15f, 0.15f), FLinearColor(0.15f, 0.15f, 0.14f), FRotator::ZeroRotator, Hand); }
		break;
	case EShelfHouse::Saldric:
		AddPart(ShelfAssets::Cube(), FVector(10.f, 0, -55.f), FVector(0.05f, 0.18f, 0.22f), FLinearColor(0.85f, 0.8f, 0.68f), FRotator::ZeroRotator, Arms[0]);
		AddPart(ShelfAssets::Cube(), FVector(25.f, 0, -60.f), FVector(0.45f, 0.02f, 0.04f), FLinearColor(0.75f, 0.78f, 0.8f), FRotator::ZeroRotator, Hand);
		break;
	case EShelfHouse::Mirrowen:
		AddPart(ShelfAssets::Cylinder(), FVector(8.f, 0, -62.f), FVector(0.03f, 0.03f, 0.35f), FLinearColor(0.1f, 0.06f, 0.03f), FRotator(60.f, 0, 0), Hand);
		AddPart(ShelfAssets::Cube(), FVector(26.f, 0, -66.f), FVector(0.3f, 0.02f, 0.05f), FLinearColor(0.4f, 0.45f, 0.3f), FRotator(0, 0, 30.f), Hand);
		for (int32 i = 0; i < 3; ++i) AddPart(ShelfAssets::Sphere(), FVector(18.f, -14.f + i * 14.f, 92.f), FVector(0.08f), FLinearColor(0.5f, 0.9f, 0.2f));
		break;
	case EShelfHouse::Ikorran:
		AddPart(ShelfAssets::Cylinder(), FVector(10.f, 0, -55.f), FVector(0.03f, 0.03f, 1.2f), FLinearColor(0.25f, 0.15f, 0.06f), FRotator(0, 0, 0), Arms[0]);
		AddPart(ShelfAssets::Cylinder(), FVector(0, 0, 150.f), FVector(0.46f, 0.46f, 0.08f), FLinearColor(0.45f, 0.1f, 0.08f));
		break;
	case EShelfHouse::Thessal:
		AddPart(ShelfAssets::Sphere(), FVector(14.f, 0, 174.f), FVector(0.08f, 0.24f, 0.24f), FLinearColor(1.f, 0.95f, 0.85f));
		AddPart(ShelfAssets::Cylinder(), FVector(8.f, 0, -20.f), FVector(0.03f, 0.03f, 1.8f), FLinearColor(0.2f, 0.16f, 0.1f), FRotator::ZeroRotator, Hand);
		AddPart(ShelfAssets::Cylinder(), FVector(8.f, 0, 72.f), FVector(0.24f, 0.24f, 0.03f), FLinearColor(1.f, 0.92f, 0.6f), FRotator(90.f, 0, 0), Hand);
		break;
	default: break;
	}
}

void AShelfCharacter::BuildHound()
{
	ClearVisuals();
	const FLinearColor C(0.17f, 0.18f, 0.11f), D(0.09f, 0.1f, 0.06f);
	AddPart(ShelfAssets::Cylinder(), FVector(0, 0, 62.f), FVector(0.32f, 0.32f, 0.9f), C, FRotator(90.f, 0, 0));
	AddPart(ShelfAssets::Sphere(), FVector(52.f, 0, 80.f), FVector(0.3f), C);
	AddPart(ShelfAssets::Cube(), FVector(72.f, 0, 74.f), FVector(0.25f, 0.15f, 0.13f), D);
	for (int32 i = 0; i < 4; ++i)
	{
		USceneComponent* L = AddPivot(FVector(i < 2 ? 30.f : -30.f, i % 2 ? 12.f : -12.f, 55.f));
		AddPart(ShelfAssets::Cylinder(), FVector(0, 0, -27.f), FVector(0.07f, 0.07f, 0.55f), D, FRotator::ZeroRotator, L);
		Legs.Add(L);
	}
	for (int32 i = 0; i < 5; ++i) AddPart(ShelfAssets::Sphere(), FVector(FMath::FRandRange(-40.f, 40.f), FMath::FRandRange(-15.f, 15.f), FMath::FRandRange(85.f, 100.f)), FVector(0.06f), FLinearColor(0.5f, 0.8f, 0.2f));
}

void AShelfCharacter::BuildSwine()
{
	ClearVisuals();
	const FLinearColor C(0.6f, 0.42f, 0.38f), D(0.3f, 0.2f, 0.18f);
	VisualRoot->SetRelativeScale3D(FVector(1.3f));
	AddPart(ShelfAssets::Sphere(), FVector(0, 0, 105.f), FVector(1.25f, 0.75f, 0.7f), C);
	AddPart(ShelfAssets::Sphere(), FVector(75.f, 0, 112.f), FVector(0.45f), C);
	AddPart(ShelfAssets::Cylinder(), FVector(100.f, 0, 104.f), FVector(0.18f, 0.18f, 0.15f), D, FRotator(90.f, 0, 0));
	for (int32 i = 0; i < 4; ++i)
	{
		USceneComponent* L = AddPivot(FVector(i < 2 ? 38.f : -38.f, i % 2 ? 22.f : -22.f, 80.f));
		AddPart(ShelfAssets::Cylinder(), FVector(0, 0, -40.f), FVector(0.1f, 0.1f, 0.8f), D, FRotator::ZeroRotator, L);
		Legs.Add(L);
	}
}

void AShelfCharacter::BuildMoths()
{
	ClearVisuals();
	for (int32 i = 0; i < 6; ++i)
	{
		USceneComponent* M = AddPivot(FVector(0, 0, 110.f));
		AddPart(ShelfAssets::Sphere(), FVector::ZeroVector, FVector(0.08f, 0.08f, 0.16f), FLinearColor(0.2f, 0.16f, 0.1f), FRotator::ZeroRotator, M);
		AddPart(ShelfAssets::Cube(), FVector(0, 0, 0), FVector(0.02f, 0.4f, 0.22f), FLinearColor(0.7f, 0.62f, 0.45f), FRotator::ZeroRotator, M);
		Orbiters.Add(M);
	}
	GetCharacterMovement()->GravityScale = 1.f;
}
