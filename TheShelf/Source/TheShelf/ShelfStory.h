#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "ShelfStory.generated.h"

class AShelfWorld;

/**
 * Runs the story script in Content/Data/chapter1.json.
 * Scenes are lists of steps (say, choice, fight, card, area, journal...). See the README for the format.
 */
UCLASS()
class THESHELF_API UShelfStory : public UObject
{
	GENERATED_BODY()
public:
	enum class EWait : uint8 { None, Line, Choice, Card, Fight };

	void Init(AShelfWorld* InWorld);
	bool HasScene(FName Id) const;
	void RunScene(FName Id);
	void Tick(float Dt);
	bool IsBusy() const { return Stack.Num() > 0 || Wait != EWait::None; }
	bool IsBlocking() const { return Wait == EWait::Line || Wait == EWait::Choice || Wait == EWait::Card; }

	// input
	void Advance();
	void Choose(int32 Index);

	// what the HUD shows
	EWait Wait = EWait::None;
	FString SpeakerName, Text, CardTitle, CardText;
	FLinearColor SpeakerColor = FLinearColor::Gray;
	bool bNarration = false;
	TArray<FString> Choices;
	FString Objective() const;

private:
	struct FFrame
	{
		TArray<TSharedPtr<FJsonValue>> Steps;
		int32 Index = 0;
		TSharedPtr<FJsonObject> LoopChoice; // re-show this choice when the frame finishes
	};
	TWeakObjectPtr<AShelfWorld> World;
	TSharedPtr<FJsonObject> Root;
	TArray<FFrame> Stack;
	TArray<FName> Queue;
	TSharedPtr<FJsonObject> CurrentChoice;
	TArray<FName> FightIds;
	bool bShop = false;
	float FightCheck = 0.f;

	void Process();
	bool RunStep(const TSharedPtr<FJsonObject>& S); // returns true if we must wait
	void ShowChoice(const TSharedPtr<FJsonObject>& ChoiceStep);
	void ShowShop();
	void FinishScene();
	bool Cond(const FString& Expr) const;
	FString Subst(const FString& In) const;
};
