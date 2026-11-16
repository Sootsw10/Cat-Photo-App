#include "ShelfStory.h"
#include "ShelfWorld.h"
#include "ShelfCharacter.h"
#include "ShelfGameInstance.h"
#include "ShelfTypes.h"
#include "TheShelf.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

void UShelfStory::Init(AShelfWorld* InWorld)
{
	World = InWorld;
	const FString Path = FPaths::ProjectContentDir() / TEXT("Data/chapter1.json");
	FString Json;
	if (!FFileHelper::LoadFileToString(Json, *Path))
	{
		UE_LOG(LogShelf, Error, TEXT("Story file missing: %s"), *Path);
		return;
	}
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
		UE_LOG(LogShelf, Error, TEXT("Story file failed to parse: %s"), *Path);
}

bool UShelfStory::HasScene(FName Id) const
{
	const TSharedPtr<FJsonObject>* Scenes;
	return Root.IsValid() && Root->TryGetObjectField(TEXT("scenes"), Scenes) && (*Scenes)->HasField(Id.ToString());
}

void UShelfStory::RunScene(FName Id)
{
	if (!HasScene(Id)) { UE_LOG(LogShelf, Warning, TEXT("No scene '%s'"), *Id.ToString()); return; }
	if (IsBusy()) { Queue.AddUnique(Id); return; }
	const TSharedPtr<FJsonObject>& Scenes = Root->GetObjectField(TEXT("scenes"));
	FFrame F;
	F.Steps = Scenes->GetArrayField(Id.ToString());
	Stack.Add(F);
	Process();
}

void UShelfStory::Tick(float Dt)
{
	if (Wait == EWait::Fight)
	{
		FightCheck -= Dt;
		if (FightCheck <= 0.f)
		{
			FightCheck = 0.4f;
			if (World.IsValid() && World->FightOver(FightIds)) { Wait = EWait::None; FightIds.Reset(); Process(); }
		}
	}
}

void UShelfStory::Advance()
{
	if (Wait == EWait::Line || Wait == EWait::Card) { Wait = EWait::None; Process(); }
}

void UShelfStory::Choose(int32 Index)
{
	if (Wait != EWait::Choice || !Choices.IsValidIndex(Index)) return;
	Wait = EWait::None;
	UShelfGameInstance* G = World.IsValid() ? World->GI() : nullptr;
	if (bShop)
	{
		const TArray<FItemDef>& Items = ShelfData::Items();
		if (Items.IsValidIndex(Index) && G)
		{
			const FItemDef& It = Items[Index];
			if (G->Salt >= It.Price) { G->Salt -= It.Price; G->AddItem(It.Id, 1); SpeakerName = TEXT("Clerk Pell"); Text = TEXT("Logged."); }
			else { SpeakerName = TEXT("Clerk Pell"); Text = TEXT("You can't afford that. I've noted that you asked."); }
			ShowShop();
			return;
		}
		bShop = false;
		Process();
		return;
	}
	const TArray<TSharedPtr<FJsonValue>>& Opts = CurrentChoice->GetArrayField(TEXT("choice"));
	TArray<TSharedPtr<FJsonObject>> Visible;
	for (const TSharedPtr<FJsonValue>& V : Opts)
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		FString If; if (O->TryGetStringField(TEXT("if"), If) && !Cond(If)) continue;
		Visible.Add(O);
	}
	if (!Visible.IsValidIndex(Index)) { Process(); return; }
	const TSharedPtr<FJsonObject> Opt = Visible[Index];
	FFrame F;
	const TArray<TSharedPtr<FJsonValue>>* Do;
	if (Opt->TryGetArrayField(TEXT("do"), Do)) F.Steps = *Do;
	bool bLoop = false, bLeave = false;
	CurrentChoice->TryGetBoolField(TEXT("loop"), bLoop);
	Opt->TryGetBoolField(TEXT("leave"), bLeave);
	if (bLoop && !bLeave) F.LoopChoice = CurrentChoice;
	Stack.Add(F);
	Process();
}

void UShelfStory::Process()
{
	while (Wait == EWait::None && Stack.Num())
	{
		FFrame& F = Stack.Last();
		if (F.Index >= F.Steps.Num())
		{
			TSharedPtr<FJsonObject> Loop = F.LoopChoice;
			Stack.Pop();
			if (Loop.IsValid()) { ShowChoice(Loop); return; }
			continue;
		}
		const TSharedPtr<FJsonObject> S = F.Steps[F.Index++]->AsObject();
		if (S.IsValid() && RunStep(S)) return;
	}
	if (Wait == EWait::None && Stack.Num() == 0) FinishScene();
}

void UShelfStory::FinishScene()
{
	if (World.IsValid())
	{
		World->SyncPartyToSave();
		if (UShelfGameInstance* G = World->GI()) G->SaveGame();
	}
	if (Queue.Num())
	{
		const FName Next = Queue[0];
		Queue.RemoveAt(0);
		RunScene(Next);
	}
}

void UShelfStory::ShowChoice(const TSharedPtr<FJsonObject>& Step)
{
	CurrentChoice = Step;
	Choices.Reset();
	for (const TSharedPtr<FJsonValue>& V : Step->GetArrayField(TEXT("choice")))
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		FString If; if (O->TryGetStringField(TEXT("if"), If) && !Cond(If)) continue;
		Choices.Add(Subst(O->GetStringField(TEXT("text"))));
	}
	Wait = EWait::Choice;
}

void UShelfStory::ShowShop()
{
	UShelfGameInstance* G = World.IsValid() ? World->GI() : nullptr;
	bShop = true;
	Choices.Reset();
	for (const FItemDef& It : ShelfData::Items())
		Choices.Add(FString::Printf(TEXT("%s - %d salt (you have %d)"), *It.Name, It.Price, G ? G->ItemCount(It.Id) : 0));
	Choices.Add(FString::Printf(TEXT("Done (salt: %d)"), G ? G->Salt : 0));
	Wait = EWait::Choice;
}

bool UShelfStory::Cond(const FString& Expr) const
{
	const UShelfGameInstance* G = World.IsValid() ? World->GI() : nullptr;
	if (!G) return false;
	FString E = Expr.TrimStartAndEnd();
	if (E.StartsWith(TEXT("!"))) return !Cond(E.RightChop(1));
	if (E.StartsWith(TEXT("house:"))) return ShelfData::House(G->PlayerHouse).Key == E.RightChop(6);
	if (E.StartsWith(TEXT("party:")))
	{
		const FString Key = E.RightChop(6);
		for (const FShelfMember& M : G->Party) if (ShelfData::House(M.House).Key == Key) return true;
		return false;
	}
	FString L, R;
	if (E.Split(TEXT(">="), &L, &R)) return G->Flag(FName(*L.TrimStartAndEnd())) >= FCString::Atoi(*R);
	return G->Flag(FName(*E)) > 0;
}

FString UShelfStory::Subst(const FString& In) const
{
	const UShelfGameInstance* G = World.IsValid() ? World->GI() : nullptr;
	if (!G) return In;
	const FHouseDef& H = ShelfData::House(G->PlayerHouse);
	FString Out = In;
	Out.ReplaceInline(TEXT("{name}"), *G->PlayerName);
	Out.ReplaceInline(TEXT("{house}"), *H.Name);
	Out.ReplaceInline(TEXT("{exile}"), *H.Exile);
	Out.ReplaceInline(TEXT("{origin}"), *H.Origin);
	Out.ReplaceInline(TEXT("{audline}"), *H.AudLine);
	Out.ReplaceInline(TEXT("{sockets}"), *FString::FromInt(G->Flag(TEXT("sockets"))));
	Out.ReplaceInline(TEXT("[[socket]]"), TEXT("________"));
	return Out;
}

bool UShelfStory::RunStep(const TSharedPtr<FJsonObject>& S)
{
	AShelfWorld* Wd = World.Get();
	UShelfGameInstance* G = Wd ? Wd->GI() : nullptr;
	if (!Wd || !G) return false;
	FString Str; double Num = 0; bool B = false;
	const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;

	if (S->TryGetStringField(TEXT("say"), Str))
	{
		bNarration = Str == TEXT("narr");
		SpeakerName.Reset(); SpeakerColor = FLinearColor(0.6f, 0.6f, 0.6f);
		if (Str == TEXT("you")) { SpeakerName = G->PlayerName; SpeakerColor = ShelfData::House(G->PlayerHouse).Color; }
		else if (!bNarration)
		{
			const TSharedPtr<FJsonObject>* Spk;
			if (Root->TryGetObjectField(TEXT("speakers"), Spk) && (*Spk)->HasField(Str))
			{
				const TSharedPtr<FJsonObject> O = (*Spk)->GetObjectField(Str);
				SpeakerName = O->GetStringField(TEXT("name"));
				FString HK; if (O->TryGetStringField(TEXT("house"), HK) && ShelfData::HouseFromKey(HK) != EShelfHouse::None) SpeakerColor = ShelfData::House(ShelfData::HouseFromKey(HK)).Color;
			}
		}
		Text = Subst(S->GetStringField(TEXT("text")));
		Wait = EWait::Line;
		return true;
	}
	if (S->HasField(TEXT("choice"))) { ShowChoice(S); return true; }
	if (S->TryGetStringField(TEXT("card"), Str))
	{
		CardTitle = Subst(Str);
		CardText.Reset(); S->TryGetStringField(TEXT("text"), CardText); CardText = Subst(CardText);
		Wait = EWait::Card;
		return true;
	}
	if (S->TryGetArrayField(TEXT("fight"), Arr))
	{
		FightIds.Reset();
		for (const TSharedPtr<FJsonValue>& V : *Arr) FightIds.Add(FName(*V->AsString()));
		Wd->StartFight(FightIds);
		Wait = EWait::Fight; FightCheck = 0.5f;
		return true;
	}
	if (S->TryGetStringField(TEXT("if"), Str))
	{
		const bool bOk = Cond(Str);
		const TArray<TSharedPtr<FJsonValue>>* Branch;
		if (S->TryGetArrayField(bOk ? TEXT("then") : TEXT("else"), Branch)) { FFrame F; F.Steps = *Branch; Stack.Add(F); }
		return false;
	}
	if (S->TryGetStringField(TEXT("set"), Str)) { double V = 1; S->TryGetNumberField(TEXT("value"), V); G->SetFlag(FName(*Str), int32(V)); return false; }
	if (S->TryGetStringField(TEXT("add"), Str)) { G->SetFlag(FName(*Str), G->Flag(FName(*Str)) + 1); return false; }
	if (S->TryGetNumberField(TEXT("salt"), Num)) { G->Salt += int32(Num); Wd->Toast(FString::Printf(TEXT("+%d witness-salt"), int32(Num)), 2.5f); return false; }
	if (S->TryGetStringField(TEXT("item"), Str)) { double N = 1; S->TryGetNumberField(TEXT("n"), N); G->AddItem(FName(*Str), int32(N)); return false; }
	if (S->TryGetStringField(TEXT("journal"), Str)) { G->AddJournal(Str, Subst(S->GetStringField(TEXT("text")))); Wd->Toast(TEXT("Journal: ") + Str, 3.f); return false; }
	if (S->TryGetStringField(TEXT("join"), Str))
	{
		const EShelfHouse H = ShelfData::HouseFromKey(S->GetStringField(TEXT("house")));
		const FString Name = S->GetStringField(TEXT("name"));
		G->JoinParty(FName(*Str), Name, H);
		if (const FShelfMember* M = G->Member(FName(*Str)))
			Wd->SpawnPartyMember(*M, Wd->Leader() ? Wd->Leader()->GetActorLocation() + FVector(-150, 100, 0) : FVector::ZeroVector);
		Wd->Toast(FString::Printf(TEXT("%s joined the party (%s %s)."), *Name, *ShelfData::House(H).Name, *ShelfData::House(H).Title), 4.f);
		return false;
	}
	if (S->TryGetStringField(TEXT("area"), Str))
	{
		FIntPoint Spawn(-1, -1);
		const TArray<TSharedPtr<FJsonValue>>* Sp;
		if (S->TryGetArrayField(TEXT("spawn"), Sp) && Sp->Num() == 2) Spawn = FIntPoint(int32((*Sp)[0]->AsNumber()), int32((*Sp)[1]->AsNumber()));
		Wd->ChangeArea(FName(*Str), Spawn);
		return false;
	}
	if (S->TryGetStringField(TEXT("run"), Str))
	{
		const TSharedPtr<FJsonObject>& Scenes = Root->GetObjectField(TEXT("scenes"));
		const TArray<TSharedPtr<FJsonValue>>* Steps;
		if (Scenes->TryGetArrayField(Str, Steps)) { FFrame F; F.Steps = *Steps; Stack.Add(F); }
		return false;
	}
	if (S->TryGetStringField(TEXT("hide"), Str)) { Wd->HideEntity(FName(*Str)); return false; }
	if (S->TryGetBoolField(TEXT("heal"), B) && B)
	{
		G->HealParty(1.f);
		for (AShelfCharacter* P : Wd->PartyChars()) { P->Revive(1.f); P->HP = P->MaxHP; }
		return false;
	}
	if (S->TryGetBoolField(TEXT("walker"), B)) { Wd->SpawnWalker(B); return false; }
	if (S->TryGetBoolField(TEXT("shop"), B) && B) { SpeakerName = TEXT("Clerk Pell"); Text = TEXT("Supplies for the road. Prices are fixed by the Second Road schedule."); ShowShop(); return true; }
	if (S->TryGetStringField(TEXT("toast"), Str)) { Wd->Toast(Subst(Str), 4.f); return false; }
	if (S->TryGetArrayField(TEXT("nudge"), Arr) && Arr->Num() == 2)
	{
		if (AShelfCharacter* L = Wd->Leader())
			L->SetActorLocation(Wd->TileToWorld(FIntPoint(int32((*Arr)[0]->AsNumber()), int32((*Arr)[1]->AsNumber()))) + FVector(0, 0, 100), false, nullptr, ETeleportType::TeleportPhysics);
		return false;
	}
	if (S->TryGetBoolField(TEXT("clock_reset"), B) && B) { Wd->Clock = 0.f; return false; }
	if (S->TryGetBoolField(TEXT("title"), B) && B) { Stack.Reset(); Queue.Reset(); Wd->ReturnToTitle(); return true; }
	UE_LOG(LogShelf, Warning, TEXT("Unknown story step"));
	return false;
}

FString UShelfStory::Objective() const
{
	const UShelfGameInstance* G = World.IsValid() ? World->GI() : nullptr;
	const TArray<TSharedPtr<FJsonValue>>* Objs;
	if (!G || !Root.IsValid() || !Root->TryGetArrayField(TEXT("objectives"), Objs)) return FString();
	for (const TSharedPtr<FJsonValue>& V : *Objs)
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		FString Area; if (O->TryGetStringField(TEXT("area"), Area) && FName(*Area) != World->Area) continue;
		bool bOk = true;
		const TArray<TSharedPtr<FJsonValue>>* Conds;
		if (O->TryGetArrayField(TEXT("if"), Conds)) for (const TSharedPtr<FJsonValue>& C : *Conds) if (!Cond(C->AsString())) bOk = false;
		if (bOk) return Subst(O->GetStringField(TEXT("text")));
	}
	return FString();
}
