#include "ShelfWorld.h"
#include "ShelfCharacter.h"
#include "ShelfStory.h"
#include "ShelfGameInstance.h"
#include "TheShelf.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	FLinearColor C(float R, float G, float B) { return FLinearColor(R, G, B); }
	FLinearColor Hex(const TCHAR* H) { return FLinearColor(FColor::FromHex(H)); }
	struct FRng { uint32 S; explicit FRng(uint32 Seed) : S(Seed ? Seed : 1) {} float Next() { S = S * 1664525u + 1013904223u; return (S >> 8) / float(1 << 24); } };
}

// ===================================================================== props

AShelfProp::AShelfProp()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

UStaticMeshComponent* AShelfProp::Part(UStaticMesh* Mesh, FVector Loc, FVector Scale, FLinearColor Color, FRotator Rot)
{
	UStaticMeshComponent* M = NewObject<UStaticMeshComponent>(this);
	M->SetStaticMesh(Mesh);
	M->RegisterComponent();
	M->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	M->SetRelativeLocationAndRotation(Loc, Rot);
	M->SetRelativeScale3D(Scale);
	M->SetCollisionProfileName(TEXT("BlockAll"));
	M->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(ShelfAssets::BaseMaterial(), this);
	MID->SetVectorParameterValue(TEXT("Color"), Color);
	M->SetMaterial(0, MID);
	return M;
}

void AShelfProp::Build(FName InKind, FIntPoint Size, float T)
{
	Kind = InKind;
	const FString K = InKind.ToString();
	const FLinearColor Stone = C(0.32f, 0.31f, 0.3f), Dark = C(0.035f, 0.025f, 0.02f), Wood = C(0.3f, 0.18f, 0.09f);
	if (K == TEXT("socket"))
	{
		Part(ShelfAssets::Cube(), FVector(0, 0, 45), FVector(0.5f, 0.5f, 0.9f), Stone);
		Part(ShelfAssets::Cylinder(), FVector(0, 0, 115), FVector(0.05f, 0.05f, 0.5f), Dark, FRotator(0, 0, 50));
		Part(ShelfAssets::Cube(), FVector(0, 18, 132), FVector(0.05f, 0.25f, 0.05f), Dark);
		Glow = NewObject<UPointLightComponent>(this);
		Glow->RegisterComponent();
		Glow->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		Glow->SetRelativeLocation(FVector(0, 0, 150));
		Glow->SetLightColor(C(1.f, 0.85f, 0.55f));
		Glow->SetIntensity(2500.f);
		Glow->SetAttenuationRadius(260.f);
	}
	else if (K == TEXT("chair") || K == TEXT("sixthchair"))
	{
		const FLinearColor Col = K == TEXT("sixthchair") ? Dark : Wood;
		Part(ShelfAssets::Cube(), FVector(0, 0, 45), FVector(0.45f, 0.45f, 0.07f), Col);
		Part(ShelfAssets::Cube(), FVector(-20, 0, 75), FVector(0.06f, 0.45f, 0.6f), Col);
		for (int32 a = -1; a <= 1; a += 2) for (int32 b = -1; b <= 1; b += 2) Part(ShelfAssets::Cylinder(), FVector(a * 18, b * 18, 22), FVector(0.05f, 0.05f, 0.44f), Col);
	}
	else if (K == TEXT("bench")) Part(ShelfAssets::Cube(), FVector(0, 0, 22), FVector(0.9f, 0.4f, 0.44f), Stone);
	else if (K == TEXT("table"))
	{
		Part(ShelfAssets::Cube(), FVector(0, 0, 72), FVector(1.0f, 0.65f, 0.08f), Dark);
		for (int32 s = -1; s <= 1; s += 2) Part(ShelfAssets::Cube(), FVector(s * 40, 0, 35), FVector(0.08f, 0.55f, 0.7f), Dark);
	}
	else if (K == TEXT("tent"))
	{
		Part(ShelfAssets::Cone(), FVector(0, 0, 75), FVector(1.8f, 1.8f, 1.5f), C(0.75f, 0.71f, 0.6f));
		Part(ShelfAssets::Cylinder(), FVector(0, 0, 90), FVector(0.04f, 0.04f, 1.8f), Wood);
	}
	else if (K == TEXT("desk"))
	{
		Part(ShelfAssets::Cube(), FVector(0, 0, 40), FVector(1.4f * Size.X, 0.6f, 0.8f), Wood);
		for (int32 i = 0; i < 4; ++i) Part(ShelfAssets::Cube(), FVector(-45 + i * 30, 0, 84), FVector(0.2f, 0.26f, 0.04f + i * 0.01f), C(0.85f, 0.8f, 0.68f));
	}
	else if (K == TEXT("marker")) Part(ShelfAssets::Cube(), FVector(0, 0, 55), FVector(0.4f, 0.25f, 1.1f), Stone);
	else if (K == TEXT("campfire"))
	{
		for (int32 i = 0; i < 5; ++i) Part(ShelfAssets::Cylinder(), FVector(FMath::Cos(i * 1.25f) * 14, FMath::Sin(i * 1.25f) * 14, 8), FVector(0.06f, 0.06f, 0.6f), Wood, FRotator(90, i * 72.f, 0));
		Part(ShelfAssets::Cone(), FVector(0, 0, 25), FVector(0.25f, 0.25f, 0.45f), C(1.f, 0.55f, 0.15f));
		UPointLightComponent* L = NewObject<UPointLightComponent>(this);
		L->RegisterComponent();
		L->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		L->SetRelativeLocation(FVector(0, 0, 80));
		L->SetLightColor(C(1.f, 0.6f, 0.25f));
		L->SetIntensity(9000.f);
		L->SetAttenuationRadius(900.f);
	}
	else if (K == TEXT("shelter"))
	{
		const float SX = Size.X * T * 0.85f / 100.f, SY = Size.Y * T * 0.8f / 100.f;
		Part(ShelfAssets::Cube(), FVector(0, 0, 130), FVector(SX, SY, 2.6f), C(0.36f, 0.34f, 0.3f));
		Part(ShelfAssets::Cube(), FVector(0, 0, 270), FVector(SX + 0.4f, SY + 0.4f, 0.25f), C(0.2f, 0.15f, 0.1f));
		Part(ShelfAssets::Cube(), FVector(0, SY * 50 + 2, 95), FVector(1.2f, 0.06f, 1.9f), Dark);
	}
	else if (K == TEXT("gate"))
	{
		for (int32 s = -1; s <= 1; s += 2)
		{
			const FRotator R(0, s * -55.f, 0);
			const FVector Hinge(s * T, 0, 0);
			const FVector Off = R.RotateVector(FVector(-s * 95.f, 0, 0));
			Part(ShelfAssets::Cube(), Hinge + Off + FVector(0, 0, 135), FVector(1.9f, 0.12f, 2.7f), Dark, R);
			Part(ShelfAssets::Cylinder(), Hinge + FVector(0, 0, 60), FVector(0.1f, 0.1f, 0.25f), C(0.55f, 0.4f, 0.18f));
			Part(ShelfAssets::Cylinder(), Hinge + FVector(0, 0, 210), FVector(0.1f, 0.1f, 0.25f), C(0.55f, 0.4f, 0.18f));
		}
	}
	else if (K == TEXT("marker_invisible")) { /* interaction point only */ }
}

// ===================================================================== world basics

AShelfWorld::AShelfWorld()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(Root);
	Sun->SetMobility(EComponentMobility::Movable);
	Sky = CreateDefaultSubobject<USkyLightComponent>(TEXT("Sky"));
	Sky->SetupAttachment(Root);
	Sky->SetMobility(EComponentMobility::Movable);
	Sky->bRealTimeCapture = true;
	Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
	Atmosphere->SetupAttachment(Root);
	Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
	Fog->SetupAttachment(Root);
}

AShelfWorld* AShelfWorld::Get(const UObject* WorldContext)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World) return nullptr;
	for (TActorIterator<AShelfWorld> It(World); It; ++It) return *It;
	return nullptr;
}

UShelfGameInstance* AShelfWorld::GI() const { return GetWorld() ? Cast<UShelfGameInstance>(GetWorld()->GetGameInstance()) : nullptr; }

void AShelfWorld::BeginPlay()
{
	Super::BeginPlay();
	Sun->SetAtmosphereSunLight(true);
	Story = NewObject<UShelfStory>(this);
	Story->Init(this);
	StartTitleBackdrop();
}

bool AShelfWorld::IsWalkable(FIntPoint P) const
{
	const TCHAR T = TileAt(P);
	if (T == TEXT('#') || T == TEXT('~') || T == TEXT('b') || T == TEXT('W') || T == TEXT('g')) return false;
	for (const auto& Pair : Entities)
	{
		const AShelfProp* Pr = Cast<AShelfProp>(Pair.Value.Get());
		if (!Pr || Pr->Kind == FName(TEXT("marker_invisible"))) continue;
		if (WorldToTile(Pr->GetActorLocation()) == P) return false;
	}
	return true;
}

TArray<FVector> AShelfWorld::FindPath(const FVector& From, const FVector& To) const
{
	TArray<FVector> Out;
	const FIntPoint S = WorldToTile(From);
	FIntPoint G = WorldToTile(To);
	if (!IsWalkable(G))
	{
		// nearest walkable neighbour of the goal
		float Best = TNumericLimits<float>::Max(); FIntPoint BestP = G; bool bFound = false;
		for (int32 dy = -2; dy <= 2; ++dy) for (int32 dx = -2; dx <= 2; ++dx)
		{
			const FIntPoint P(G.X + dx, G.Y + dy);
			if (!IsWalkable(P)) continue;
			const float D = FVector::Dist2D(TileToWorld(P), From);
			if (D < Best) { Best = D; BestP = P; bFound = true; }
		}
		if (!bFound) return Out;
		G = BestP;
	}
	if (S == G) { Out.Add(FVector(To.X, To.Y, From.Z)); return Out; }
	const int32 N = W * H;
	TArray<float> Cost; Cost.Init(TNumericLimits<float>::Max(), N);
	TArray<int32> Prev; Prev.Init(-1, N);
	TArray<bool> Closed; Closed.Init(false, N);
	auto Idx = [this](FIntPoint P) { return P.Y * W + P.X; };
	auto Heur = [G](FIntPoint P) { const int32 dx = FMath::Abs(P.X - G.X), dy = FMath::Abs(P.Y - G.Y); return float(FMath::Max(dx, dy)) + 0.414f * FMath::Min(dx, dy); };
	if (S.X < 0 || S.Y < 0 || S.X >= W || S.Y >= H) return Out;
	TArray<TPair<float, int32>> Open;
	Cost[Idx(S)] = 0; Open.Add(TPair<float, int32>(Heur(S), Idx(S)));
	static const int32 DX[8] = { 1, -1, 0, 0, 1, 1, -1, -1 }, DY[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };
	bool bReached = false;
	while (Open.Num())
	{
		int32 BestI = 0;
		for (int32 i = 1; i < Open.Num(); ++i) if (Open[i].Key < Open[BestI].Key) BestI = i;
		const int32 Cur = Open[BestI].Value; Open.RemoveAtSwap(BestI);
		if (Closed[Cur]) continue;
		Closed[Cur] = true;
		const FIntPoint P(Cur % W, Cur / W);
		if (P == G) { bReached = true; break; }
		for (int32 d = 0; d < 8; ++d)
		{
			const FIntPoint Q(P.X + DX[d], P.Y + DY[d]);
			if (!IsWalkable(Q)) continue;
			if (d >= 4 && (!IsWalkable(FIntPoint(P.X + DX[d], P.Y)) || !IsWalkable(FIntPoint(P.X, P.Y + DY[d])))) continue;
			const int32 QI = Idx(Q);
			const float NC = Cost[Cur] + (d >= 4 ? 1.414f : 1.f);
			if (NC < Cost[QI]) { Cost[QI] = NC; Prev[QI] = Cur; Open.Add(TPair<float, int32>(NC + Heur(Q), QI)); }
		}
	}
	if (!bReached) return Out;
	for (int32 I = Idx(G); I != -1 && I != Idx(S); I = Prev[I]) Out.Insert(TileToWorld(FIntPoint(I % W, I / W)) + FVector(0, 0, From.Z), 0);
	return Out;
}

// ===================================================================== characters

TArray<AShelfCharacter*> AShelfWorld::AllCharacters() const
{
	TArray<AShelfCharacter*> Out;
	for (AShelfCharacter* P : Party) if (P) Out.Add(P);
	for (const auto& Pair : Entities) if (AShelfCharacter* Ch = Cast<AShelfCharacter>(Pair.Value.Get())) Out.Add(Ch);
	for (AShelfCharacter* A : AmbientEnemies) if (A) Out.AddUnique(A);
	return Out;
}

AShelfCharacter* AShelfWorld::Leader() const { return Party.Num() ? Party[0].Get() : nullptr; }

TArray<AShelfCharacter*> AShelfWorld::PartyChars() const
{
	TArray<AShelfCharacter*> Out;
	for (AShelfCharacter* P : Party) if (P) Out.Add(P);
	return Out;
}

int32 AShelfWorld::PartyIndex(const AShelfCharacter* Ch) const
{
	for (int32 i = 0; i < Party.Num(); ++i) if (Party[i] == Ch) return i;
	return -1;
}

AShelfCharacter* AShelfWorld::FindChar(FName Id) const { return Cast<AShelfCharacter>(FindEntity(Id)); }
AActor* AShelfWorld::FindEntity(FName Id) const { const TObjectPtr<AActor>* A = Entities.Find(Id); return A ? A->Get() : nullptr; }

void AShelfWorld::SpawnPartyMember(const FShelfMember& M, const FVector& At)
{
	const bool bLead = Party.Num() == 0;
	FTransform Xf(FRotator::ZeroRotator, At + FVector(0, 0, 100));
	AShelfCharacter* Ch = GetWorld()->SpawnActorDeferred<AShelfCharacter>(AShelfCharacter::StaticClass(), Xf, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Ch) return;
	if (bLead) Ch->AutoPossessAI = EAutoPossessAI::Disabled;
	Ch->FinishSpawning(Xf);
	Ch->InitMember(M);
	Party.Add(Ch);
	if (bLead)
	{
		AttachCamera(Ch);
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController()) PC->Possess(Ch);
	}
}

void AShelfWorld::AttachCamera(AShelfCharacter* Ch)
{
	USpringArmComponent* Arm = NewObject<USpringArmComponent>(Ch, TEXT("CameraArm"));
	Arm->RegisterComponent();
	Arm->AttachToComponent(Ch->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	Arm->SetUsingAbsoluteRotation(true);
	Arm->SetWorldRotation(FRotator(-52.f, -60.f, 0.f));
	Arm->TargetArmLength = 1700.f;
	Arm->bDoCollisionTest = false;
	Arm->bEnableCameraLag = true;
	Arm->CameraLagSpeed = 8.f;
	UCameraComponent* Cam = NewObject<UCameraComponent>(Ch, TEXT("Camera"));
	Cam->RegisterComponent();
	Cam->AttachToComponent(Arm, FAttachmentTransformRules::KeepRelativeTransform, USpringArmComponent::SocketName);
	Cam->SetFieldOfView(55.f);
}

void AShelfWorld::SyncPartyToSave()
{
	UShelfGameInstance* G = GI();
	if (!G) return;
	for (AShelfCharacter* P : Party) if (P) if (FShelfMember* M = G->Member(P->MemberId)) M->HP = FMath::Max(1, P->HP);
	if (AShelfCharacter* L = Leader()) G->Tile = WorldToTile(L->GetActorLocation());
	G->Area = Area;
}

void AShelfWorld::RefreshPartyStats()
{
	UShelfGameInstance* G = GI();
	if (!G) return;
	for (AShelfCharacter* P : Party)
	{
		if (!P) continue;
		const FShelfMember* M = G->Member(P->MemberId);
		if (!M) continue;
		const FShelfStats S = ShelfData::Stats(*M);
		const int32 Gain = S.MaxHP - P->MaxHP;
		P->MaxHP = S.MaxHP; P->Atk = S.Atk; P->Def = S.Def; P->Spd = S.Spd;
		if (Gain > 0 && P->IsAlive()) P->HP += Gain;
		P->HP = FMath::Min(P->HP, P->MaxHP);
		P->Abilities.Reset();
		for (const FName& A : ShelfData::House(M->House).Abilities)
			if (const FAbilityDef* D = ShelfData::Ability(A)) if (M->Level >= D->Unlock) P->Abilities.Add(A);
	}
}

void AShelfWorld::OnCharacterDown(AShelfCharacter* Ch)
{
	if (Ch->Team == EShelfTeam::Enemy)
	{
		if (UShelfGameInstance* G = GI())
		{
			G->Salt += Ch->SaltValue;
			Float(Ch->GetActorLocation() + FVector(0, 0, 80), FString::Printf(TEXT("+%d XP  +%d salt"), Ch->XpValue, Ch->SaltValue), FColor(243, 220, 160));
			SyncPartyToSave();
			for (const FString& Msg : G->GainXP(Ch->XpValue)) Toast(Msg, 4.f);
			RefreshPartyStats();
		}
	}
	else if (Ch->Team == EShelfTeam::Party)
	{
		Toast(FString::Printf(TEXT("%s is down."), *Ch->DisplayName), 2.5f);
	}
}

// ===================================================================== story hooks

bool AShelfWorld::IsCombatPaused() const
{
	return FadeAlpha > 0.5f || (Story && Story->IsBlocking());
}

bool AShelfWorld::IsSleepless() const
{
	const UShelfGameInstance* G = GI();
	return G && Area == FName(TEXT("ring")) && G->Flag(TEXT("sleepless")) > 0;
}

void AShelfWorld::Interact(AActor* Target)
{
	if (!Target || !Story || Story->IsBusy()) return;
	FName Ev;
	if (AShelfProp* P = Cast<AShelfProp>(Target))
	{
		if (!P->DoneFlag.IsNone() && GI() && GI()->Flag(P->DoneFlag)) return;
		Ev = P->EventId;
	}
	else if (AShelfCharacter* Ch = Cast<AShelfCharacter>(Target))
	{
		Ev = Ch->EventId;
		if (AShelfCharacter* L = Leader()) Ch->FaceTowards(L->GetActorLocation());
	}
	if (!Ev.IsNone()) Story->RunScene(Ev);
}

void AShelfWorld::StartFight(const TArray<FName>& Ids)
{
	AShelfCharacter* L = Leader();
	for (const FName& Id : Ids)
		if (AShelfCharacter* E = FindChar(Id)) { E->bHostile = true; E->Target = L; E->EventId = NAME_None; }
	Toast(TEXT("Click an enemy to attack. Keys 1-4 use your House powers."), 4.f);
}

bool AShelfWorld::FightOver(const TArray<FName>& Ids) const
{
	for (const FName& Id : Ids) if (const AShelfCharacter* E = FindChar(Id)) if (E->IsAlive()) return false;
	return true;
}

void AShelfWorld::HideEntity(FName Id)
{
	if (AActor* A = FindEntity(Id)) { A->Destroy(); Entities.Remove(Id); }
}

void AShelfWorld::SpawnWalker(bool bSpawn)
{
	if (!bSpawn) { if (Walker) Walker->Destroy(); Walker = nullptr; return; }
	if (Walker || !Leader()) return;
	const FVector At = Leader()->GetActorLocation() + FVector(1400.f, 0, 0);
	Walker = GetWorld()->SpawnActor<AShelfCharacter>(AShelfCharacter::StaticClass(), At, FRotator::ZeroRotator);
	if (Walker) { Walker->InitNPC(EShelfHouse::Thessal, TEXT("A figure"), C(0.5f, 0.48f, 0.42f), C(0.35f, 0.33f, 0.28f)); Walker->SetActorEnableCollision(false); }
}

void AShelfWorld::Float(const FVector& Loc, const FString& Text, FColor Color)
{
	if (Floats.Num() > 60) Floats.RemoveAt(0);
	FFloatText F; F.Loc = Loc + FVector(FMath::FRandRange(-20.f, 20.f), FMath::FRandRange(-20.f, 20.f), 200.f); F.Text = Text; F.Color = Color;
	Floats.Add(F);
}

void AShelfWorld::Toast(const FString& Text, float Life)
{
	for (const FToast& T : Toasts) if (T.Text == Text) return;
	if (Toasts.Num() >= 4) Toasts.RemoveAt(0);
	FToast T; T.Text = Text; T.Life = Life;
	Toasts.Add(T);
}

FString AShelfWorld::AreaTitle() const
{
	if (Area == FName(TEXT("waystation"))) return TEXT("Second Road waystation");
	if (Area == FName(TEXT("ring"))) return IsSleepless() ? TEXT("Concord Ring - dark. Nobody sleeps here.") : FString::Printf(TEXT("Concord Ring - dimming %d%%. Leave before dark."), FMath::RoundToInt(Clock * 100));
	if (Area == FName(TEXT("road"))) return FString::Printf(TEXT("The Sixth Road - day %d"), Day);
	if (Area == FName(TEXT("gate"))) return TEXT("The Sixth Gate - day 18");
	return FString();
}

// ===================================================================== flow

void AShelfWorld::StartTitleBackdrop()
{
	for (AShelfCharacter* P : Party) if (P) P->Destroy();
	Party.Reset();
	ClearArea();
	BuildArea(TEXT("ring"));
	if (!TitleCam) TitleCam = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->UnPossess();
		PC->SetViewTarget(TitleCam);
	}
	Area = NAME_None;
}

void AShelfWorld::BeginNewGame()
{
	if (Story) Story->RunScene(TEXT("intro"));
}

void AShelfWorld::ContinueGame()
{
	UShelfGameInstance* G = GI();
	if (!G || !G->LoadGame()) return;
	ChangeArea(G->Area.IsNone() ? FName(TEXT("waystation")) : G->Area, G->Tile);
}

void AShelfWorld::ReturnToTitle()
{
	if (UShelfGameInstance* G = GI()) { SyncPartyToSave(); G->SaveGame(); G->bInGame = false; }
	StartTitleBackdrop();
}

void AShelfWorld::ChangeArea(FName Id, FIntPoint Spawn)
{
	UShelfGameInstance* G = GI();
	if (Party.Num()) SyncPartyToSave();
	FadeAlpha = 1.f;
	ClearArea();
	BuildArea(Id);
	if (Spawn.X < 0 || !IsWalkable(Spawn)) Spawn = SpawnTile;
	PlaceParty(Spawn);
	if (G) { G->Area = Id; G->Tile = Spawn; G->SaveGame(); }
	if (Id != FName(TEXT("ring")) && G) G->SetFlag(TEXT("sleepless"), 0);
	const FName Enter(*(Id.ToString() + TEXT("_enter")));
	if (Story && Story->HasScene(Enter)) Story->RunScene(Enter);
}

void AShelfWorld::PlaceParty(FIntPoint Spawn)
{
	UShelfGameInstance* G = GI();
	if (!G) return;
	// spawn any members that do not have a body yet
	for (const FShelfMember& M : G->Party)
	{
		bool bHas = false;
		for (AShelfCharacter* P : Party) if (P && P->MemberId == M.Id) bHas = true;
		if (!bHas) SpawnPartyMember(M, TileToWorld(Spawn));
	}
	const FIntPoint Offsets[] = { FIntPoint(0, 0), FIntPoint(0, 1), FIntPoint(1, 1), FIntPoint(-1, 1), FIntPoint(1, 0), FIntPoint(-1, 0) };
	int32 Slot = 0;
	for (AShelfCharacter* P : Party)
	{
		if (!P) continue;
		FIntPoint T = Spawn;
		for (; Slot < 6; ++Slot) { const FIntPoint Q = Spawn + Offsets[Slot]; if (IsWalkable(Q)) { T = Q; ++Slot; break; } }
		P->SetActorLocation(TileToWorld(T) + FVector(0, 0, 100), false, nullptr, ETeleportType::TeleportPhysics);
		P->ClearPath();
		P->Target = nullptr;
		if (!P->IsAlive()) P->Revive(0.5f);
	}
	LastLeaderTile = Spawn;
}

void AShelfWorld::ClearArea()
{
	for (UActorComponent* Cmp : AreaComponents) if (Cmp) Cmp->DestroyComponent();
	AreaComponents.Reset();
	for (AActor* A : AreaActors) if (A) A->Destroy();
	AreaActors.Reset();
	for (auto& Pair : Entities) if (Pair.Value) Pair.Value->Destroy();
	Entities.Reset();
	EntityHideFlags.Reset();
	AmbientEnemies.Reset();
	Triggers.Reset();
	if (Walker) { Walker->Destroy(); Walker = nullptr; }
	Floats.Reset();
}

void AShelfWorld::BuildArea(FName Id)
{
	Area = Id;
	if (Id == FName(TEXT("waystation"))) DefineWaystation();
	else if (Id == FName(TEXT("road"))) DefineRoad();
	else if (Id == FName(TEXT("gate"))) DefineGate();
	else DefineRing();
}

// ===================================================================== tick

void AShelfWorld::Tick(float Dt)
{
	Super::Tick(Dt);
	FadeAlpha = FMath::Max(0.f, FadeAlpha - Dt * 1.6f);
	for (int32 i = Floats.Num() - 1; i >= 0; --i) { Floats[i].T += Dt; Floats[i].Loc.Z += Dt * 70.f; if (Floats[i].T > 1.4f) Floats.RemoveAt(i); }
	for (int32 i = Toasts.Num() - 1; i >= 0; --i) { Toasts[i].Life -= Dt; if (Toasts[i].Life <= 0) Toasts.RemoveAt(i); }
	if (Story) Story->Tick(Dt);

	UShelfGameInstance* G = GI();
	if (!G || !G->bInGame || Area.IsNone())
	{
		TitleT += Dt;
		if (TitleCam)
		{
			const FVector Center = TileToWorld(FIntPoint(W / 2, H / 2));
			const FVector Pos = Center + FVector(FMath::Cos(TitleT * 0.05f) * 2600.f, FMath::Sin(TitleT * 0.05f) * 2600.f, 1900.f);
			TitleCam->SetActorLocationAndRotation(Pos, (Center - Pos).Rotation());
		}
		return;
	}

	RefreshTimer -= Dt;
	if (RefreshTimer <= 0.f) { RefreshTimer = 0.5f; RefreshEntities(); }
	if (!Story || !Story->IsBusy()) CheckTiles();

	// party wipe
	bool bAnyUp = false;
	for (AShelfCharacter* P : Party) if (P && P->IsAlive()) bAnyUp = true;
	if (!bAnyUp && Party.Num())
	{
		WipeTimer += Dt;
		if (WipeTimer > 2.5f)
		{
			WipeTimer = 0.f;
			Toast(TEXT("The party has fallen. On the Shelf, even this is written down somewhere. Try again."), 5.f);
			for (AShelfCharacter* Ch : AllCharacters())
				if (Ch && Ch->Team == EShelfTeam::Enemy && Ch->bHostile) { Ch->Revive(1.f); Ch->SetActorLocation(Ch->SpawnLocation, false, nullptr, ETeleportType::TeleportPhysics); Ch->Target = nullptr; }
			for (AShelfCharacter* P : Party) if (P) { P->Revive(0.6f); }
			PlaceParty(SpawnTile);
		}
	}

	// area-specific clocks
	if (Area == FName(TEXT("ring")) && !IsCombatPaused())
	{
		Clock = FMath::Min(1.f, Clock + Dt / 480.f);
		if (Clock >= 0.75f && !G->Flag(TEXT("warned"))) { G->SetFlag(TEXT("warned")); Toast(TEXT("The light is going. Maintain, log, and depart before dark."), 5.f); }
		if (Clock >= 1.f && !G->Flag(TEXT("sleepless"))) { G->SetFlag(TEXT("sleepless")); Toast(TEXT("Dark has come. Nobody sleeps in the Ring. The party is Sleepless (-15% damage) until you leave."), 6.f); }
		Sun->SetIntensity(FMath::Lerp(6.f, 0.8f, FMath::Clamp((Clock - 0.4f) / 0.6f, 0.f, 1.f)));
	}
	if (Area == FName(TEXT("road")))
		if (AShelfCharacter* L = Leader()) Day = FMath::Clamp(1 + FMath::FloorToInt((63 - WorldToTile(L->GetActorLocation()).Y) / 63.f * 17.f), 1, 18);
	if (Walker && Leader())
	{
		const FVector L = Leader()->GetActorLocation();
		const FVector Goal(L.X + 1400.f, L.Y, Walker->GetActorLocation().Z);
		Walker->SetActorLocation(FMath::VInterpTo(Walker->GetActorLocation(), Goal, Dt, 2.f));
		Walker->SetActorRotation(FRotator(0, -90.f, 0));
	}
}

void AShelfWorld::CheckTiles()
{
	AShelfCharacter* L = Leader();
	UShelfGameInstance* G = GI();
	if (!L || !G || !L->IsAlive()) return;
	const FIntPoint T = WorldToTile(L->GetActorLocation());
	if (T == LastLeaderTile) return;
	LastLeaderTile = T;
	if (TileAt(T) == TEXT('x'))
	{
		FString Scene = TEXT("exit_") + Area.ToString();
		if (Area == FName(TEXT("ring"))) Scene += T.X <= 1 ? TEXT("_west") : TEXT("_south");
		L->ClearPath();
		Story->RunScene(FName(*Scene));
		return;
	}
	for (const FShelfTrigger& Tr : Triggers)
	{
		if (G->Flag(Tr.Flag)) continue;
		if (T.X >= Tr.Rect.Min.X && T.X <= Tr.Rect.Max.X && T.Y >= Tr.Rect.Min.Y && T.Y <= Tr.Rect.Max.Y)
		{
			G->SetFlag(Tr.Flag);
			L->ClearPath();
			Story->RunScene(Tr.Scene);
			return;
		}
	}
}

void AShelfWorld::RefreshEntities()
{
	UShelfGameInstance* G = GI();
	if (!G) return;
	TArray<FName> Remove;
	for (const auto& Pair : EntityHideFlags) if (G->Flag(Pair.Value)) Remove.Add(Pair.Key);
	for (const FName& Id : Remove) { EntityHideFlags.Remove(Id); HideEntity(Id); }
	for (const auto& Pair : Entities)
		if (AShelfProp* P = Cast<AShelfProp>(Pair.Value.Get()))
			if (P->Glow) P->Glow->SetVisibility(P->DoneFlag.IsNone() || !G->Flag(P->DoneFlag));
}

// ===================================================================== building

UInstancedStaticMeshComponent* AShelfWorld::MakeISM(UStaticMesh* Mesh, FLinearColor Color, bool bCollide)
{
	UInstancedStaticMeshComponent* I = NewObject<UInstancedStaticMeshComponent>(this);
	I->SetStaticMesh(Mesh);
	I->RegisterComponent();
	I->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
	I->SetCollisionProfileName(bCollide ? TEXT("BlockAll") : TEXT("NoCollision"));
	if (bCollide) I->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(ShelfAssets::BaseMaterial(), this);
	MID->SetVectorParameterValue(TEXT("Color"), Color);
	I->SetMaterial(0, MID);
	AreaComponents.Add(I);
	return I;
}

void AShelfWorld::BuildTerrain(FName Style)
{
	const FString S = Style.ToString();
	const float T = TileSize, U = TileSize / 100.f;
	// ground slab (collision for walking)
	FLinearColor Ground = S == TEXT("ring") ? C(0.2f, 0.195f, 0.19f) : S == TEXT("compound") ? C(0.62f, 0.6f, 0.53f) : C(0.16f, 0.14f, 0.085f);
	UInstancedStaticMeshComponent* G = MakeISM(ShelfAssets::Cube(), Ground, true);
	G->AddInstance(FTransform(FRotator::ZeroRotator, FVector((W - 1) * T * 0.5f, (H - 1) * T * 0.5f, -50.f), FVector((W + 12) * U, (H + 12) * U, 1.f)));

	TMap<TCHAR, UInstancedStaticMeshComponent*> Sets;
	FRng Rng(W * 131 + H * 7 + S.Len());
	for (int32 y = 0; y < H; ++y) for (int32 x = 0; x < W; ++x)
	{
		const TCHAR Ch = TileAt(FIntPoint(x, y));
		const FVector P = TileToWorld(FIntPoint(x, y));
		auto GetSet = [&](TCHAR Key, UStaticMesh* Mesh, FLinearColor Col, bool bCol) -> UInstancedStaticMeshComponent*
		{
			if (UInstancedStaticMeshComponent** F = Sets.Find(Key)) return *F;
			UInstancedStaticMeshComponent* I = MakeISM(Mesh, Col, bCol);
			Sets.Add(Key, I);
			return I;
		};
		if (Ch == TEXT('#'))
		{
			if (S == TEXT("ring"))
			{
				const bool bTall = ((x / 3) * 7 + (y / 3) * 13) % 5 < 2;
				const float Hh = bTall ? 5.f : 3.6f;
				GetSet(bTall ? TEXT('T') : TEXT('#'), ShelfAssets::Cube(), bTall ? C(0.4f, 0.39f, 0.37f) : C(0.44f, 0.43f, 0.41f), true)
					->AddInstance(FTransform(FRotator::ZeroRotator, P + FVector(0, 0, Hh * 50.f), FVector(U, U, Hh)));
				GetSet(TEXT('^'), ShelfAssets::Cube(), C(0.3f, 0.29f, 0.28f), false)->AddInstance(FTransform(FRotator::ZeroRotator, P + FVector(0, 0, Hh * 100.f + 5.f), FVector(U * 1.02f, U * 1.02f, 0.12f)));
			}
			else
			{
				GetSet(TEXT('#'), ShelfAssets::Cube(), C(0.78f, 0.76f, 0.7f), true)->AddInstance(FTransform(FRotator::ZeroRotator, P + FVector(0, 0, 120.f), FVector(U, U, 2.4f)));
				GetSet(TEXT('^'), ShelfAssets::Cube(), C(0.36f, 0.45f, 0.58f), false)->AddInstance(FTransform(FRotator::ZeroRotator, P + FVector(0, 0, 245.f), FVector(U * 1.04f, U * 1.04f, 0.12f)));
			}
		}
		else if (Ch == TEXT(','))
			GetSet(Ch, ShelfAssets::Cube(), S == TEXT("ring") ? C(0.3f, 0.29f, 0.27f) : C(0.7f, 0.67f, 0.58f), false)->AddInstance(FTransform(FRotator::ZeroRotator, P + FVector(0, 0, 2.f), FVector(U, U, 0.04f)));
		else if (Ch == TEXT('o'))
			GetSet(Ch, ShelfAssets::Cube(), C(0.36f, 0.35f, 0.33f), false)->AddInstance(FTransform(FRotator::ZeroRotator, P + FVector(0, 0, 3.f), FVector(U, U, 0.05f)));
		else if (Ch == TEXT('~'))
			GetSet(Ch, ShelfAssets::Cube(), C(0.08f, 0.16f, 0.2f), true)->AddInstance(FTransform(FRotator::ZeroRotator, P + FVector(0, 0, -3.f), FVector(U * 0.8f, U, 0.08f)));
		else if (Ch == TEXT('='))
			GetSet(Ch, ShelfAssets::Cube(), C(0.26f, 0.255f, 0.25f), false)->AddInstance(FTransform(FRotator::ZeroRotator, P + FVector(0, 0, 3.f), FVector(U, U, 0.06f)));
		else if (Ch == TEXT('-'))
			GetSet(Ch, ShelfAssets::Cube(), C(0.2f, 0.19f, 0.16f), false)->AddInstance(FTransform(FRotator::ZeroRotator, P + FVector(0, 0, 1.f), FVector(U * 0.9f, U * 0.9f, 0.02f)));
		else if (Ch == TEXT('W') || Ch == TEXT('g'))
		{
			if (Ch == TEXT('W')) GetSet(Ch, ShelfAssets::Cube(), C(0.42f, 0.41f, 0.39f), true)->AddInstance(FTransform(FRotator::ZeroRotator, P + FVector(0, 0, 165.f), FVector(U, U * 0.6f, 3.3f)));
		}
		else if (Ch == TEXT('b'))
		{
			const float Sc = 0.9f + Rng.Next() * 0.9f;
			if (Rng.Next() < 0.5f) GetSet(TEXT('b'), ShelfAssets::Sphere(), C(0.25f, 0.24f, 0.21f), true)->AddInstance(FTransform(FRotator(Rng.Next() * 40, Rng.Next() * 360, 0), P + FVector(0, 0, 25.f), FVector(Sc, Sc, Sc * 0.6f)));
			else GetSet(TEXT('B'), ShelfAssets::Sphere(), C(0.12f, 0.16f, 0.07f), true)->AddInstance(FTransform(FRotator::ZeroRotator, P + FVector(0, 0, 40.f), FVector(Sc, Sc, Sc * 0.8f)));
		}
		if (Ch == TEXT('s'))
			for (int32 k = 0; k < 3; ++k)
				GetSet(TEXT('s'), ShelfAssets::Cone(), C(0.24f, 0.22f, 0.1f), false)->AddInstance(FTransform(FRotator(Rng.Next() * 20 - 10, 0, Rng.Next() * 20 - 10), P + FVector(Rng.Next() * 180 - 90, Rng.Next() * 180 - 90, 12.f), FVector(0.12f, 0.12f, 0.3f + Rng.Next() * 0.3f)));
	}
	SetupLighting(Style);
}

void AShelfWorld::SetupLighting(FName Style)
{
	// "There is no night, only dimming": a grey-gold band of sky light with no visible source.
	Sun->SetWorldRotation(FRotator(-48.f, 35.f, 0.f));
	Sun->SetIntensity(6.f);
	Sun->SetLightColor(C(1.f, 0.88f, 0.68f));
	Sky->SetIntensity(1.2f);
	Fog->SetFogDensity(Style == FName(TEXT("ring")) ? 0.03f : 0.018f);
	Fog->SetFogInscatteringColor(C(0.55f, 0.48f, 0.34f));
	Fog->SetVolumetricFog(true);
}

AShelfProp* AShelfWorld::AddProp(FName Id, FName Kind, FIntPoint Tile, FIntPoint Size, FName Event, const FString& InLabel, FName DoneFlag, float Yaw)
{
	const FVector Center = TileToWorld(Tile) + FVector((Size.X - 1) * TileSize * 0.5f, (Size.Y - 1) * TileSize * 0.5f, 0);
	AShelfProp* P = GetWorld()->SpawnActor<AShelfProp>(AShelfProp::StaticClass(), Center, FRotator(0, Yaw, 0));
	if (!P) return nullptr;
	P->EntityId = Id; P->EventId = Event; P->Label = InLabel; P->DoneFlag = DoneFlag;
	P->Build(Kind, Size, TileSize);
	Entities.Add(Id, P);
	return P;
}

AShelfCharacter* AShelfWorld::AddNPC(FName Id, FIntPoint Tile, EShelfHouse House, const FString& Name, FName Event, const FString& InLabel, FLinearColor Cloak, FLinearColor Tunic, FName HideFlag, float Yaw)
{
	if (!HideFlag.IsNone() && GI() && GI()->Flag(HideFlag)) return nullptr;
	AShelfCharacter* Ch = GetWorld()->SpawnActor<AShelfCharacter>(AShelfCharacter::StaticClass(), TileToWorld(Tile) + FVector(0, 0, 100), FRotator(0, Yaw, 0));
	if (!Ch) return nullptr;
	Ch->InitNPC(House, Name, Cloak, Tunic);
	Ch->EntityId = Id; Ch->EventId = Event; Ch->Label = InLabel;
	Entities.Add(Id, Ch);
	if (!HideFlag.IsNone()) EntityHideFlags.Add(Id, HideFlag);
	return Ch;
}

AShelfCharacter* AShelfWorld::AddEnemy(FName Id, FName Key, FIntPoint Tile, FName HideFlag, float Yaw, bool bAmbient)
{
	if (!HideFlag.IsNone() && GI() && GI()->Flag(HideFlag)) return nullptr;
	AShelfCharacter* Ch = GetWorld()->SpawnActor<AShelfCharacter>(AShelfCharacter::StaticClass(), TileToWorld(Tile) + FVector(0, 0, 100), FRotator(0, Yaw, 0));
	if (!Ch) return nullptr;
	Ch->InitEnemy(Key);
	Ch->EntityId = Id;
	Ch->SpawnLocation = Ch->GetActorLocation();
	Entities.Add(Id, Ch);
	if (!HideFlag.IsNone()) EntityHideFlags.Add(Id, HideFlag);
	if (bAmbient) AmbientEnemies.Add(Ch);
	return Ch;
}

void AShelfWorld::AddTrigger(int32 X0, int32 Y0, int32 X1, int32 Y1, FName Flag, FName Scene)
{
	FShelfTrigger T; T.Rect = FIntRect(X0, Y0, X1, Y1); T.Flag = Flag; T.Scene = Scene;
	Triggers.Add(T);
}

// ===================================================================== areas (same layouts as the web prototype)

void AShelfWorld::DefineWaystation()
{
	W = 17; H = 13; Tiles.Init(TEXT('#'), W * H);
	Fill(1, 1, 15, 11, TEXT(','));
	Fill(7, 0, 9, 0, TEXT('x'));
	SpawnTile = FIntPoint(8, 10);
	BuildTerrain(TEXT("compound"));
	AddNPC(TEXT("aud"), FIntPoint(8, 4), EShelfHouse::Saldric, TEXT("Commissioner Aud Verrel"), TEXT("aud"), TEXT("Talk to Commissioner Verrel"), Hex(TEXT("2F3B55")), Hex(TEXT("E6DFCF")), NAME_None, 90.f);
	AddNPC(TEXT("pell"), FIntPoint(3, 7), EShelfHouse::Saldric, TEXT("Clerk Pell"), TEXT("pell"), TEXT("Talk to Clerk Pell"), Hex(TEXT("4A3A2A")), Hex(TEXT("CFC6B0")), NAME_None, 0.f);
	AddProp(TEXT("desk"), TEXT("desk"), FIntPoint(3, 6), FIntPoint(2, 1));
	AddProp(TEXT("archive"), TEXT("tent"), FIntPoint(2, 2), FIntPoint(1, 1), TEXT("archive"), TEXT("Read the archive notice"));
	AddProp(TEXT("tent2"), TEXT("tent"), FIntPoint(14, 2));
	AddProp(TEXT("tent3"), TEXT("tent"), FIntPoint(14, 9));
	AddProp(TEXT("desk2"), TEXT("desk"), FIntPoint(12, 5), FIntPoint(2, 1));
}

void AShelfWorld::DefineRing()
{
	W = 30; H = 24; Tiles.Init(TEXT('#'), W * H);
	Fill(13, 6, 16, 23, TEXT('.'));
	Fill(13, 8, 13, 22, TEXT('~')); Fill(16, 8, 16, 22, TEXT('~'));
	Circle(14.5f, 3.5f, 4.3f, TEXT('o'));
	Fill(3, 13, 26, 14, TEXT('.'));
	Fill(3, 6, 9, 10, TEXT(',')); Fill(6, 11, 6, 12, TEXT('.'));
	Fill(20, 6, 26, 10, TEXT(',')); Fill(23, 11, 23, 12, TEXT('.'));
	Fill(3, 17, 9, 21, TEXT(',')); Fill(6, 15, 6, 16, TEXT('.'));
	Fill(20, 17, 26, 21, TEXT(',')); Fill(23, 15, 23, 16, TEXT('.'));
	Fill(1, 3, 10, 4, TEXT('.')); Fill(0, 3, 0, 4, TEXT('x'));
	Fill(14, 23, 15, 23, TEXT('x'));
	SpawnTile = FIntPoint(14, 22);
	BuildTerrain(TEXT("ring"));
	if (Area != FName(TEXT("ring"))) return;

	for (int32 k = 0; k < 6; ++k)
	{
		const float A = k / 6.f * PI * 2.f + PI / 6.f;
		const FIntPoint T(FMath::RoundToInt(14.5f + FMath::Cos(A) * 2.2f), FMath::RoundToInt(3.5f + FMath::Sin(A) * 2.2f));
		const bool bSixth = k == 2;
		AddProp(FName(*FString::Printf(TEXT("chair%d"), k)), bSixth ? TEXT("sixthchair") : TEXT("chair"), T, FIntPoint(1, 1),
			bSixth ? FName(TEXT("chair")) : NAME_None, bSixth ? TEXT("Examine the sixth chair") : FString(), NAME_None, FMath::RadiansToDegrees(A) + 180.f);
	}
	AddProp(TEXT("s1"), TEXT("socket"), FIntPoint(4, 7), FIntPoint(1, 1), TEXT("socket1"), TEXT("Examine the object"), TEXT("s1"));
	AddProp(TEXT("s2"), TEXT("socket"), FIntPoint(25, 7), FIntPoint(1, 1), TEXT("socket2"), TEXT("Examine the object"), TEXT("s2"));
	AddProp(TEXT("s3"), TEXT("socket"), FIntPoint(4, 20), FIntPoint(1, 1), TEXT("socket3"), TEXT("Examine the object"), TEXT("s3"));
	AddProp(TEXT("chan"), TEXT("marker_invisible"), FIntPoint(13, 17), FIntPoint(1, 1), TEXT("channel"), TEXT("Examine the water channel"));
	AddProp(TEXT("b1"), TEXT("bench"), FIntPoint(7, 8)); AddProp(TEXT("b2"), TEXT("bench"), FIntPoint(22, 8));
	AddProp(TEXT("b3"), TEXT("table"), FIntPoint(7, 19)); AddProp(TEXT("b4"), TEXT("table"), FIntPoint(21, 7));
	AddNPC(TEXT("oda"), FIntPoint(25, 20), EShelfHouse::Ikorran, TEXT("Oda Thaw"), NAME_None, FString(), Hex(TEXT("2B3850")), Hex(TEXT("5B6E7C")), TEXT("odaJoined"), 180.f);
	AddEnemy(TEXT("r1"), TEXT("broker"), FIntPoint(22, 20), TEXT("odaFought"), 0.f);
	AddEnemy(TEXT("r2"), TEXT("cutpurse"), FIntPoint(23, 18), TEXT("odaFought"), 0.f);
	AddEnemy(TEXT("r3"), TEXT("anchor"), FIntPoint(21, 19), TEXT("odaFought"), 0.f);
	AddTrigger(20, 17, 26, 21, TEXT("odaFight"), TEXT("oda"));
}

void AShelfWorld::DefineRoad()
{
	W = 11; H = 64; Tiles.Init(TEXT('s'), W * H);
	FRng R(99);
	for (int32 y = 0; y < H; ++y) for (int32 x = 0; x < W; ++x) if ((x < 3 || x > 7) && R.Next() < 0.09f) SetTile(x, y, TEXT('b'));
	Fill(4, 0, 6, 63, TEXT('='));
	Fill(4, 0, 6, 0, TEXT('x'));
	SpawnTile = FIntPoint(5, 62);
	BuildTerrain(TEXT("road"));
	for (int32 d = 1; d <= 6; ++d)
		AddProp(FName(*FString::Printf(TEXT("mk%d"), d)), TEXT("marker"), FIntPoint(7, 63 - d * 9), FIntPoint(1, 1), TEXT("marker"), TEXT("Read the road marker"));
	AddEnemy(TEXT("h1"), TEXT("hound"), FIntPoint(5, 45), TEXT("houndsDone"), -90.f);
	AddEnemy(TEXT("h2"), TEXT("hound"), FIntPoint(4, 44), TEXT("houndsDone"), -90.f);
	AddEnemy(TEXT("h3"), TEXT("hound"), FIntPoint(6, 44), TEXT("houndsDone"), -90.f);
	AddProp(TEXT("fire"), TEXT("campfire"), FIntPoint(8, 26), FIntPoint(1, 1), TEXT("camp_rest"), TEXT("Rest at the campfire"));
	AddEnemy(TEXT("sw"), TEXT("swine"), FIntPoint(5, 12), TEXT("swineDone"), -90.f);
	AddEnemy(TEXT("mo"), TEXT("moths"), FIntPoint(4, 11), TEXT("swineDone"), -90.f);
	AddTrigger(0, 46, 10, 50, TEXT("hounds"), TEXT("hounds"));
	AddTrigger(0, 36, 10, 38, TEXT("walker"), TEXT("walker"));
	AddTrigger(0, 26, 10, 27, TEXT("camp"), TEXT("camp"));
	AddTrigger(0, 13, 10, 16, TEXT("swine"), TEXT("swine"));
}

void AShelfWorld::DefineGate()
{
	W = 20; H = 16; Tiles.Init(TEXT('s'), W * H);
	FRng R(5);
	for (int32 y = 0; y < H; ++y) for (int32 x = 0; x < W; ++x) if (R.Next() < 0.05f && (x < 7 || x > 12)) SetTile(x, y, TEXT('b'));
	Fill(9, 6, 10, 15, TEXT('='));
	Fill(2, 5, 17, 5, TEXT('W'));
	Fill(9, 5, 10, 5, TEXT('g'));
	Fill(9, 0, 10, 4, TEXT('-'));
	SpawnTile = FIntPoint(9, 15);
	BuildTerrain(TEXT("gate"));
	AddProp(TEXT("leaves"), TEXT("gate"), FIntPoint(9, 5), FIntPoint(2, 1), TEXT("gate"), TEXT("Examine the gate"));
	AddProp(TEXT("shelter"), TEXT("shelter"), FIntPoint(13, 8), FIntPoint(3, 3), TEXT("shelter"), TEXT("Search the shelter"));
	AddEnemy(TEXT("sellim"), TEXT("sellim"), FIntPoint(13, 11), TEXT("sellimGone"), 90.f);
	AddEnemy(TEXT("a1"), TEXT("anchor"), FIntPoint(12, 12), TEXT("sellimFought"), 90.f);
	AddEnemy(TEXT("a2"), TEXT("anchor"), FIntPoint(15, 12), TEXT("sellimFought"), 90.f);
	AddTrigger(0, 0, 19, 12, TEXT("sellimFight"), TEXT("sellim"));
}
