# The Shelf — Unreal Engine 5.8 project (Chapter One)

A dark-fantasy RPG built from *The Shelf* setting bible. This project contains all the gameplay — movement, real-time combat, the five Houses' powers, companions, enemies, dialogue, quests, journal, shop, saving — running on **grey-box placeholder shapes** so the art can be dropped in later without touching the rules.

> **Status:** written without access to Unreal, so it has **not been compiled yet**. Expect a handful of compile errors on the first build; fix them with Claude Code running on your machine (it can build and read the errors) — see step 4.

## 1. What you need

| | Windows | Mac |
|---|---|---|
| Engine | Unreal Engine **5.8** (Epic Games Launcher → Unreal Engine → Library) | same |
| Compiler | Visual Studio 2022 with **Game development with C++** and **Desktop development with C++** | Xcode (App Store) |

## 2. Open the project

1. Copy the `TheShelf` folder somewhere short, e.g. `C:\Dev\TheShelf` (Unreal dislikes long paths).
2. Right-click `TheShelf.uproject` → **Generate Visual Studio project files** (Mac: **Generate Xcode project**).
3. Double-click `TheShelf.uproject`. When asked to rebuild the `TheShelf` module, choose **Yes**.
4. If the build fails, open a terminal in the folder and run `claude` (Claude Code), then say: *"Build this Unreal project and fix the compile errors."*

No levels or Blueprints need creating. The game boots into Unreal's empty `Entry` map, and `AShelfWorld` builds every area in code. Press **Play**.

## 3. Controls (RuneScape / WoW style)

| Input | Action |
|---|---|
| Left click | Walk · talk to people · examine glowing objects · target an enemy (auto-attack) |
| Right-drag | Rotate camera |
| Mouse wheel | Zoom |
| W A S D | Walk |
| 1 – 4 | House powers |
| 5 / 6 / 7 | Slow draught · Regard bead · Witness-salt chip |
| Tab | Next enemy |
| E / Space / Enter | Interact · continue dialogue |
| J | Journal |

Combat is real-time: auto-attacks on a swing timer, powers cost **Focus** (regenerates 1 per 3 s, max 6) and have cooldowns. Companions (Nim, Oda) fight and use their powers on their own. Sellim, the Chapter One boss, periodically **shoulders the shelter's weight** — break her focus with *Set* (Undrum) or *Fix* (Thessal), or the whole party eats a *Shear*.

## 4. How the code is laid out

| File | What it does |
|---|---|
| `ShelfTypes.h`, `ShelfData.cpp` | Houses, abilities, enemies, items, level curve — all numbers from the setting bible in one place |
| `ShelfGameInstance` | Party, story flags, inventory, salt, journal, save/load (`SaveGame` slot `ShelfChapter1`) |
| `ShelfWorld` | Builds each area from a tile grid, A* pathfinding, NPCs/props/enemies, triggers, exits, lighting, the Ring's dimming clock |
| `ShelfCharacter` | Everyone in the game: stats, statuses (Rot, Bond, Load, Regard, Set, Stay…), every House power, enemy skills, placeholder body |
| `ShelfAIController` | Companion and enemy brains |
| `ShelfPlayerController` | Click-to-move, camera, hotkeys (Enhanced Input, created in code) |
| `ShelfHUD` | Title screen, House picker, party/target frames, action bar, dialogue, cards, journal |
| `ShelfStory` + `Content/Data/chapter1.json` | The whole Chapter One script |

### Writing story

`Content/Data/chapter1.json` is plain text, so you can edit dialogue without recompiling. Each scene is a list of steps:

```json
{ "say": "nim", "text": "Open." }
{ "choice": [ { "text": "Sit in it.", "do": [ ...steps... ] }, { "text": "Leave it.", "do": [] } ] }
{ "if": "readLog", "then": [ ... ], "else": [ ... ] }      // also "!flag", "flag>=3", "house:thessal", "party:ikorran"
{ "set": "flag" }   { "add": "counter" }   { "salt": 30 }   { "item": "draught", "n": 1 }
{ "journal": "Title", "text": "..." }   { "join": "oda", "name": "Oda", "house": "ikorran" }
{ "fight": ["r1", "r2", "r3"] }   { "card": "Title", "text": "..." }   { "area": "road", "spawn": [5, 62] }
{ "heal": true }   { "shop": true }   { "toast": "..." }   { "run": "other_scene" }   { "title": true }
```

Text can use `{name}`, `{house}`, `{exile}`, `{origin}`, `{sockets}`, and `[[socket]]` (renders as a blank — a Tolling hole).

## 5. Swapping in real art

The rules never touch meshes directly, so art is a drop-in:

1. **Characters:** in `AShelfCharacter::BuildHumanoid` / `BuildHound` / `BuildSwine` / `BuildMoths`, replace the primitive `AddPart` calls with a `USkeletalMeshComponent` using your character mesh and an Animation Blueprint (idle / walk / attack / cast / death). The legs/arms pivots are only there to fake walking.
2. **Environments:** in `AShelfWorld::BuildTerrain`, change the meshes passed to `MakeISM` (walls, paving, courts, rocks, grass) to your modular kit pieces. Tile size is 200 units.
3. **Props:** `AShelfProp::Build` — one branch per prop (`socket`, `sixthchair`, `shelter`, `gate`, `campfire`…).
4. **UI:** `ShelfHUD` is Canvas-drawn for speed of iteration; a UMG skin can replace it later.

### Asset shopping list (Fab marketplace)

Aim for one coherent stylized dark-fantasy look rather than mixing styles:

- **Modular ancient city kit** (pale grey, jointless stone; low, broad buildings with inner courtyards) — the Concord Ring.
- **Stylized medieval / fantasy characters** with a shared skeleton + an animation pack (idle, walk, run, melee, cast, hit, death) — party, NPCs, ring-runners, Sellim.
- **Fantasy creatures pack** with dogs/hounds and a boar/swine — spore-hounds, hollow-swine.
- **Scrubland / dry steppe foliage + rocks** — the Sixth Road and the Gate.
- **Medieval camp / outpost props** (tents, desks, crates, palisade) — the waystation.
- **VFX pack** (spores, glows, impacts, auras) — Rot, Bond, Regard, Shear, Step.
- **Ambient fantasy music + UI SFX** — for the RuneScape-style nostalgia.

## 6. Roadmap

- Chapter One is playable start to finish here and in the web prototype (`../shelf-rpg/index.html`).
- Next: art pass, sound, a UMG UI skin, then Chapter Two — *Eighteen Days Out* (beyond the gate).
