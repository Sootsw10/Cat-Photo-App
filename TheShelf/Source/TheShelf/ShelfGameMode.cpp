#include "ShelfGameMode.h"
#include "ShelfPlayerController.h"
#include "ShelfHUD.h"
#include "ShelfWorld.h"
#include "Engine/World.h"

AShelfGameMode::AShelfGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AShelfPlayerController::StaticClass();
	HUDClass = AShelfHUD::StaticClass();
}

void AShelfGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (!AShelfWorld::Get(this))
		GetWorld()->SpawnActor<AShelfWorld>(AShelfWorld::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
}
