#include "ShelfAIController.h"
#include "ShelfCharacter.h"
#include "ShelfWorld.h"

AShelfAIController::AShelfAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AShelfAIController::Tick(float Dt)
{
	Super::Tick(Dt);
	AShelfCharacter* Me = Cast<AShelfCharacter>(GetPawn());
	AShelfWorld* W = AShelfWorld::Get(this);
	if (!Me || !W || !Me->IsAlive() || W->IsCombatPaused()) return;
	if (Me->Team == EShelfTeam::Party && Me->bCompanion) TickCompanion(Me, Dt);
	else if (Me->Team == EShelfTeam::Enemy) TickEnemy(Me, Dt);
}

void AShelfAIController::MoveTowards(AShelfCharacter* Me, const FVector& Goal, float StopDist)
{
	if (FVector::Dist2D(Me->GetActorLocation(), Goal) <= StopDist) { Me->ClearPath(); return; }
	if (RepathTimer > 0.f && Me->Path.Num()) return;
	RepathTimer = 0.5f;
	if (AShelfWorld* W = AShelfWorld::Get(this)) Me->SetPath(W->FindPath(Me->GetActorLocation(), Goal));
}

void AShelfAIController::TickCompanion(AShelfCharacter* Me, float Dt)
{
	RepathTimer -= Dt; ThinkTimer -= Dt;
	AShelfWorld* W = AShelfWorld::Get(this);
	AShelfCharacter* Leader = W->Leader();
	if (!Leader) return;

	// Pick a fight: whatever the leader is hitting, else whatever is hitting us.
	AShelfCharacter* T = Me->Target.Get();
	if (!T || !Me->IsHostileTo(T))
	{
		T = (Leader->Target.IsValid() && Me->IsHostileTo(Leader->Target.Get())) ? Leader->Target.Get() : nullptr;
		if (!T) for (AShelfCharacter* F : Me->Foes()) if (F->Target.IsValid() && F->Target->Team == EShelfTeam::Party && Me->Dist2D(F) < 1400.f) { T = F; break; }
		Me->Target = T;
	}

	if (ThinkTimer <= 0.f) { ThinkTimer = 0.8f; CompanionPowers(Me); }

	if (T)
	{
		if (Me->Dist2D(T) > Me->AttackRange) MoveTowards(Me, T->GetActorLocation(), Me->AttackRange - 20.f);
		return;
	}
	// Follow in formation behind the leader.
	const int32 Slot = W->PartyIndex(Me);
	const FVector Behind = Leader->GetActorLocation() - Leader->GetActorForwardVector() * 160.f + Leader->GetActorRightVector() * (Slot % 2 ? 110.f : -110.f);
	if (Me->Dist2D(Leader) > 320.f) MoveTowards(Me, Behind, 90.f);
	else Me->ClearPath();
	if (Me->Dist2D(Leader) > 2500.f) Me->SetActorLocation(Behind, false, nullptr, ETeleportType::TeleportPhysics);
}

void AShelfAIController::CompanionPowers(AShelfCharacter* Me)
{
	AShelfCharacter* Low = Me->PickAllyLowest(false);
	const float LowPct = Low ? float(Low->HP) / Low->MaxHP : 1.f;
	AShelfCharacter* T = Me->Target.Get();
	auto Try = [Me](const TCHAR* Id) { return Me->AbilityBlockReason(FName(Id)).IsEmpty() && Me->TryAbility(FName(Id)); };

	switch (Me->House)
	{
	case EShelfHouse::Mirrowen:
		if (LowPct < 0.25f && Try(TEXT("stay"))) return;
		if (LowPct < 0.55f && Try(TEXT("draught"))) return;
		if (T && T->St.Rot >= 3 && Try(TEXT("quicken"))) return;
		if (T && T->bBoss && Try(TEXT("longgreen"))) return;
		break;
	case EShelfHouse::Ikorran:
		if (T && T->IsCasting() && Try(TEXT("stretch"))) return;
		if (Me->HP < Me->MaxHP * 0.35f && Try(TEXT("step"))) return;
		if (T && Try(TEXT("attenuate"))) return;
		if (T && Me->Focus >= 3 && Try(TEXT("handthrough"))) return;
		break;
	case EShelfHouse::Undrum:
		if (T && T->IsCasting() && Try(TEXT("set"))) return;
		if (LowPct < 0.5f && Low != Me && Try(TEXT("shoulder"))) return;
		if (T && Me->St.Load >= 2 && Try(TEXT("overburden"))) return;
		if (T && Try(TEXT("plumb"))) return;
		break;
	case EShelfHouse::Saldric:
		if (LowPct < 0.45f && Try(TEXT("assign"))) return;
		if (T && T->St.Bond >= 4 && Try(TEXT("distraint"))) return;
		if (T && Try(TEXT("bind"))) return;
		break;
	case EShelfHouse::Thessal:
		if (T && T->IsCasting() && Try(TEXT("fix"))) return;
		if (LowPct < 0.35f && Try(TEXT("persist"))) return;
		if (LowPct < 0.4f && Low && Low->Team == EShelfTeam::Party && Try(TEXT("unregard"))) return;
		if (T && Me->St.Regard >= 4 && Try(TEXT("scrutiny"))) return;
		break;
	default: break;
	}
}

void AShelfAIController::TickEnemy(AShelfCharacter* Me, float Dt)
{
	RepathTimer -= Dt; SkillTimer -= Dt; ShearTimer -= Dt;
	if (!Me->bHostile)
	{
		// Wandering creatures aggro on sight; story enemies wait for their scene.
		AShelfWorld* W = AShelfWorld::Get(this);
		AShelfCharacter* L = W ? W->Leader() : nullptr;
		if (L && W->IsAmbient(Me) && Me->Dist2D(L) < 700.f) Me->bHostile = true;
		return;
	}
	AShelfCharacter* T = Me->Target.Get();
	if (!T || !Me->IsHostileTo(T) || T->St.Unreg > 0) { T = Me->NearestHostile(3000.f); Me->Target = T; }
	if (!T) { Me->ClearPath(); return; }
	if (Me->IsCasting() || Me->St.IsStunned()) return;

	// Sellim: shoulder the shelter's load, then Shear. Break it with Set, Fix or by stunning.
	if (Me->bBoss && ShearTimer <= 0.f)
	{
		ShearTimer = 16.f;
		Me->StartCast(TEXT("shear"), 4.f, TEXT("Sellim shoulders the shelter's whole weight - break her focus with Set or Fix!"));
		return;
	}
	if (SkillTimer <= 0.f && Me->Skills.Num() > 1)
	{
		SkillTimer = FMath::FRandRange(5.f, 8.f);
		TArray<FName> Opts;
		for (int32 i = 1; i < Me->Skills.Num(); ++i) if (Me->Skills[i] != FName(TEXT("shear"))) Opts.Add(Me->Skills[i]);
		if (Opts.Num() && Me->Dist2D(T) < Me->AttackRange + 300.f)
		{
			Me->DoEnemySkill(Opts[FMath::RandRange(0, Opts.Num() - 1)], T);
			Me->AttackTimer = Me->AttackInterval();
			return;
		}
	}
	if (Me->Dist2D(T) > Me->AttackRange) MoveTowards(Me, T->GetActorLocation(), Me->AttackRange - 20.f);
}
