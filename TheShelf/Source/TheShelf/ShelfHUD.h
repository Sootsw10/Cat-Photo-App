#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ShelfHUD.generated.h"

class UFont;
class AShelfWorld;
class AShelfCharacter;

/**
 * Canvas-drawn interface: title, House picker, party/target frames, action bar,
 * dialogue, cards, journal, floating combat text. Replace with UMG later if you like.
 */
UCLASS()
class THESHELF_API AShelfHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;
	/** Returns true if a HUD button consumed the click. */
	bool HandleClick(FVector2D Pos);

private:
	struct FButton { FVector2D Pos, Size; FString Id; };
	TArray<FButton> Buttons;
	bool bPicking = false;
	int32 NameIndex = 0;
	float Ui = 1.f; // UI scale

	void DrawTitle(AShelfWorld* W);
	void DrawGame(AShelfWorld* W);
	void DrawWorldOverlays(AShelfWorld* W);
	void DrawPartyFrames(AShelfWorld* W);
	void DrawTargetFrame(AShelfCharacter* T);
	void DrawActionBar(AShelfWorld* W);
	void DrawDialogue(AShelfWorld* W);
	void DrawJournal(AShelfWorld* W);

	void Panel(float X, float Y, float Wd, float Ht, FLinearColor Fill = FLinearColor(0.06f, 0.05f, 0.07f, 0.88f), FLinearColor Edge = FLinearColor(0.85f, 0.7f, 0.42f, 0.9f));
	void Bar(float X, float Y, float Wd, float Ht, float Frac, FLinearColor Col);
	void Text(const FString& S, float X, float Y, FLinearColor Col, UFont* Font, float Scale = 1.f, bool bCenter = false);
	float TextW(const FString& S, UFont* Font, float Scale = 1.f);
	TArray<FString> Wrap(const FString& S, float MaxW, UFont* Font, float Scale = 1.f);
	bool Button(const FString& Label, float X, float Y, float Wd, float Ht, const FString& Id, bool bEnabled = true, UFont* Font = nullptr);
	FString StatusLine(const AShelfCharacter* C) const;
	UFont* Small() const;
	UFont* Medium() const;
	UFont* Large() const;
};
