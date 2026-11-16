#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameFramework/SaveGame.h"
#include "ShelfTypes.h"
#include "ShelfGameInstance.generated.h"

USTRUCT()
struct FJournalEntry
{
	GENERATED_BODY()
	UPROPERTY(SaveGame) FString Title;
	UPROPERTY(SaveGame) FString Body;
};

UCLASS()
class UShelfSaveGame : public USaveGame
{
	GENERATED_BODY()
public:
	UPROPERTY(SaveGame) FString PlayerName;
	UPROPERTY(SaveGame) EShelfHouse PlayerHouse = EShelfHouse::Undrum;
	UPROPERTY(SaveGame) TArray<FShelfMember> Party;
	UPROPERTY(SaveGame) TMap<FName, int32> Flags;
	UPROPERTY(SaveGame) TMap<FName, int32> Items;
	UPROPERTY(SaveGame) int32 Salt = 0;
	UPROPERTY(SaveGame) TArray<FJournalEntry> Journal;
	UPROPERTY(SaveGame) FName Area;
	UPROPERTY(SaveGame) FIntPoint Tile = FIntPoint::ZeroValue;
};

/** Everything that persists between areas: party, story flags, inventory, journal. */
UCLASS()
class THESHELF_API UShelfGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	static constexpr const TCHAR* SlotName = TEXT("ShelfChapter1");

	UPROPERTY() bool bInGame = false;
	UPROPERTY() FString PlayerName = TEXT("Ashe");
	UPROPERTY() EShelfHouse PlayerHouse = EShelfHouse::Undrum;
	UPROPERTY() TArray<FShelfMember> Party;
	UPROPERTY() TMap<FName, int32> Flags;
	UPROPERTY() TMap<FName, int32> Items;
	UPROPERTY() int32 Salt = 0;
	UPROPERTY() TArray<FJournalEntry> Journal;
	UPROPERTY() FName Area;
	UPROPERTY() FIntPoint Tile = FIntPoint::ZeroValue;

	void NewGame(EShelfHouse House, const FString& Name);
	bool HasSave() const;
	bool LoadGame();
	void SaveGame();

	int32 Flag(FName Name) const { const int32* V = Flags.Find(Name); return V ? *V : 0; }
	void SetFlag(FName Name, int32 Value = 1) { Flags.Add(Name, Value); }
	int32 ItemCount(FName Id) const { const int32* V = Items.Find(Id); return V ? *V : 0; }
	void AddItem(FName Id, int32 N) { Items.Add(Id, FMath::Max(0, ItemCount(Id) + N)); }

	void AddJournal(const FString& Title, const FString& Body);
	FShelfMember* Member(FName Id);
	void JoinParty(FName Id, const FString& Name, EShelfHouse House);
	/** Splits XP across the whole party. Returns level-up messages. */
	TArray<FString> GainXP(int32 Amount);
	void HealParty(float Fraction = 1.f);
};
