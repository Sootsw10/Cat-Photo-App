#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShelfTypes.h"
#include "ShelfCharacter.generated.h"

class UStaticMeshComponent;

/**
 * Every person and creature in the game: the player, companions, NPCs and enemies.
 * Visuals are grey-box shapes built from engine primitives; see README for swapping in skeletal meshes.
 */
UCLASS()
class THESHELF_API AShelfCharacter : public ACharacter
{
	GENERATED_BODY()
public:
	AShelfCharacter();
	virtual void Tick(float DeltaSeconds) override;

	// ---- identity ----
	FName EntityId;
	FName MemberId;      // party members
	FName EnemyKey;      // enemies
	FString DisplayName;
	EShelfTeam Team = EShelfTeam::Neutral;
	EShelfHouse House = EShelfHouse::None;
	bool bHostile = false;   // enemies only attack once hostile (story fights) or when aggroed
	bool bBoss = false;
	bool bCompanion = false;
	FName EventId;           // story scene run when the player interacts
	FString Label;           // "Talk to Nim" etc.
	int32 XpValue = 0, SaltValue = 0;
	FVector SpawnLocation = FVector::ZeroVector;

	// ---- stats ----
	int32 HP = 1, MaxHP = 1, Atk = 1, Def = 0, Spd = 1;
	float Focus = 2.f;
	static constexpr float MaxFocus = 6.f;
	float AttackRange = 220.f;
	TArray<FName> Abilities; // party: from House
	TArray<FName> Skills;    // enemies
	FShelfStatus St;
	TMap<FName, float> Cooldowns;
	float GCD = 0.f;
	float AttackTimer = 0.f;
	TWeakObjectPtr<AShelfCharacter> Target;
	TArray<FVector> Path;
	bool bDead = false;

	// ---- boss casting ----
	FName CastSkill;
	float CastTime = 0.f, CastTotal = 0.f;
	FString CastLabel;

	// ---- setup ----
	void InitMember(const FShelfMember& M);
	void InitEnemy(FName Key);
	void InitNPC(EShelfHouse InHouse, const FString& Name, FLinearColor Cloak, FLinearColor Tunic);
	void Revive(float Fraction);

	// ---- queries ----
	bool IsAlive() const { return !bDead; }
	bool IsHostileTo(const AShelfCharacter* Other) const;
	float Dist2D(const AActor* Other) const;
	bool IsCasting() const { return CastTotal > 0.f; }
	float AttackInterval() const;

	// ---- combat ----
	int32 CalcDamage(const AShelfCharacter* Src, float Mult, bool bPierce) const;
	int32 TakeHit(AShelfCharacter* Src, int32 Damage, bool bCanBeRedirected = true, FColor Color = FColor(255, 214, 194));
	void Heal(int32 Amount);
	void AddRot(int32 N);
	/** Returns an empty string if the ability can be used now, else the reason. */
	FString AbilityBlockReason(FName Id) const;
	bool TryAbility(FName Id);
	void DoEnemySkill(FName Skill, AShelfCharacter* T);
	void StartCast(FName Skill, float Duration, const FString& LabelText);
	AShelfCharacter* PickAllyLowest(bool bExcludeSelf) const;
	AShelfCharacter* NearestHostile(float MaxDist) const;
	TArray<AShelfCharacter*> Allies() const;
	TArray<AShelfCharacter*> Foes() const;

	// ---- movement ----
	void SetPath(const TArray<FVector>& InPath) { Path = InPath; }
	void ClearPath() { Path.Reset(); }
	void FaceTowards(const FVector& Loc);

	// ---- visuals ----
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> VisualRoot;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;
	UPROPERTY() TArray<TObjectPtr<USceneComponent>> Legs;
	UPROPERTY() TArray<TObjectPtr<USceneComponent>> Arms;
	UPROPERTY() TArray<TObjectPtr<USceneComponent>> Orbiters;
	FName Shape;
	float AnimT = 0.f;
	float FlashT = 0.f;

	UStaticMeshComponent* AddPart(UStaticMesh* Mesh, FVector Loc, FVector Scale, FLinearColor Color, FRotator Rot = FRotator::ZeroRotator, USceneComponent* Parent = nullptr);
	USceneComponent* AddPivot(FVector Loc, USceneComponent* Parent = nullptr);
	void BuildHumanoid(EShelfHouse InHouse, FLinearColor Cloak, FLinearColor Tunic, float Scale, bool bBlade, bool bHood);
	void BuildHound();
	void BuildSwine();
	void BuildMoths();
	void ClearVisuals();

private:
	void TickStatus(float Dt);
	void TickMovement(float Dt);
	void TickCombat(float Dt);
	void TickVisuals(float Dt);
	void AutoAttack();
	void ApplyAbility(FName Id, AShelfCharacter* T);
	void Die();
};
