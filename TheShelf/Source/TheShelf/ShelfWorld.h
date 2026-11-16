#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShelfTypes.h"
#include "ShelfWorld.generated.h"

class AShelfCharacter;
class UShelfStory;
class UShelfGameInstance;
class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UDirectionalLightComponent;
class USkyLightComponent;
class USkyAtmosphereComponent;
class UExponentialHeightFogComponent;
class UPointLightComponent;
class ACameraActor;

/** An interactable or decorative prop built from grey-box shapes. */
UCLASS()
class THESHELF_API AShelfProp : public AActor
{
	GENERATED_BODY()
public:
	AShelfProp();
	FName EntityId;
	FName Kind;
	FName EventId;
	FString Label;
	FName DoneFlag;   // once this story flag is set the prop stops glowing / being interactable
	UPROPERTY() TObjectPtr<UPointLightComponent> Glow;
	void Build(FName InKind, FIntPoint Size, float TileSize);
	UStaticMeshComponent* Part(UStaticMesh* Mesh, FVector Loc, FVector Scale, FLinearColor Color, FRotator Rot = FRotator::ZeroRotator);
};

struct FShelfTrigger
{
	FIntRect Rect;   // inclusive tile rect (Min..Max)
	FName Flag;
	FName Scene;
};

struct FFloatText
{
	FVector Loc;
	FString Text;
	FColor Color;
	float T = 0.f;
};

struct FToast
{
	FString Text;
	float Life = 3.5f;
};

/**
 * Owns the current area: tile grid, terrain, props, characters, triggers and lighting.
 * Also routes interactions into the story runner and feeds the HUD.
 */
UCLASS()
class THESHELF_API AShelfWorld : public AActor
{
	GENERATED_BODY()
public:
	AShelfWorld();
	static AShelfWorld* Get(const UObject* WorldContext);
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// ---- grid ----
	static constexpr float TileSize = 200.f;
	int32 W = 0, H = 0;
	TArray<TCHAR> Tiles;
	TCHAR TileAt(FIntPoint P) const { return (P.X < 0 || P.Y < 0 || P.X >= W || P.Y >= H) ? TEXT('#') : Tiles[P.Y * W + P.X]; }
	void SetTile(int32 X, int32 Y, TCHAR C) { if (X >= 0 && Y >= 0 && X < W && Y < H) Tiles[Y * W + X] = C; }
	void Fill(int32 X0, int32 Y0, int32 X1, int32 Y1, TCHAR C) { for (int32 y = Y0; y <= Y1; ++y) for (int32 x = X0; x <= X1; ++x) SetTile(x, y, C); }
	void Circle(float CX, float CY, float R, TCHAR C) { for (int32 y = 0; y < H; ++y) for (int32 x = 0; x < W; ++x) if (FMath::Sqrt(FMath::Square(x - CX) + FMath::Square(y - CY)) <= R) SetTile(x, y, C); }
	FVector TileToWorld(FIntPoint P) const { return FVector(P.X * TileSize, P.Y * TileSize, 0.f); }
	FIntPoint WorldToTile(const FVector& L) const { return FIntPoint(FMath::RoundToInt(L.X / TileSize), FMath::RoundToInt(L.Y / TileSize)); }
	bool IsWalkable(FIntPoint P) const;
	TArray<FVector> FindPath(const FVector& From, const FVector& To) const;

	// ---- area ----
	FName Area;
	FIntPoint SpawnTile;
	void ChangeArea(FName Id, FIntPoint Spawn);
	void StartTitleBackdrop();
	void BeginNewGame();
	void ContinueGame();
	void ReturnToTitle();

	// ---- characters ----
	TArray<AShelfCharacter*> AllCharacters() const;
	AShelfCharacter* Leader() const;
	TArray<AShelfCharacter*> PartyChars() const;
	int32 PartyIndex(const AShelfCharacter* C) const;
	AShelfCharacter* FindChar(FName Id) const;
	AActor* FindEntity(FName Id) const;
	bool IsAmbient(const AShelfCharacter* C) const { return AmbientEnemies.Contains(C); }
	void OnCharacterDown(AShelfCharacter* C);
	void SpawnPartyMember(const FShelfMember& M, const FVector& At);
	void SyncPartyToSave();
	void RefreshPartyStats();

	// ---- story hooks ----
	UPROPERTY() TObjectPtr<UShelfStory> Story;
	void Interact(AActor* Target);
	void StartFight(const TArray<FName>& Ids);
	bool FightOver(const TArray<FName>& Ids) const;
	void HideEntity(FName Id);
	void SpawnWalker(bool bSpawn);
	bool IsCombatPaused() const;
	bool IsSleepless() const;

	// ---- HUD feeds ----
	TArray<FFloatText> Floats;
	TArray<FToast> Toasts;
	float FadeAlpha = 0.f;
	float Clock = 0.f;     // Concord Ring dimming, 0..1
	int32 Day = 1;         // Sixth Road
	void Float(const FVector& Loc, const FString& Text, FColor Color);
	void Toast(const FString& Text, float Life = 3.5f);
	FString AreaTitle() const;

	UShelfGameInstance* GI() const;

private:
	UPROPERTY() TObjectPtr<USceneComponent> Root;
	UPROPERTY() TObjectPtr<UDirectionalLightComponent> Sun;
	UPROPERTY() TObjectPtr<USkyLightComponent> Sky;
	UPROPERTY() TObjectPtr<USkyAtmosphereComponent> Atmosphere;
	UPROPERTY() TObjectPtr<UExponentialHeightFogComponent> Fog;
	UPROPERTY() TArray<TObjectPtr<UActorComponent>> AreaComponents;
	UPROPERTY() TArray<TObjectPtr<AActor>> AreaActors;
	UPROPERTY() TArray<TObjectPtr<AShelfCharacter>> Party;
	UPROPERTY() TArray<TObjectPtr<AShelfCharacter>> AmbientEnemies;
	UPROPERTY() TObjectPtr<AShelfCharacter> Walker;
	UPROPERTY() TObjectPtr<ACameraActor> TitleCam;
	TMap<FName, TObjectPtr<AActor>> Entities;
	TMap<FName, FName> EntityHideFlags;
	TArray<FShelfTrigger> Triggers;
	FIntPoint LastLeaderTile = FIntPoint(-999, -999);
	float RefreshTimer = 0.f;
	float WipeTimer = 0.f;
	float TitleT = 0.f;

	void ClearArea();
	void BuildArea(FName Id);
	void BuildTerrain(FName Style);
	void SetupLighting(FName Style);
	void PlaceParty(FIntPoint Spawn);
	void AttachCamera(AShelfCharacter* C);
	UInstancedStaticMeshComponent* MakeISM(UStaticMesh* Mesh, FLinearColor Color, bool bCollide);
	void RefreshEntities();
	void CheckTiles();

	// area definitions
	void DefineWaystation();
	void DefineRing();
	void DefineRoad();
	void DefineGate();
	AShelfProp* AddProp(FName Id, FName Kind, FIntPoint Tile, FIntPoint Size = FIntPoint(1, 1), FName Event = NAME_None, const FString& InLabel = FString(), FName DoneFlag = NAME_None, float Yaw = 0.f);
	AShelfCharacter* AddNPC(FName Id, FIntPoint Tile, EShelfHouse House, const FString& Name, FName Event, const FString& InLabel, FLinearColor Cloak, FLinearColor Tunic, FName HideFlag = NAME_None, float Yaw = 0.f);
	AShelfCharacter* AddEnemy(FName Id, FName Key, FIntPoint Tile, FName HideFlag = NAME_None, float Yaw = 0.f, bool bAmbient = false);
	void AddTrigger(int32 X0, int32 Y0, int32 X1, int32 Y1, FName Flag, FName Scene);
};
