#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ShelfAIController.generated.h"

class AShelfCharacter;

/**
 * Brains for companions (follow the leader, assist, use House powers sensibly)
 * and enemies (aggro, chase, attack, use skills; Sellim charges a Shear).
 * Movement uses the area grid's A* paths rather than a navmesh.
 */
UCLASS()
class THESHELF_API AShelfAIController : public AAIController
{
	GENERATED_BODY()
public:
	AShelfAIController();
	virtual void Tick(float DeltaSeconds) override;

private:
	float RepathTimer = 0.f;
	float ThinkTimer = 0.f;
	float SkillTimer = 4.f;
	float ShearTimer = 12.f;

	void TickCompanion(AShelfCharacter* Me, float Dt);
	void TickEnemy(AShelfCharacter* Me, float Dt);
	void MoveTowards(AShelfCharacter* Me, const FVector& Goal, float StopDist);
	void CompanionPowers(AShelfCharacter* Me);
};
