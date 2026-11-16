#pragma once

#include "CoreMinimal.h"
#include "ShelfTypes.generated.h"

class AShelfCharacter;
class UStaticMesh;
class UMaterialInterface;

UENUM(BlueprintType)
enum class EShelfHouse : uint8
{
	Undrum, Saldric, Mirrowen, Ikorran, Thessal, None
};

UENUM(BlueprintType)
enum class EShelfTeam : uint8
{
	Party, Enemy, Neutral
};

enum class EAbilityTarget : uint8
{
	Enemy, Ally, Self, AllEnemies
};

/** One party member as the game remembers them between areas and in the save file. */
USTRUCT(BlueprintType)
struct FShelfMember
{
	GENERATED_BODY()

	UPROPERTY(SaveGame) FName Id;
	UPROPERTY(SaveGame) FString Name;
	UPROPERTY(SaveGame) EShelfHouse House = EShelfHouse::Undrum;
	UPROPERTY(SaveGame) int32 Level = 1;
	UPROPERTY(SaveGame) int32 XP = 0;
	UPROPERTY(SaveGame) int32 HP = 0;
};

struct FShelfStats
{
	int32 MaxHP = 1, Atk = 1, Def = 0, Spd = 1;
};

struct FHouseDef
{
	EShelfHouse House = EShelfHouse::None;
	FString Key, Name, Title, Motto, Exile, Origin, Blurb, AudLine;
	FLinearColor Color, Cloak, Tunic;
	int32 Hp = 0, GHp = 0, Atk = 0, GAtk = 0, Def = 0, GDef = 0, Spd = 0, GSpd = 0;
	float AttackRange = 200.f;
	/** Index 0 is the auto-attack; 1..4 go on hotkeys 1..4. */
	TArray<FName> Abilities;
};

struct FAbilityDef
{
	FName Id;
	FString Name;
	int32 Cost = 0;
	float Cooldown = 0.f;
	EAbilityTarget Target = EAbilityTarget::Enemy;
	int32 Unlock = 1;
	float Range = 900.f;
	FString Desc;
};

struct FEnemyDef
{
	FName Id;
	FString Name;
	EShelfHouse Kind = EShelfHouse::None;
	/** "humanoid", "hound", "swine", "moths" */
	FName Shape;
	int32 Hp = 1, Atk = 1, Def = 0, Spd = 1, Xp = 0, Salt = 0;
	bool bBoss = false;
	float Range = 200.f;
	TArray<FName> Skills;
};

struct FItemDef
{
	FName Id;
	FString Name, Desc;
	int32 Price = 0;
};

/** Live status effects on a character. Durations are in seconds. */
struct FShelfStatus
{
	int32 Rot = 0, Bond = 0, Load = 0, Regard = 0;
	float Heavy = 0, Light = 0, Set = 0, Stare = 0, Stay = 0, Unreg = 0, Green = 0, Guard = 0, Defend = 0;
	bool bWeak = false, bEvade = false;
	TWeakObjectPtr<AShelfCharacter> GuardBy;
	float DotTimer = 0;
	float DebtTimer = 0;
	int32 DebtAmount = 0;

	bool IsStunned() const { return Set > 0 || Stare > 0; }
};

namespace ShelfData
{
	const TArray<FHouseDef>& Houses();
	const FHouseDef& House(EShelfHouse H);
	EShelfHouse HouseFromKey(const FString& Key);
	const FAbilityDef* Ability(FName Id);
	const FEnemyDef* Enemy(FName Id);
	const TArray<FItemDef>& Items();
	const FItemDef* Item(FName Id);
	int32 XPForLevel(int32 Level); // XP needed to reach Level
	int32 MaxLevel();
	FShelfStats Stats(const FShelfMember& M);
}

/** Engine placeholder assets. Swap these for real art later (see README). */
namespace ShelfAssets
{
	UStaticMesh* Cube();
	UStaticMesh* Sphere();
	UStaticMesh* Cylinder();
	UStaticMesh* Cone();
	UStaticMesh* Plane();
	UMaterialInterface* BaseMaterial(); // has a "Color" vector parameter
}
