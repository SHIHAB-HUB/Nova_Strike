# NovaStrike: Ascension

A 2D side-scrolling action-platformer for Windows, written in C++ on top of the **iGraphics** (OpenGL/GLUT) framework. You play as **Nova**, a lone soldier who boards an orbital station, fights through a failing derelict ship, and finally faces **Thanos** on the surface of Titan.

> Version 1.0 - a university course project, built by students at AUST.

---

## Table of Contents

1. [Story](#story)
2. [Gameplay Overview](#gameplay-overview)
3. [Controls](#controls)
4. [Levels](#levels)
5. [Weapons](#weapons)
6. [Enemies](#enemies)
7. [Scoring and Save Data](#scoring-and-save-data)
8. [Getting Started](#getting-started)
9. [Project Structure](#project-structure)
10. [Credits](#credits)
11. [Legal](#legal)

---

## Story

Thanos has thrown his army at Earth to seize the Mind and Soul Stones. The orbital station **Sanctuary II** is the staging ground. Nova goes in alone, and goes in first.

## Gameplay Overview

- **Three levels**, each with its own map, hazards, music and objective.
- **Two survival clocks** in the first two levels: an oxygen timer in Level 1 and a rising toxic flood in Level 2 (four minutes each).
- **Four puzzle terminals** that you solve to earn keycards, weapons and health. Opening a puzzle freezes the world behind it.
- **An optional side quest** in Level 2 that rewards you with the Space Stone, a teleport ability, more max HP and the Plasma Rifle.
- **A boss fight** against Thanos, who revives once with a bigger health pool.
- **Persistent high score, run count and best time**, saved between sessions.
- **Pause and resume** from anywhere in a run with `ESC`.

## Controls

### In the level

| Action                              | Keys                                                                |
| ----------------------------------- | ------------------------------------------------------------------- |
| Move left / right                   | `A` / `D` or Left / Right arrows                                    |
| Jump                                | `W` or Up arrow                                                     |
| Fire / attack                       | `SPACE`                                                             |
| Interact (doors, terminals)         | `E` (`Q` also works on doors)                                       |
| Throw grenade                       | `TAB`                                                               |
| Switch weapon                       | `1` Knife, `2` Pistol, `3` AK47, `4` Flamethrower, `5` Plasma Rifle |
| Quick-toggle AK47 / Knife           | `K`                                                                 |
| Space Stone teleport                | `Q` (once the stone is earned)                                      |
| Skip the Level 3 pre-boss countdown | `P`                                                                 |
| Pause / resume                      | `ESC`                                                               |

Locked weapons show a "locked" notice if you try to equip them.

### Menus and screens

| Screen           | Keys                                                   |
| ---------------- | ------------------------------------------------------ |
| Cover page       | Click anywhere                                         |
| Game Over        | `SPACE`                                                |
| Victory          | `ENTER` or `M` for the main menu, `R` to play again    |
| Puzzle terminals | `ESC` to back out (no reward is granted)               |
| Side quest       | `ESC` to walk out; you can re-enter from the same door |

### Waveform Match puzzle

| Adjust    | Keys      |
| --------- | --------- |
| Amplitude | `A` / `D` |
| Frequency | `W` / `S` |
| Phase     | `Q` / `E` |

The other puzzles (Circuit Rerouting, Clear Asteroids, Weather Node) are mouse-driven. The Weather Node maze uses click-and-drag.

## Levels

### Level 1 - Breach of Sanctuary II

The station is venting atmosphere and you have **four minutes of air**. When it runs out you asphyxiate. Two sealed terminals hold the override codes:

- **Circuit Rerouting** - unlocks the Pistol and grants a keycard.
- **Waveform Match** - grants a keycard and restores health (a medkit).

The exit door needs **two keys**. Health also drains slowly while a Level 1 terminal is open, so solve fast.

### Level 2 - Void Incursion

The Q-ship core is failing and toxic fallout is flooding the lower decks. **Four minutes until saturation - altitude is survival.**

- **Hazards:** gravity wells invert your footing, thrust vents launch you higher than a jump, and dead sections kill all light but your own.
- **Door 1 - Clear Asteroids:** unlocks the AK47 and grants a key.
- **Door 2 - Fix Weather Node:** unlocks the Flamethrower and grants a key.
- **Door 3 - Side quest (optional, dangerous):** a frozen switch mini-map marked DO NOT ENTER. Completing it awards the **Space Stone**, raises max HP, and unlocks the **Plasma Rifle**.
- With both keys, the rift on the highest right ledge opens.

### Level 3 - Titan

A one-minute wave of regular enemies, then **Thanos** arrives. He has 5,000 HP on his first life and revives once with 10,000 HP. When he finally falls you earn a large score bonus, the run is saved to the scoreboard, and the victory screen shows your time.

## Weapons

| Slot | Weapon       | Unlocked by                 | Notes                                                                       |
| ---- | ------------ | --------------------------- | --------------------------------------------------------------------------- |
| 1    | Knife        | Start                       | Melee, always available                                                     |
| 2    | Pistol       | Level 1 Circuit puzzle      | 320 damage; 6 shots, then a 2 s cooldown                                    |
| 3    | AK47         | Level 2 Asteroids puzzle    | 60 damage per round, automatic; magazine cap grows in Level 2, 2 s cooldown |
| 4    | Flamethrower | Level 2 Weather Node puzzle | 100 damage per tick to everything in the flame area                         |
| 5    | Plasma Rifle | Space Stone (side quest)    | 260-450 damage, falls off with distance                                     |
| -    | Grenade      | Start (`TAB`)               | 250 damage, small blast radius, 2 per life                                  |

**Space Stone:** press `Q` to teleport 600 px in the direction you are facing. 10 second cooldown.

## Enemies

Eight enemy types with their own walk/fly and attack animations, speeds, sizes, health and aggro range. Highlights:

- **Enemy 1** - fast, fragile ground unit (100 HP).
- **Enemy 4** - the only flyer (1,000 HP).
- **Enemy 7** - the heavy tank: biggest, slowest, most damage (700 HP).
- **Enemy 8** - a quick, fragile skirmisher (80 HP).

Every kill awards points and heals the hero a little. Tuning values live in `Header/enemy_properties.h`.

## Scoring and Save Data

Score is awarded per enemy kill and shown in the HUD. It resets on every fresh run. Progress across sessions is stored in plain text in `highscore.txt`:

```
NOVASTRIKE_SAVE 1
HIGHSCORE 0
TOTAL_RUNS 24
LAST_SCORE 4955
LAST_TIME 323
BEST_TIME 0
```

Times are in whole seconds, and `BEST_TIME 0` means no clear has been recorded yet. You can wipe the record from **Settings > Reset**.

## Getting Started

### Requirements

- **Windows** (the game uses the Win32 API and MCI for audio)
- **Visual Studio 2013** (platform toolset `v120`), or a newer Visual Studio that can retarget the project
- **Win32 (x86)** build configuration

All third-party libraries the game needs are already included in the project folder: `glut32.lib`, `glaux.lib`, `GLU32.LIB`, `OPENGL32.LIB`, `glui32.lib`, plus `GLUT32.DLL`, `glut.h`, `glaux.h` and `stb_image.h`. Audio uses `winmm.lib`, which ships with Windows.

### Build and run

1. Open `Nova_Strike/Nova_Strike.sln` in Visual Studio.
2. Select the **Debug | Win32** (or **Release | Win32**) configuration.
3. Build and run (`F5`).

### Running the prebuilt executable

A debug build is included at `Nova_Strike/Debug/Nova_Strike.exe`. The game loads its assets using relative paths (`Images\...`, `Audios\...`, `data\...`), so the **working directory must be the inner project folder** (`Nova_Strike/Nova_Strike/`), which is what Visual Studio uses by default. If you launch the `.exe` by double-clicking it from the `Debug` folder, assets may fail to load. Run it from Visual Studio, or copy the executable next to the asset folders.

### Menu tips

- **Play** starts a fresh run from Level 1.
- **Mission Briefing** shows the story and controls for Levels 1 and 2.
- **Settings** has a music/sound mute, master volume, and high-score reset.
- **Resume** appears on the main menu while a run is parked with `ESC`.
- Two **debug buttons** at the bottom right of the main menu jump straight to Level 2 or to the Level 3 boss arena (fully equipped), which is handy for testing.

## Project Structure

```
Nova_Strike_Final/
└── Nova_Strike/
    ├── Nova_Strike.sln
    ├── Debug/                      Prebuilt debug executable
    └── Nova_Strike/                Project folder (working directory)
        ├── iMain.cpp               Entry point, main loop, input routing
        ├── iGraphics.h             iGraphics framework
        ├── Variables.h             Global state and page enum
        ├── GameFlow.hpp            Oxygen clock, score, banners, Game Over, victory
        ├── level.hpp               Level 1 map and gameplay
        ├── Level2.hpp              Level 2 map, flood clock, hazards
        ├── Level3.hpp              Titan arena and wave/boss logic
        ├── Leveltransition.hpp     Fades between levels
        ├── Playercontroller.hpp    Movement, weapon switching, pause
        ├── UIComponents.hpp        Menus, briefings, settings, credits
        ├── Puzzle1-4.hpp           The four puzzle terminals
        ├── Puzzlecommon.hpp        Shared puzzle rewards and drain
        ├── SideQuest.hpp           Level 2 optional side quest
        ├── SpaceStone.hpp          Teleport ability and HUD
        ├── SoundManager.hpp        Music and sound effects (MCI)
        ├── Header/                 Hero, weapons, bullets, enemies, Thanos, HUD
        ├── data/levels/            level1.txt, level2.txt tile maps
        ├── Images/                 Backgrounds, tiles, hero, Thanos, UI sprites
        ├── Enemy_Images/           Animation frames for the 8 enemy types
        ├── Audios/                 Music and weapon/effect sounds
        └── highscore.txt           Persistent save data
```

Levels 1 and 2 are laid out as tile grids in `data/levels/*.txt`, where each number is a tile type (0 is empty space, 1 is a solid block, 2-4 are platform pieces, and so on).

## Credits

**Supervised by**

- Mr. Saha Reno - Assistant Professor
- Md. Zahid Hossain - Lecturer, Grade-I

Department of Computer Science & Engineering, AUST

**Developed by**

- Mahdin Al Rahman (00725105101122)
- Abdullah Al Amin Monim (00725105101127)
- Md. Shihabul Islam (00725105101142)

**Special thanks and developer assistance**

- Anthropic Claude - logic and debugging
- Google Gemini - asset sourcing and AI assistance
- Google Search - asset research and sprite sourcing
- iGraphics Framework - engine and rendering base

## Legal

"Thanos", "Sanctuary II" and "Earth-616" belong entirely to Marvel Studios and The Walt Disney Company. This is a zero-budget, non-commercial student project made for a grade, and is not affiliated with or endorsed by Marvel or Disney.
