#include "ShelfGameInstance.h"
#include "Kismet/GameplayStatics.h"

void UShelfGameInstance::NewGame(EShelfHouse House, const FString& Name)
{
	PlayerName = Name.IsEmpty() ? TEXT("Ashe") : Name;
	PlayerHouse = House;
	Party.Reset();
	Flags.Reset();
	Items.Reset();
	Journal.Reset();
	Salt = 20;
	Items.Add(TEXT("draught"), 2);
	Items.Add(TEXT("bead"), 1);
	Area = TEXT("waystation");
	Tile = FIntPoint(-1, -1);

	FShelfMember You;
	You.Id = TEXT("you");
	You.Name = PlayerName;
	You.House = House;
	You.Level = 1;
	You.HP = ShelfData::Stats(You).MaxHP;
	Party.Add(You);
	bInGame = true;
}

bool UShelfGameInstance::HasSave() const
{
	return UGameplayStatics::DoesSaveGameExist(SlotName, 0);
}

bool UShelfGameInstance::LoadGame()
{
	UShelfSaveGame* S = Cast<UShelfSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	if (!S) return false;
	PlayerName = S->PlayerName; PlayerHouse = S->PlayerHouse; Party = S->Party; Flags = S->Flags; Items = S->Items;
	Salt = S->Salt; Journal = S->Journal; Area = S->Area; Tile = S->Tile;
	bInGame = true;
	return true;
}

void UShelfGameInstance::SaveGame()
{
	if (!bInGame) return;
	UShelfSaveGame* S = Cast<UShelfSaveGame>(UGameplayStatics::CreateSaveGameObject(UShelfSaveGame::StaticClass()));
	S->PlayerName = PlayerName; S->PlayerHouse = PlayerHouse; S->Party = Party; S->Flags = Flags; S->Items = Items;
	S->Salt = Salt; S->Journal = Journal; S->Area = Area; S->Tile = Tile;
	UGameplayStatics::SaveGameToSlot(S, SlotName, 0);
}

void UShelfGameInstance::AddJournal(const FString& Title, const FString& Body)
{
	for (const FJournalEntry& E : Journal) if (E.Title == Title) return;
	FJournalEntry E; E.Title = Title; E.Body = Body;
	Journal.Add(E);
}

FShelfMember* UShelfGameInstance::Member(FName Id)
{
	for (FShelfMember& M : Party) if (M.Id == Id) return &M;
	return nullptr;
}

void UShelfGameInstance::JoinParty(FName Id, const FString& Name, EShelfHouse House)
{
	if (Member(Id)) return;
	FShelfMember M;
	M.Id = Id; M.Name = Name; M.House = House;
	M.Level = Party.Num() ? Party[0].Level : 1;
	M.XP = ShelfData::XPForLevel(M.Level);
	M.HP = ShelfData::Stats(M).MaxHP;
	Party.Add(M);
}

TArray<FString> UShelfGameInstance::GainXP(int32 Amount)
{
	TArray<FString> Msgs;
	for (FShelfMember& M : Party)
	{
		M.XP += Amount;
		while (M.Level < ShelfData::MaxLevel() && M.XP >= ShelfData::XPForLevel(M.Level + 1))
		{
			const int32 Before = ShelfData::Stats(M).MaxHP;
			M.Level++;
			M.HP += ShelfData::Stats(M).MaxHP - Before;
			Msgs.Add(FString::Printf(TEXT("%s reached level %d."), *M.Name, M.Level));
			for (const FName& A : ShelfData::House(M.House).Abilities)
			{
				const FAbilityDef* D = ShelfData::Ability(A);
				if (D && D->Unlock == M.Level) Msgs.Add(FString::Printf(TEXT("%s learned %s."), *M.Name, *D->Name));
			}
		}
	}
	return Msgs;
}

void UShelfGameInstance::HealParty(float Fraction)
{
	for (FShelfMember& M : Party)
	{
		const int32 Max = ShelfData::Stats(M).MaxHP;
		M.HP = FMath::Clamp(FMath::Max(M.HP, FMath::CeilToInt(Max * Fraction)), 1, Max);
	}
}
