#include "ShelfHUD.h"
#include "ShelfWorld.h"
#include "ShelfCharacter.h"
#include "ShelfStory.h"
#include "ShelfGameInstance.h"
#include "ShelfPlayerController.h"
#include "ShelfTypes.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"

namespace
{
	const FLinearColor Gold(0.95f, 0.8f, 0.5f), Ink(0.94f, 0.9f, 0.82f), Muted(0.66f, 0.62f, 0.55f), Bad(0.9f, 0.35f, 0.35f), Good(0.55f, 0.8f, 0.45f), Focus(0.56f, 0.71f, 0.86f);
	const TCHAR* Names[] = { TEXT("Ashe"), TEXT("Corin"), TEXT("Wren"), TEXT("Tamsel"), TEXT("Isolde"), TEXT("Harrow") };
}

UFont* AShelfHUD::Small() const { return GEngine->GetSmallFont(); }
UFont* AShelfHUD::Medium() const { return GEngine->GetMediumFont(); }
UFont* AShelfHUD::Large() const { return GEngine->GetLargeFont(); }

// ---------------------------------------------------------------- primitives

void AShelfHUD::Panel(float X, float Y, float Wd, float Ht, FLinearColor Fill, FLinearColor Edge)
{
	DrawRect(Fill, X, Y, Wd, Ht);
	DrawRect(Edge, X, Y, Wd, 2); DrawRect(Edge, X, Y + Ht - 2, Wd, 2);
	DrawRect(Edge, X, Y, 2, Ht); DrawRect(Edge, X + Wd - 2, Y, 2, Ht);
}

void AShelfHUD::Bar(float X, float Y, float Wd, float Ht, float Frac, FLinearColor Col)
{
	DrawRect(FLinearColor(0, 0, 0, 0.7f), X, Y, Wd, Ht);
	DrawRect(Col, X + 1, Y + 1, FMath::Max(0.f, (Wd - 2) * FMath::Clamp(Frac, 0.f, 1.f)), Ht - 2);
}

float AShelfHUD::TextW(const FString& S, UFont* Font, float Scale)
{
	float W = 0, H = 0;
	GetTextSize(S, W, H, Font, Scale * Ui);
	return W;
}

void AShelfHUD::Text(const FString& S, float X, float Y, FLinearColor Col, UFont* Font, float Scale, bool bCenter)
{
	if (bCenter) X -= TextW(S, Font, Scale) * 0.5f;
	DrawText(S, FLinearColor(0, 0, 0, 0.8f), X + 1.5f, Y + 1.5f, Font, Scale * Ui);
	DrawText(S, Col, X, Y, Font, Scale * Ui);
}

TArray<FString> AShelfHUD::Wrap(const FString& S, float MaxW, UFont* Font, float Scale)
{
	TArray<FString> Lines, Words;
	S.ParseIntoArray(Words, TEXT(" "));
	FString Cur;
	for (const FString& Wd : Words)
	{
		const FString Try = Cur.IsEmpty() ? Wd : Cur + TEXT(" ") + Wd;
		if (TextW(Try, Font, Scale) > MaxW && !Cur.IsEmpty()) { Lines.Add(Cur); Cur = Wd; }
		else Cur = Try;
	}
	if (!Cur.IsEmpty()) Lines.Add(Cur);
	return Lines;
}

bool AShelfHUD::Button(const FString& Label, float X, float Y, float Wd, float Ht, const FString& Id, bool bEnabled, UFont* Font)
{
	float MX = 0, MY = 0;
	const bool bHover = GetOwningPlayerController() && GetOwningPlayerController()->GetMousePosition(MX, MY) && MX >= X && MX <= X + Wd && MY >= Y && MY <= Y + Ht;
	Panel(X, Y, Wd, Ht, bEnabled ? (bHover ? FLinearColor(0.2f, 0.16f, 0.12f, 0.95f) : FLinearColor(0.1f, 0.085f, 0.11f, 0.92f)) : FLinearColor(0.06f, 0.06f, 0.06f, 0.7f),
		bHover && bEnabled ? Gold : FLinearColor(0.45f, 0.38f, 0.3f, 0.9f));
	UFont* F = Font ? Font : Medium();
	float TW = 0, TH = 0; GetTextSize(Label, TW, TH, F, Ui);
	Text(Label, X + Wd * 0.5f, Y + (Ht - TH) * 0.5f, bEnabled ? Ink : Muted, F, 1.f, true);
	if (bEnabled) { FButton B; B.Pos = FVector2D(X, Y); B.Size = FVector2D(Wd, Ht); B.Id = Id; Buttons.Add(B); }
	return bHover;
}

FString AShelfHUD::StatusLine(const AShelfCharacter* C) const
{
	TArray<FString> P;
	const FShelfStatus& S = C->St;
	if (S.Rot) P.Add(FString::Printf(TEXT("Rot %d"), S.Rot));
	if (S.Bond) P.Add(FString::Printf(TEXT("Bond %d"), S.Bond));
	if (S.Load) P.Add(FString::Printf(TEXT("Load %d"), S.Load));
	if (S.Regard) P.Add(FString::Printf(TEXT("Regard %d"), S.Regard));
	if (S.Heavy > 0) P.Add(TEXT("Heavy"));
	if (S.Light > 0) P.Add(TEXT("Light"));
	if (S.Set > 0) P.Add(TEXT("Set"));
	if (S.Stare > 0) P.Add(TEXT("Staring"));
	if (S.Stay > 0) P.Add(TEXT("Stayed"));
	if (S.Unreg > 0) P.Add(TEXT("Unregarded"));
	if (S.Guard > 0) P.Add(TEXT("Shouldered"));
	if (S.Green > 0) P.Add(TEXT("Long Green"));
	if (S.bEvade) P.Add(TEXT("Evading"));
	return FString::Join(P, TEXT(" · "));
}

// ---------------------------------------------------------------- input

bool AShelfHUD::HandleClick(FVector2D Pos)
{
	AShelfWorld* W = AShelfWorld::Get(this);
	UShelfGameInstance* G = W ? W->GI() : nullptr;
	AShelfPlayerController* PC = Cast<AShelfPlayerController>(GetOwningPlayerController());
	for (int32 bi = Buttons.Num() - 1; bi >= 0; --bi)
	{
		const FButton& B = Buttons[bi];
		if (Pos.X < B.Pos.X || Pos.Y < B.Pos.Y || Pos.X > B.Pos.X + B.Size.X || Pos.Y > B.Pos.Y + B.Size.Y) continue;
		const FString& Id = B.Id;
		if (Id == TEXT("new")) bPicking = true;
		else if (Id == TEXT("back")) bPicking = false;
		else if (Id == TEXT("name")) NameIndex = (NameIndex + 1) % int32(UE_ARRAY_COUNT(Names));
		else if (Id == TEXT("cont") && W) W->ContinueGame();
		else if (Id.StartsWith(TEXT("house:")) && G && W)
		{
			G->NewGame(ShelfData::HouseFromKey(Id.RightChop(6)), Names[NameIndex]);
			bPicking = false;
			W->BeginNewGame();
		}
		else if (Id.StartsWith(TEXT("choice:")) && W && W->Story) W->Story->Choose(FCString::Atoi(*Id.RightChop(7)));
		else if ((Id == TEXT("dialog") || Id == TEXT("card")) && W && W->Story) W->Story->Advance();
		else if (Id == TEXT("journal") && PC) PC->bJournalOpen = !PC->bJournalOpen;
		else if (Id.StartsWith(TEXT("ab:")) && PC) PC->UseAbilitySlot(FCString::Atoi(*Id.RightChop(3)));
		else if (Id.StartsWith(TEXT("item:")) && PC) PC->UseItem(FName(*Id.RightChop(5)));
		return true;
	}
	return false;
}

// ---------------------------------------------------------------- draw

void AShelfHUD::DrawHUD()
{
	Super::DrawHUD();
	Buttons.Reset();
	if (!Canvas) return;
	Ui = FMath::Clamp(Canvas->ClipY / 900.f, 0.8f, 2.f);
	AShelfWorld* W = AShelfWorld::Get(this);
	UShelfGameInstance* G = W ? W->GI() : nullptr;
	if (!W || !G) return;
	if (!G->bInGame) DrawTitle(W);
	else DrawGame(W);
	if (W->FadeAlpha > 0) DrawRect(FLinearColor(0, 0, 0, W->FadeAlpha), 0, 0, Canvas->ClipX, Canvas->ClipY);
}

void AShelfHUD::DrawTitle(AShelfWorld* W)
{
	const float CX = Canvas->ClipX * 0.5f, H = Canvas->ClipY;
	DrawRect(FLinearColor(0, 0, 0, 0.45f), 0, 0, Canvas->ClipX, H);
	UShelfGameInstance* G = W->GI();
	if (!bPicking)
	{
		Text(TEXT("THE SHELF"), CX, H * 0.25f, FLinearColor(0.96f, 0.9f, 0.77f), Large(), 3.2f, true);
		Text(TEXT("Chapter One  ·  The Quarterly"), CX, H * 0.25f + 110 * Ui, Gold, Large(), 1.f, true);
		Text(TEXT("Every nineteen years the world goes silent for a day, and afterwards something is missing."), CX, H * 0.25f + 160 * Ui, Ink, Medium(), 1.f, true);
		Text(TEXT("The next Tolling is four years away."), CX, H * 0.25f + 190 * Ui, Ink, Medium(), 1.f, true);
		const float BW = 240 * Ui, BH = 46 * Ui;
		Button(TEXT("New game"), CX - BW - 10, H * 0.62f, BW, BH, TEXT("new"));
		Button(TEXT("Continue"), CX + 10, H * 0.62f, BW, BH, TEXT("cont"), G && G->HasSave());
		Text(TEXT("Left click: walk / talk / attack   Right-drag: turn camera   Wheel: zoom   1-4: powers   5-7: items   Tab: next enemy   J: journal"), CX, H - 50 * Ui, Muted, Small(), 1.f, true);
		return;
	}
	Text(TEXT("Who were you?"), CX, 40 * Ui, FLinearColor(0.96f, 0.9f, 0.77f), Large(), 1.6f, true);
	Text(TEXT("Commissioner Verrel needs people with no House standing. You were born to one of the five Houses and cast out. Your House decides your powers."), CX, 100 * Ui, Muted, Small(), 1.f, true);
	const TArray<FHouseDef>& Hs = ShelfData::Houses();
	const float CardW = FMath::Min(330.f * Ui, (Canvas->ClipX - 80) / 3.f), CardH = 250 * Ui, Gap = 14;
	for (int32 i = 0; i < Hs.Num(); ++i)
	{
		const int32 Row = i / 3, Col = i % 3, InRow = Row == 0 ? 3 : Hs.Num() - 3;
		const float RowW = InRow * CardW + (InRow - 1) * Gap;
		const float X = CX - RowW * 0.5f + Col * (CardW + Gap), Y = 140 * Ui + Row * (CardH + Gap);
		const FHouseDef& D = Hs[i];
		Button(FString(), X, Y, CardW, CardH, TEXT("house:") + D.Key);
		DrawRect(D.Color, X + 12, Y + 14, 26 * Ui, 26 * Ui);
		Text(FString::Printf(TEXT("%s · %s"), *D.Name, *D.Title), X + 48 * Ui, Y + 12, Gold, Large(), 0.8f);
		Text(D.Motto, X + 14, Y + 50 * Ui, Ink, Small());
		float Ly = Y + 76 * Ui;
		for (const FString& L : Wrap(FString::Printf(TEXT("Exile: %s. %s"), *D.Exile, *D.Blurb), CardW - 28, Small())) { Text(L, X + 14, Ly, Muted, Small()); Ly += 18 * Ui; }
		Ly += 6;
		for (int32 a = 1; a <= 3 && a < D.Abilities.Num(); ++a)
			if (const FAbilityDef* A = ShelfData::Ability(D.Abilities[a])) { Text(FString::Printf(TEXT("%d  %s (%d Focus)"), a, *A->Name, A->Cost), X + 14, Ly, Ink, Small()); Ly += 18 * Ui; }
	}
	Button(FString::Printf(TEXT("Name: %s  (click to change)"), Names[NameIndex]), CX - 170 * Ui, H - 120 * Ui, 340 * Ui, 40 * Ui, TEXT("name"));
	Button(TEXT("Back"), CX - 60 * Ui, H - 70 * Ui, 120 * Ui, 36 * Ui, TEXT("back"));
}

void AShelfHUD::DrawGame(AShelfWorld* W)
{
	AShelfPlayerController* PC = Cast<AShelfPlayerController>(GetOwningPlayerController());
	DrawWorldOverlays(W);
	DrawPartyFrames(W);
	if (AShelfCharacter* L = W->Leader()) DrawTargetFrame(L->Target.Get());
	DrawActionBar(W);

	// area + objective (top right)
	const float R = Canvas->ClipX - 20;
	const FString Area = W->AreaTitle();
	Panel(R - 420 * Ui, 16, 420 * Ui, 34 * Ui);
	Text(Area, R - 410 * Ui, 22, Gold, Medium());
	const FString Obj = W->Story ? W->Story->Objective() : FString();
	if (!Obj.IsEmpty())
	{
		TArray<FString> Lines = Wrap(Obj, 400 * Ui, Small());
		Panel(R - 420 * Ui, 56 * Ui, 420 * Ui, (26 + Lines.Num() * 18) * Ui, FLinearColor(0.06f, 0.05f, 0.07f, 0.7f));
		Text(TEXT("OBJECTIVE"), R - 410 * Ui, 60 * Ui, Gold, Small());
		for (int32 i = 0; i < Lines.Num(); ++i) Text(Lines[i], R - 410 * Ui, (78 + i * 18) * Ui, Ink, Small());
	}
	Button(TEXT("Journal (J)"), R - 140 * Ui, Canvas->ClipY - 56 * Ui, 130 * Ui, 36 * Ui, TEXT("journal"), true, Small());

	// toasts
	float TY = 150 * Ui;
	for (const FToast& T : W->Toasts)
	{
		const float TW = TextW(T.Text, Medium()) + 30;
		Panel(Canvas->ClipX * 0.5f - TW * 0.5f, TY, TW, 32 * Ui, FLinearColor(0.08f, 0.06f, 0.1f, FMath::Min(0.92f, T.Life)));
		Text(T.Text, Canvas->ClipX * 0.5f, TY + 6, Ink, Medium(), 1.f, true);
		TY += 38 * Ui;
	}
	// hover label
	if (PC && !PC->HoverLabel.IsEmpty() && !(W->Story && W->Story->IsBlocking()))
	{
		float MX, MY;
		if (PC->GetMousePosition(MX, MY)) Text(PC->HoverLabel, MX + 18, MY + 14, Gold, Medium());
	}
	DrawDialogue(W);
	if (PC && PC->bJournalOpen) DrawJournal(W);
}

void AShelfHUD::DrawWorldOverlays(AShelfWorld* W)
{
	AShelfCharacter* L = W->Leader();
	for (AShelfCharacter* C : W->AllCharacters())
	{
		if (!C || !C->IsAlive()) continue;
		const bool bEnemy = C->Team == EShelfTeam::Enemy;
		if (!bEnemy && C->Team != EShelfTeam::Party) continue;
		if (bEnemy && !C->bHostile && !W->IsAmbient(C)) continue;
		const FVector S = Project(C->GetActorLocation() + FVector(0, 0, 150));
		if (S.Z <= 0) continue;
		const float BW = 80 * Ui;
		Bar(S.X - BW * 0.5f, S.Y, BW, 7 * Ui, float(C->HP) / C->MaxHP, bEnemy ? Bad : Good);
		if (L && L->Target.Get() == C) { DrawRect(Gold, S.X - BW * 0.5f - 4, S.Y - 3, BW + 8, 2); DrawRect(Gold, S.X - BW * 0.5f - 4, S.Y + 8 * Ui, BW + 8, 2); }
		if (C->IsCasting()) Bar(S.X - BW * 0.5f, S.Y + 9 * Ui, BW, 6 * Ui, C->CastTime / C->CastTotal, FLinearColor(1.f, 0.6f, 0.2f));
	}
	for (const FFloatText& F : W->Floats)
	{
		const FVector S = Project(F.Loc);
		if (S.Z <= 0) continue;
		FLinearColor Col(F.Color); Col.A = FMath::Clamp(2.f - F.T * 1.6f, 0.f, 1.f);
		Text(F.Text, S.X, S.Y, Col, Large(), 0.75f, true);
	}
}

void AShelfHUD::DrawPartyFrames(AShelfWorld* W)
{
	float Y = 16;
	for (AShelfCharacter* C : W->PartyChars())
	{
		const float X = 16, Wd = 300 * Ui, Ht = 64 * Ui;
		Panel(X, Y, Wd, Ht);
		const FHouseDef& H = ShelfData::House(C->House);
		DrawRect(H.Color, X + 10, Y + 10, 20 * Ui, 20 * Ui);
		const FShelfMember* M = W->GI() ? W->GI()->Member(C->MemberId) : nullptr;
		Text(FString::Printf(TEXT("%s  ·  Lv %d %s"), *C->DisplayName, M ? M->Level : 1, *H.Title), X + 38 * Ui, Y + 8, C->IsAlive() ? Ink : Muted, Medium());
		Bar(X + 10, Y + 34 * Ui, Wd - 20, 10 * Ui, float(C->HP) / C->MaxHP, Good);
		Text(FString::Printf(TEXT("%d/%d"), C->HP, C->MaxHP), X + Wd - 80 * Ui, Y + 8, Muted, Small());
		const FString St = StatusLine(C);
		if (!St.IsEmpty()) Text(St, X + 10, Y + 46 * Ui, Gold, Small());
		Y += Ht + 6;
	}
}

void AShelfHUD::DrawTargetFrame(AShelfCharacter* T)
{
	if (!T || !T->IsAlive()) return;
	const float Wd = 360 * Ui, X = Canvas->ClipX * 0.5f - Wd * 0.5f, Y = 16;
	Panel(X, Y, Wd, 70 * Ui);
	Text(T->DisplayName, X + 12, Y + 8, T->bBoss ? Gold : Ink, Medium());
	Bar(X + 10, Y + 34 * Ui, Wd - 20, 12 * Ui, float(T->HP) / T->MaxHP, Bad);
	const FString St = T->IsCasting() ? T->CastLabel : StatusLine(T);
	if (!St.IsEmpty()) Text(St, X + 12, Y + 50 * Ui, T->IsCasting() ? FLinearColor(1.f, 0.6f, 0.2f) : Gold, Small());
}

void AShelfHUD::DrawActionBar(AShelfWorld* W)
{
	AShelfCharacter* L = W->Leader();
	UShelfGameInstance* G = W->GI();
	if (!L || !G) return;
	const FHouseDef& H = ShelfData::House(L->House);
	const float SW = 120 * Ui, SH = 62 * Ui, Gap = 6;
	const int32 Slots = 7;
	const float Total = Slots * SW + (Slots - 1) * Gap;
	float X = Canvas->ClipX * 0.5f - Total * 0.5f;
	const float Y = Canvas->ClipY - SH - 20 * Ui;
	// focus pips
	for (int32 i = 0; i < 6; ++i) DrawRect(i < FMath::FloorToInt(L->Focus) ? Focus : FLinearColor(0, 0, 0, 0.6f), X + i * 18 * Ui, Y - 18 * Ui, 14 * Ui, 10 * Ui);
	Text(FString::Printf(TEXT("Focus %d/6   Salt %d"), FMath::FloorToInt(L->Focus), G->Salt), X + 120 * Ui, Y - 24 * Ui, Muted, Small());
	for (int32 s = 1; s <= 4; ++s)
	{
		const FName Id = H.Abilities.IsValidIndex(s) ? H.Abilities[s] : NAME_None;
		const FAbilityDef* D = ShelfData::Ability(Id);
		const bool bLearned = L->Abilities.Contains(Id);
		const FString Why = L->AbilityBlockReason(Id);
		const bool bReady = bLearned && (Why.IsEmpty() || Why == TEXT("No target"));
		Button(D ? D->Name : FString(), X, Y, SW, SH, FString::Printf(TEXT("ab:%d"), s), bLearned, Small());
		Text(FString::Printf(TEXT("%d"), s), X + 6, Y + 4, Gold, Small());
		if (D && bLearned) Text(FString::Printf(TEXT("%d"), D->Cost), X + SW - 16 * Ui, Y + 4, Focus, Small());
		if (!bLearned && D) Text(FString::Printf(TEXT("Lv %d"), D->Unlock), X + SW * 0.5f, Y + SH - 20 * Ui, Muted, Small(), 1.f, true);
		if (const float* Cd = L->Cooldowns.Find(Id)) if (*Cd > 0 && D) DrawRect(FLinearColor(0, 0, 0, 0.55f), X, Y + SH * (1.f - *Cd / FMath::Max(0.1f, D->Cooldown)), SW, SH * (*Cd / FMath::Max(0.1f, D->Cooldown)));
		if (bLearned && !bReady) DrawRect(FLinearColor(0, 0, 0, 0.35f), X, Y, SW, SH);
		X += SW + Gap;
	}
	const TCHAR* Items[] = { TEXT("draught"), TEXT("bead"), TEXT("salt") };
	for (int32 i = 0; i < 3; ++i)
	{
		const FItemDef* It = ShelfData::Item(Items[i]);
		const int32 N = G->ItemCount(Items[i]);
		Button(FString::Printf(TEXT("%s x%d"), It ? *It->Name : TEXT(""), N), X, Y, SW, SH, FString::Printf(TEXT("item:%s"), Items[i]), N > 0, Small());
		Text(FString::Printf(TEXT("%d"), 5 + i), X + 6, Y + 4, Gold, Small());
		X += SW + Gap;
	}
}

void AShelfHUD::DrawDialogue(AShelfWorld* W)
{
	UShelfStory* S = W->Story;
	if (!S) return;
	if (S->Wait == UShelfStory::EWait::Card)
	{
		DrawRect(FLinearColor(0, 0, 0, 0.92f), 0, 0, Canvas->ClipX, Canvas->ClipY);
		const float CX = Canvas->ClipX * 0.5f;
		Text(S->CardTitle, CX, Canvas->ClipY * 0.36f, FLinearColor(0.96f, 0.86f, 0.62f), Large(), 1.8f, true);
		float Y = Canvas->ClipY * 0.36f + 80 * Ui;
		for (const FString& L : Wrap(S->CardText, FMath::Min(760.f * Ui, Canvas->ClipX - 60), Medium())) { Text(L, CX, Y, Ink, Medium(), 1.f, true); Y += 28 * Ui; }
		Text(TEXT("Click or press Space to continue"), CX, Canvas->ClipY - 70 * Ui, Muted, Small(), 1.f, true);
		FButton B; B.Pos = FVector2D(0, 0); B.Size = FVector2D(Canvas->ClipX, Canvas->ClipY); B.Id = TEXT("card"); Buttons.Add(B);
		return;
	}
	if (S->Wait != UShelfStory::EWait::Line && S->Wait != UShelfStory::EWait::Choice) return;
	const float Wd = FMath::Min(900.f * Ui, Canvas->ClipX - 40), X = Canvas->ClipX * 0.5f - Wd * 0.5f;
	const TArray<FString> Lines = Wrap(S->Text, Wd - 40, Medium());
	const float ChoiceH = 40 * Ui;
	const float Ht = (58 + Lines.Num() * 26) * Ui + (S->Wait == UShelfStory::EWait::Choice ? S->Choices.Num() * (ChoiceH + 6) + 8 : 0);
	const float Y = Canvas->ClipY - Ht - 110 * Ui;
	Panel(X, Y, Wd, Ht, FLinearColor(0.07f, 0.055f, 0.08f, 0.95f));
	if (!S->bNarration && !S->SpeakerName.IsEmpty())
	{
		DrawRect(S->SpeakerColor, X + 16, Y + 16, 18 * Ui, 18 * Ui);
		Text(S->SpeakerName, X + 44 * Ui, Y + 12, Gold, Large(), 0.8f);
	}
	float LY = Y + (S->bNarration ? 18 : 48) * Ui;
	for (const FString& L : Lines) { Text(L, X + 20, LY, S->bNarration ? FLinearColor(0.88f, 0.83f, 0.72f) : Ink, Medium()); LY += 26 * Ui; }
	if (S->Wait == UShelfStory::EWait::Line)
	{
		Text(TEXT("click / space"), X + Wd - 120 * Ui, Y + Ht - 26 * Ui, Muted, Small());
		FButton B; B.Pos = FVector2D(X, Y); B.Size = FVector2D(Wd, Ht); B.Id = TEXT("dialog"); Buttons.Add(B);
	}
	else
	{
		LY += 8;
		for (int32 i = 0; i < S->Choices.Num(); ++i)
		{
			Button(FString::Printf(TEXT("%d. %s"), i + 1, *S->Choices[i]), X + 20, LY, Wd - 40, ChoiceH, FString::Printf(TEXT("choice:%d"), i), true, Small());
			LY += ChoiceH + 6;
		}
	}
}

void AShelfHUD::DrawJournal(AShelfWorld* W)
{
	UShelfGameInstance* G = W->GI();
	const float Wd = FMath::Min(800.f * Ui, Canvas->ClipX - 40), Ht = Canvas->ClipY - 200 * Ui, X = Canvas->ClipX * 0.5f - Wd * 0.5f, Y = 100 * Ui;
	Panel(X, Y, Wd, Ht, FLinearColor(0.06f, 0.05f, 0.07f, 0.97f));
	Text(TEXT("Journal"), X + 20, Y + 12, Gold, Large(), 1.f);
	float LY = Y + 60 * Ui;
	for (int32 i = G->Journal.Num() - 1; i >= 0 && LY < Y + Ht - 40; --i)
	{
		Text(G->Journal[i].Title, X + 20, LY, Gold, Medium()); LY += 26 * Ui;
		for (const FString& L : Wrap(G->Journal[i].Body, Wd - 40, Small())) { Text(L, X + 20, LY, Ink, Small()); LY += 19 * Ui; }
		LY += 12 * Ui;
	}
	if (!G->Journal.Num()) Text(TEXT("Nothing yet."), X + 20, LY, Muted, Medium());
	Button(TEXT("Close"), X + Wd - 120 * Ui, Y + 12, 100 * Ui, 34 * Ui, TEXT("journal"), true, Small());
}
