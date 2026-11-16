#include "ShelfTypes.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

namespace
{
	FLinearColor Hex(const TCHAR* H) { return FLinearColor(FColor::FromHex(H)); }

	TArray<FHouseDef> MakeHouses()
	{
		TArray<FHouseDef> Out;
		auto Add = [&Out](EShelfHouse E, const TCHAR* Key, const TCHAR* Name, const TCHAR* Title, const TCHAR* Motto, const TCHAR* Exile,
			const TCHAR* Origin, const TCHAR* Blurb, const TCHAR* AudLine, const TCHAR* Col, const TCHAR* Cloak, const TCHAR* Tunic,
			int32 Hp, int32 GHp, int32 Atk, int32 GAtk, int32 Def, int32 GDef, int32 Spd, int32 GSpd, float Range, TArray<FName> Ab)
		{
			FHouseDef D;
			D.House = E; D.Key = Key; D.Name = Name; D.Title = Title; D.Motto = Motto; D.Exile = Exile; D.Origin = Origin; D.Blurb = Blurb; D.AudLine = AudLine;
			D.Color = Hex(Col); D.Cloak = Hex(Cloak); D.Tunic = Hex(Tunic);
			D.Hp = Hp; D.GHp = GHp; D.Atk = Atk; D.GAtk = GAtk; D.Def = Def; D.GDef = GDef; D.Spd = Spd; D.GSpd = GSpd;
			D.AttackRange = Range; D.Abilities = MoveTemp(Ab);
			Out.Add(D);
		};
		Add(EShelfHouse::Undrum, TEXT("undrum"), TEXT("Undrum"), TEXT("Bearer"), TEXT("We are what the world leans on."), TEXT("Unassigned"),
			TEXT("You were born at depth in the Sink and never given a carry. In Hollowspine the word for that is the worst word they have: unassigned."),
			TEXT("Slow and immovable. Takes allies' weight, pins foes in place, turns stored load into crushing blows."),
			TEXT("Nobody in the Sink can lay a carry on you."),
			TEXT("C08A3E"), TEXT("2F2823"), TEXT("5B4835"), 120, 22, 11, 3, 7, 2, 4, 1, 220.f,
			{ "crush", "shoulder", "plumb", "set", "overburden" });
		Add(EShelfHouse::Saldric, TEXT("saldric"), TEXT("Saldric"), TEXT("Reckoner"), TEXT("Nothing is given. Everything is owed."), TEXT("One of the clear"),
			TEXT("You were born in the Hollow Markets of Tally with no bond to your name. On the Flats that means you do not legally exist."),
			TEXT("Binds enemies with debts that tug at them, then collects. Can move an ally's wounds onto a debtor."),
			TEXT("There is no bond with your name on it. I checked."),
			TEXT("E6DFCF"), TEXT("33405E"), TEXT("CFC6B0"), 100, 18, 10, 3, 5, 1, 6, 1, 220.f,
			{ "strike", "bind", "distraint", "assign", "forgive" });
		Add(EShelfHouse::Mirrowen, TEXT("mirrowen"), TEXT("Mirrowen"), TEXT("Hastener"), TEXT("Let it finish."), TEXT("Disowned lineage"),
			TEXT("Your lineage was struck from the Wound's rotations. You grew up on Imaryn's Road, the track that does not regrow."),
			TEXT("Sets rot running in every wound, then finishes it all at once. Can arrest a dying ally so they cannot fall."),
			TEXT("No rotation will call you home."),
			TEXT("7F9C45"), TEXT("3A2C3C"), TEXT("4F5B33"), 95, 16, 11, 3, 4, 1, 7, 1, 220.f,
			{ "touch", "quicken", "stay", "draught", "longgreen" });
		Add(EShelfHouse::Ikorran, TEXT("ikorran"), TEXT("Ikorran"), TEXT("Stepper"), TEXT("Nothing is far. Some things are merely unapproached."), TEXT("Struck from the rolls"),
			TEXT("The watchcourt struck your name from its rolls. With no shard that will watch for you, you crossed to the Shelf on foot."),
			TEXT("Fast and fragile. Hands blades into occupied space, steps out of harm, stretches the distance a foe must cross."),
			TEXT("No shard is watching for you."),
			TEXT("86A9CC"), TEXT("2B3850"), TEXT("4A5D73"), 85, 15, 12, 4, 4, 1, 9, 2, 900.f,
			{ "hand", "handthrough", "step", "stretch", "attenuate" });
		Add(EShelfHouse::Thessal, TEXT("thessal"), TEXT("Thessal"), TEXT("Regarder"), TEXT("What is watched, remains."), TEXT("Thinned"),
			TEXT("You drifted below notice in Vantage Antwhile. People forget you mid-sentence. You have learned to repeat yourself politely."),
			TEXT("Builds Regard from attention and spends it as raw force. Makes foes stare at nothing and hides allies from notice."),
			TEXT("Half my clerks have already forgotten you came in."),
			TEXT("EFE3C2"), TEXT("D9D2BF"), TEXT("A99F86"), 90, 16, 10, 3, 4, 1, 6, 1, 850.f,
			{ "glare", "scrutiny", "fix", "unregard", "persist" });
		return Out;
	}

	TArray<FAbilityDef> MakeAbilities()
	{
		TArray<FAbilityDef> Out;
		auto Add = [&Out](const TCHAR* Id, const TCHAR* Name, int32 Cost, float Cd, EAbilityTarget T, int32 Unlock, float Range, const TCHAR* Desc)
		{
			FAbilityDef A; A.Id = Id; A.Name = Name; A.Cost = Cost; A.Cooldown = Cd; A.Target = T; A.Unlock = Unlock; A.Range = Range; A.Desc = Desc;
			Out.Add(A);
		};
		using T = EAbilityTarget;
		// Undrum
		Add(TEXT("crush"), TEXT("Crush"), 0, 0, T::Enemy, 1, 240, TEXT("Auto-attack. A heavy, deliberate blow."));
		Add(TEXT("shoulder"), TEXT("Shoulder"), 1, 8, T::Ally, 1, 1200, TEXT("Take the most injured ally's weight for 8s: half of every hit on them comes to you, and each hit you absorb stores 1 Load."));
		Add(TEXT("plumb"), TEXT("Plumb"), 2, 6, T::Enemy, 1, 700, TEXT("Turn the target's weight against it: 1.3x damage and Heavy (half speed) for 6s."));
		Add(TEXT("set"), TEXT("Set"), 3, 14, T::Enemy, 1, 700, TEXT("Lend your weight to the target: it cannot act for 3s and any charge it is building breaks. You are Light (+50% damage taken) for 3s."));
		Add(TEXT("overburden"), TEXT("Overburden"), 2, 10, T::Enemy, 3, 260, TEXT("Put all stored Load through one body: 1x damage, +1x per Load."));
		// Saldric
		Add(TEXT("strike"), TEXT("Strike"), 0, 0, T::Enemy, 1, 240, TEXT("Auto-attack. A clerk's blade."));
		Add(TEXT("bind"), TEXT("Bind"), 1, 4, T::Enemy, 1, 800, TEXT("Witness a promise the target never meant to make: +2 Bond (max 6). Each Bond tugs for 2 damage every 2s."));
		Add(TEXT("distraint"), TEXT("Distraint"), 3, 10, T::Enemy, 1, 800, TEXT("Collect years from a bonded target: 0.9x damage per Bond, ignoring defense. You heal half."));
		Add(TEXT("assign"), TEXT("Assign"), 2, 12, T::Ally, 1, 1200, TEXT("Move the most injured ally's wounds onto your most-bonded enemy: heal 35% of their max HP and deal the same."));
		Add(TEXT("forgive"), TEXT("Forgiveness"), 5, 30, T::AllEnemies, 3, 1500, TEXT("Annul every bond at once. Each enemy takes 7 damage per Bond."));
		// Mirrowen
		Add(TEXT("touch"), TEXT("Hasten"), 0, 0, T::Enemy, 1, 240, TEXT("Auto-attack. 0.8x damage and +1 Rot (3 damage per stack every 2s, decaying)."));
		Add(TEXT("quicken"), TEXT("Quicken"), 2, 7, T::Enemy, 1, 700, TEXT("Advance every wound to its end: 1x damage plus all remaining Rot damage at once."));
		Add(TEXT("stay"), TEXT("Stay"), 2, 18, T::Ally, 1, 1200, TEXT("Arrest the most injured ally for 6s: they cannot fall below 1 HP and their Rot stops."));
		Add(TEXT("draught"), TEXT("Draught of the Slow"), 1, 10, T::Ally, 1, 1200, TEXT("Heal the most injured ally 40% now; 10% of their max HP comes due 8s later."));
		Add(TEXT("longgreen"), TEXT("The Long Green"), 0, 40, T::Self, 3, 0, TEXT("Spend your own years: lose 20% max HP, gain +60% attack and attack speed for 12s."));
		// Ikorran
		Add(TEXT("hand"), TEXT("Hand"), 0, 0, T::Enemy, 1, 900, TEXT("Auto-attack. A blade handed across the gap."));
		Add(TEXT("handthrough"), TEXT("Hand-through"), 2, 6, T::Enemy, 1, 1000, TEXT("Hand an arrowhead into occupied space: 1.6x damage, ignores defense."));
		Add(TEXT("step"), TEXT("Step"), 1, 10, T::Self, 1, 0, TEXT("Shorten the nearness: blink 4m away from your target and evade the next hit."));
		Add(TEXT("stretch"), TEXT("Stretch"), 2, 12, T::Enemy, 1, 1000, TEXT("Lengthen the target's last ten feet: 0.6x damage, Heavy for 6s, and its next hit deals half."));
		Add(TEXT("attenuate"), TEXT("Attenuate"), 3, 12, T::Enemy, 3, 1000, TEXT("Make a limb far from its body: 1.2x damage ignoring defense and +3 Rot."));
		// Thessal
		Add(TEXT("glare"), TEXT("Glare"), 0, 0, T::Enemy, 1, 850, TEXT("Auto-attack. 0.8x damage and +1 Regard (max 8)."));
		Add(TEXT("scrutiny"), TEXT("Scrutiny"), 1, 5, T::Enemy, 1, 900, TEXT("Spend all Regard: 0.75x damage per Regard, ignoring defense."));
		Add(TEXT("fix"), TEXT("Fix"), 2, 12, T::Enemy, 1, 900, TEXT("Make something compelling: 75% chance the target stares for 3s (30% on bosses), breaking any charge."));
		Add(TEXT("unregard"), TEXT("Unregard"), 2, 16, T::Ally, 1, 1200, TEXT("The most injured ally becomes not worth noticing: enemies lose interest in them for 5s."));
		Add(TEXT("persist"), TEXT("Spend a Regard"), 0, 20, T::Ally, 3, 1200, TEXT("Needs 3 Regard. The most injured ally persists past their nature: heal 60%, clear Rot, Stay 3s."));
		return Out;
	}

	TArray<FEnemyDef> MakeEnemies()
	{
		TArray<FEnemyDef> Out;
		auto Add = [&Out](const TCHAR* Id, const TCHAR* Name, EShelfHouse K, const TCHAR* Shape, int32 Hp, int32 Atk, int32 Def, int32 Spd, int32 Xp, int32 Salt, bool Boss, float Range, TArray<FName> Sk)
		{
			FEnemyDef E; E.Id = Id; E.Name = Name; E.Kind = K; E.Shape = Shape; E.Hp = Hp; E.Atk = Atk; E.Def = Def; E.Spd = Spd; E.Xp = Xp; E.Salt = Salt; E.bBoss = Boss; E.Range = Range; E.Skills = MoveTemp(Sk);
			Out.Add(E);
		};
		Add(TEXT("cutpurse"), TEXT("Ring-runner"), EShelfHouse::Ikorran, TEXT("humanoid"), 60, 9, 2, 9, 14, 6, false, 800, { "stab", "hthru" });
		Add(TEXT("broker"), TEXT("Bond-broker"), EShelfHouse::Saldric, TEXT("humanoid"), 70, 8, 3, 6, 14, 8, false, 700, { "stab", "bindE", "collect" });
		Add(TEXT("anchor"), TEXT("Anchor-man"), EShelfHouse::Undrum, TEXT("humanoid"), 110, 10, 5, 3, 18, 6, false, 220, { "maul", "plumbE" });
		Add(TEXT("hound"), TEXT("Spore-hound"), EShelfHouse::None, TEXT("hound"), 50, 8, 1, 8, 12, 2, false, 200, { "bite" });
		Add(TEXT("swine"), TEXT("Hollow-swine"), EShelfHouse::None, TEXT("swine"), 140, 11, 4, 4, 30, 4, false, 260, { "gore", "trample" });
		Add(TEXT("moths"), TEXT("Bloom-moths"), EShelfHouse::None, TEXT("moths"), 60, 6, 0, 10, 20, 2, false, 500, { "spores" });
		Add(TEXT("sellim"), TEXT("Sellim Ninth-Hand"), EShelfHouse::Undrum, TEXT("humanoid"), 520, 14, 6, 5, 120, 60, true, 240, { "cable", "plumbE", "shear" });
		return Out;
	}

	TArray<FItemDef> MakeItems()
	{
		TArray<FItemDef> Out;
		auto Add = [&Out](const TCHAR* Id, const TCHAR* Name, const TCHAR* Desc, int32 Price) { FItemDef I; I.Id = Id; I.Name = Name; I.Desc = Desc; I.Price = Price; Out.Add(I); };
		Add(TEXT("draught"), TEXT("Slow draught"), TEXT("Mirrowen medicine. Heals the most injured party member 50%. (Key 5)"), 15);
		Add(TEXT("bead"), TEXT("Regard bead"), TEXT("A charged bead of mirror-glass. Restores 3 Focus to the party leader. (Key 6)"), 20);
		Add(TEXT("salt"), TEXT("Witness-salt chip"), TEXT("Clears Rot and Bond from the whole party. (Key 7)"), 12);
		return Out;
	}
}

namespace ShelfData
{
	const TArray<FHouseDef>& Houses() { static TArray<FHouseDef> H = MakeHouses(); return H; }
	const FHouseDef& House(EShelfHouse E)
	{
		for (const FHouseDef& D : Houses()) if (D.House == E) return D;
		return Houses()[0];
	}
	EShelfHouse HouseFromKey(const FString& Key)
	{
		for (const FHouseDef& D : Houses()) if (D.Key == Key) return D.House;
		return EShelfHouse::None;
	}
	const FAbilityDef* Ability(FName Id)
	{
		static TArray<FAbilityDef> A = MakeAbilities();
		for (const FAbilityDef& D : A) if (D.Id == Id) return &D;
		return nullptr;
	}
	const FEnemyDef* Enemy(FName Id)
	{
		static TArray<FEnemyDef> E = MakeEnemies();
		for (const FEnemyDef& D : E) if (D.Id == Id) return &D;
		return nullptr;
	}
	const TArray<FItemDef>& Items() { static TArray<FItemDef> I = MakeItems(); return I; }
	const FItemDef* Item(FName Id)
	{
		for (const FItemDef& D : Items()) if (D.Id == Id) return &D;
		return nullptr;
	}
	int32 XPForLevel(int32 Level)
	{
		static const int32 Table[] = { 0, 0, 40, 100, 180, 280, 400 };
		return Table[FMath::Clamp(Level, 0, 6)];
	}
	int32 MaxLevel() { return 6; }
	FShelfStats Stats(const FShelfMember& M)
	{
		const FHouseDef& H = House(M.House);
		const int32 L = M.Level - 1;
		FShelfStats S;
		S.MaxHP = H.Hp + H.GHp * L; S.Atk = H.Atk + H.GAtk * L; S.Def = H.Def + H.GDef * L; S.Spd = H.Spd + H.GSpd * L;
		return S;
	}
}

namespace ShelfAssets
{
	UStaticMesh* Cube() { return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")); }
	UStaticMesh* Sphere() { return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")); }
	UStaticMesh* Cylinder() { return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")); }
	UStaticMesh* Cone() { return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone")); }
	UStaticMesh* Plane() { return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")); }
	UMaterialInterface* BaseMaterial() { return LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")); }
}
