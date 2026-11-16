#include "ShelfPlayerController.h"
#include "ShelfCharacter.h"
#include "ShelfWorld.h"
#include "ShelfStory.h"
#include "ShelfHUD.h"
#include "ShelfGameInstance.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "GameFramework/SpringArmComponent.h"
#include "EngineUtils.h"

AShelfPlayerController::AShelfPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	DefaultMouseCursor = EMouseCursor::Default;
}

void AShelfPlayerController::BeginPlay()
{
	Super::BeginPlay();
	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
	if (UEnhancedInputLocalPlayerSubsystem* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		if (Context) Sub->AddMappingContext(Context, 0);
}

UInputAction* AShelfPlayerController::MakeAction(UInputMappingContext* Ctx, FKey Key, bool bAxis)
{
	UInputAction* A = NewObject<UInputAction>(this);
	A->ValueType = bAxis ? EInputActionValueType::Axis1D : EInputActionValueType::Boolean;
	Ctx->MapKey(A, Key);
	return A;
}

void AShelfPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	Context = NewObject<UInputMappingContext>(this);
	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC) return;
	EIC->BindAction(MakeAction(Context, EKeys::LeftMouseButton), ETriggerEvent::Started, this, &AShelfPlayerController::OnClick);
	EIC->BindAction(MakeAction(Context, EKeys::MouseX, true), ETriggerEvent::Triggered, this, &AShelfPlayerController::OnLook);
	EIC->BindAction(MakeAction(Context, EKeys::MouseWheelAxis, true), ETriggerEvent::Triggered, this, &AShelfPlayerController::OnZoom);
	UInputAction* F = MakeAction(Context, EKeys::W); EIC->BindAction(F, ETriggerEvent::Triggered, this, &AShelfPlayerController::OnMoveF); EIC->BindAction(F, ETriggerEvent::Completed, this, &AShelfPlayerController::OnMoveF);
	UInputAction* Bk = MakeAction(Context, EKeys::S); EIC->BindAction(Bk, ETriggerEvent::Triggered, this, &AShelfPlayerController::OnMoveB); EIC->BindAction(Bk, ETriggerEvent::Completed, this, &AShelfPlayerController::OnMoveB);
	UInputAction* L = MakeAction(Context, EKeys::A); EIC->BindAction(L, ETriggerEvent::Triggered, this, &AShelfPlayerController::OnMoveL); EIC->BindAction(L, ETriggerEvent::Completed, this, &AShelfPlayerController::OnMoveL);
	UInputAction* R = MakeAction(Context, EKeys::D); EIC->BindAction(R, ETriggerEvent::Triggered, this, &AShelfPlayerController::OnMoveR); EIC->BindAction(R, ETriggerEvent::Completed, this, &AShelfPlayerController::OnMoveR);
	EIC->BindAction(MakeAction(Context, EKeys::One), ETriggerEvent::Started, this, &AShelfPlayerController::OnKey1);
	EIC->BindAction(MakeAction(Context, EKeys::Two), ETriggerEvent::Started, this, &AShelfPlayerController::OnKey2);
	EIC->BindAction(MakeAction(Context, EKeys::Three), ETriggerEvent::Started, this, &AShelfPlayerController::OnKey3);
	EIC->BindAction(MakeAction(Context, EKeys::Four), ETriggerEvent::Started, this, &AShelfPlayerController::OnKey4);
	EIC->BindAction(MakeAction(Context, EKeys::Five), ETriggerEvent::Started, this, &AShelfPlayerController::OnKey5);
	EIC->BindAction(MakeAction(Context, EKeys::Six), ETriggerEvent::Started, this, &AShelfPlayerController::OnKey6);
	EIC->BindAction(MakeAction(Context, EKeys::Seven), ETriggerEvent::Started, this, &AShelfPlayerController::OnKey7);
	EIC->BindAction(MakeAction(Context, EKeys::Tab), ETriggerEvent::Started, this, &AShelfPlayerController::OnTab);
	EIC->BindAction(MakeAction(Context, EKeys::E), ETriggerEvent::Started, this, &AShelfPlayerController::OnInteract);
	EIC->BindAction(MakeAction(Context, EKeys::SpaceBar), ETriggerEvent::Started, this, &AShelfPlayerController::OnInteract);
	EIC->BindAction(MakeAction(Context, EKeys::Enter), ETriggerEvent::Started, this, &AShelfPlayerController::OnInteract);
	EIC->BindAction(MakeAction(Context, EKeys::J), ETriggerEvent::Started, this, &AShelfPlayerController::OnJournal);
	EIC->BindAction(MakeAction(Context, EKeys::Escape), ETriggerEvent::Started, this, &AShelfPlayerController::OnEscape);
}

AShelfCharacter* AShelfPlayerController::LeaderChar() const { return Cast<AShelfCharacter>(GetPawn()); }

// ---------------------------------------------------------------- input handlers

void AShelfPlayerController::OnClick(const FInputActionValue&)
{
	float X, Y;
	if (!GetMousePosition(X, Y)) return;
	if (AShelfHUD* Hud = Cast<AShelfHUD>(GetHUD())) if (Hud->HandleClick(FVector2D(X, Y))) return;
	AShelfWorld* W = AShelfWorld::Get(this);
	if (!W || !W->Story) return;
	if (W->Story->IsBlocking()) { W->Story->Advance(); return; }
	if (bJournalOpen) { bJournalOpen = false; return; }
	HandleWorldClick();
}

void AShelfPlayerController::HandleWorldClick()
{
	AShelfCharacter* Me = LeaderChar();
	AShelfWorld* W = AShelfWorld::Get(this);
	if (!Me || !W || !Me->IsAlive()) return;
	FHitResult Hit;
	if (!GetHitResultUnderCursor(ECC_Visibility, false, Hit)) return;
	AActor* A = Hit.GetActor();
	if (AShelfCharacter* C = Cast<AShelfCharacter>(A))
	{
		if (C->Team == EShelfTeam::Enemy && C->IsAlive())
		{
			if (!C->bHostile && !W->IsAmbient(C)) { W->Toast(TEXT("Not yet."), 1.2f); return; }
			C->bHostile = true;
			Me->Target = C; bChase = true; PendingInteract = nullptr; ChaseRepath = 0.f;
			return;
		}
		if (!C->EventId.IsNone()) { PendingInteract = C; bChase = false; Me->SetPath(W->FindPath(Me->GetActorLocation(), C->GetActorLocation())); return; }
	}
	if (AShelfProp* P = Cast<AShelfProp>(A))
	{
		if (!P->EventId.IsNone()) { PendingInteract = P; bChase = false; Me->SetPath(W->FindPath(Me->GetActorLocation(), P->GetActorLocation())); return; }
	}
	PendingInteract = nullptr;
	bChase = false;
	Me->SetPath(W->FindPath(Me->GetActorLocation(), Hit.ImpactPoint));
}

void AShelfPlayerController::OnLook(const FInputActionValue& V)
{
	if (IsInputKeyDown(EKeys::RightMouseButton)) CamYaw += V.Get<float>() * 2.2f;
}

void AShelfPlayerController::OnZoom(const FInputActionValue& V)
{
	CamZoom = FMath::Clamp(CamZoom - V.Get<float>() * 160.f, 700.f, 3200.f);
}

void AShelfPlayerController::OnMoveF(const FInputActionValue& V) { MoveInput.X = V.Get<bool>() ? 1.f : (MoveInput.X > 0 ? 0.f : MoveInput.X); }
void AShelfPlayerController::OnMoveB(const FInputActionValue& V) { MoveInput.X = V.Get<bool>() ? -1.f : (MoveInput.X < 0 ? 0.f : MoveInput.X); }
void AShelfPlayerController::OnMoveR(const FInputActionValue& V) { MoveInput.Y = V.Get<bool>() ? 1.f : (MoveInput.Y > 0 ? 0.f : MoveInput.Y); }
void AShelfPlayerController::OnMoveL(const FInputActionValue& V) { MoveInput.Y = V.Get<bool>() ? -1.f : (MoveInput.Y < 0 ? 0.f : MoveInput.Y); }

void AShelfPlayerController::OnTab(const FInputActionValue&)
{
	AShelfCharacter* Me = LeaderChar();
	if (!Me) return;
	TArray<AShelfCharacter*> Foes = Me->Foes();
	Foes.Sort([Me](const AShelfCharacter& A, const AShelfCharacter& B) { return Me->Dist2D(&A) < Me->Dist2D(&B); });
	if (!Foes.Num()) return;
	int32 I = Foes.IndexOfByKey(Me->Target.Get());
	Me->Target = Foes[(I + 1) % Foes.Num()];
}

void AShelfPlayerController::OnInteract(const FInputActionValue&)
{
	AShelfWorld* W = AShelfWorld::Get(this);
	if (!W || !W->Story) return;
	if (W->Story->IsBlocking()) { if (W->Story->Wait != UShelfStory::EWait::Choice) W->Story->Advance(); return; }
	AShelfCharacter* Me = LeaderChar();
	if (!Me) return;
	AActor* Best = nullptr; float BestD = 330.f;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* A = *It;
		FName Ev;
		if (const AShelfProp* P = Cast<AShelfProp>(A)) Ev = P->EventId;
		else if (const AShelfCharacter* C = Cast<AShelfCharacter>(A)) Ev = C->EventId;
		if (Ev.IsNone()) continue;
		const float D = Me->Dist2D(A);
		if (D < BestD) { BestD = D; Best = A; }
	}
	if (Best) W->Interact(Best);
}

void AShelfPlayerController::UseAbilitySlot(int32 Slot)
{
	AShelfWorld* W = AShelfWorld::Get(this);
	AShelfCharacter* Me = LeaderChar();
	if (!Me || !W || (W->Story && W->Story->IsBlocking())) return;
	const FHouseDef& H = ShelfData::House(Me->House);
	if (!H.Abilities.IsValidIndex(Slot)) return;
	if (!Me->Target.IsValid())
		if (const FAbilityDef* D = ShelfData::Ability(H.Abilities[Slot])) if (D->Target == EAbilityTarget::Enemy) Me->Target = Me->NearestHostile(1200.f);
	Me->TryAbility(H.Abilities[Slot]);
}

void AShelfPlayerController::UseItem(FName Id)
{
	AShelfWorld* W = AShelfWorld::Get(this);
	UShelfGameInstance* G = W ? W->GI() : nullptr;
	AShelfCharacter* Me = LeaderChar();
	if (!G || !Me || G->ItemCount(Id) <= 0) { if (W) W->Toast(TEXT("None left."), 1.2f); return; }
	G->AddItem(Id, -1);
	if (Id == FName(TEXT("draught"))) { AShelfCharacter* T = Me->PickAllyLowest(false); T->Heal(FMath::RoundToInt(T->MaxHP * 0.5f)); }
	else if (Id == FName(TEXT("bead"))) { Me->Focus = FMath::Min(AShelfCharacter::MaxFocus, Me->Focus + 3.f); W->Float(Me->GetActorLocation(), TEXT("+3 Focus"), FColor(168, 212, 255)); }
	else if (Id == FName(TEXT("salt"))) { for (AShelfCharacter* P : W->PartyChars()) { P->St.Rot = 0; P->St.Bond = 0; } W->Float(Me->GetActorLocation(), TEXT("Cleansed"), FColor::White); }
}

// ---------------------------------------------------------------- tick

void AShelfPlayerController::PlayerTick(float Dt)
{
	Super::PlayerTick(Dt);
	UpdateCamera(Dt);
	AShelfWorld* W = AShelfWorld::Get(this);
	AShelfCharacter* Me = LeaderChar();
	HoverLabel.Reset();
	if (!W || !Me) return;

	FHitResult Hit;
	if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		if (const AShelfProp* P = Cast<AShelfProp>(Hit.GetActor())) HoverLabel = P->Label;
		else if (const AShelfCharacter* C = Cast<AShelfCharacter>(Hit.GetActor()))
			HoverLabel = C->Team == EShelfTeam::Enemy ? (C->IsAlive() ? FString::Printf(TEXT("Attack %s"), *C->DisplayName) : FString()) : C->Label;
	}
	if (W->Story && W->Story->IsBlocking()) { Me->ClearPath(); return; }

	if (!MoveInput.IsNearlyZero())
	{
		Me->ClearPath(); PendingInteract = nullptr; bChase = false;
		const FRotator Yaw(0.f, CamYaw, 0.f);
		const FVector Fwd = FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Right = FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y);
		Me->AddMovementInput((Fwd * MoveInput.X + Right * MoveInput.Y).GetSafeNormal(), 1.f);
	}
	if (PendingInteract.IsValid() && Me->Dist2D(PendingInteract.Get()) < 280.f)
	{
		Me->ClearPath();
		Me->FaceTowards(PendingInteract->GetActorLocation());
		W->Interact(PendingInteract.Get());
		PendingInteract = nullptr;
	}
	ChaseRepath -= Dt;
	if (bChase && Me->Target.IsValid() && Me->Target->IsAlive())
	{
		if (Me->Dist2D(Me->Target.Get()) > Me->AttackRange && ChaseRepath <= 0.f)
		{
			ChaseRepath = 0.4f;
			Me->SetPath(W->FindPath(Me->GetActorLocation(), Me->Target->GetActorLocation()));
		}
	}
	else bChase = false;
}

void AShelfPlayerController::UpdateCamera(float Dt)
{
	if (APawn* P = GetPawn())
		if (USpringArmComponent* Arm = P->FindComponentByClass<USpringArmComponent>())
		{
			Arm->SetWorldRotation(FRotator(-52.f, CamYaw, 0.f));
			Arm->TargetArmLength = FMath::FInterpTo(Arm->TargetArmLength, CamZoom, Dt, 8.f);
		}
}
