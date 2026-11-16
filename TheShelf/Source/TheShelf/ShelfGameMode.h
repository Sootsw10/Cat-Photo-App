#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShelfGameMode.generated.h"

/** Boots the game: no default pawn; AShelfWorld spawns the party and builds areas in code. */
UCLASS()
class THESHELF_API AShelfGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	AShelfGameMode();
	virtual void BeginPlay() override;
	virtual void RestartPlayer(AController* NewPlayer) override {}
};
