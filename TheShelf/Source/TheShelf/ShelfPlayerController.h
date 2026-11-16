#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "ShelfPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class AShelfCharacter;
class AShelfWorld;
class UShelfStory;

/**
 * RuneScape/WoW-style controls:
 *  Left click: walk, talk, examine, or target an enemy (auto-attack follows).
 *  Right-drag: rotate camera.  Wheel: zoom.  WASD: walk.
 *  1-4: House powers.  5-7: items.  Tab: next enemy.  E/Space: interact or continue dialogue.  J: journal.
 */
UCLASS()
class THESHELF_API AShelfPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	AShelfPlayerController();
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

	bool bJournalOpen = false;
	FString HoverLabel;
	float CamYaw = -60.f;
	float CamZoom = 1700.f;

	AShelfCharacter* LeaderChar() const;
	void UseAbilitySlot(int32 Slot);
	void UseItem(FName Id);

private:
	UPROPERTY() TObjectPtr<UInputMappingContext> Context;
	TWeakObjectPtr<AActor> PendingInteract;
	bool bChase = false;
	float ChaseRepath = 0.f;
	FVector2D MoveInput = FVector2D::ZeroVector;

	UInputAction* MakeAction(UInputMappingContext* Ctx, FKey Key, bool bAxis = false);

	void OnClick(const FInputActionValue& V);
	void OnLook(const FInputActionValue& V);
	void OnZoom(const FInputActionValue& V);
	void OnMoveF(const FInputActionValue& V);
	void OnMoveB(const FInputActionValue& V);
	void OnMoveL(const FInputActionValue& V);
	void OnMoveR(const FInputActionValue& V);
	void OnKey1(const FInputActionValue& V) { UseAbilitySlot(1); }
	void OnKey2(const FInputActionValue& V) { UseAbilitySlot(2); }
	void OnKey3(const FInputActionValue& V) { UseAbilitySlot(3); }
	void OnKey4(const FInputActionValue& V) { UseAbilitySlot(4); }
	void OnKey5(const FInputActionValue& V) { UseItem(TEXT("draught")); }
	void OnKey6(const FInputActionValue& V) { UseItem(TEXT("bead")); }
	void OnKey7(const FInputActionValue& V) { UseItem(TEXT("salt")); }
	void OnTab(const FInputActionValue& V);
	void OnInteract(const FInputActionValue& V);
	void OnJournal(const FInputActionValue& V) { bJournalOpen = !bJournalOpen; }
	void OnEscape(const FInputActionValue& V) { bJournalOpen = false; }
	void HandleWorldClick();
	void UpdateCamera(float Dt);
};
