// ===== Variables.h =====
#ifndef VARIABLES_H
#define VARIABLES_H
#include <windows.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

enum PageState {
	PAGE_COVER,
	PAGE_HOME,
	PAGE_PLAYING,
	PAGE_SCORES,
	PAGE_INSTRUCTIONS,
	PAGE_SETTINGS,
	PAGE_CREDITS,
	PAGE_PUZZLE1,   // Level 1 - Circuit Rerouting   (Pistol + Key)
	PAGE_PUZZLE2,   // Level 1 - Waveform Match      (Key + Health)
	PAGE_PUZZLE3,   // Level 2 - Clear Asteroids     (AK47 + Key)
	PAGE_PUZZLE4,   // Level 2 - Fix Weather Node    (Key + Health)
	PAGE_SIDEQUEST, // Level 2 - frozen switch mini-map (third door)
	PAGE_LEVEL3,        // Planet Titan - boss arena (Thanos)
	PAGE_BRIEF_LEVEL1,  // Instructions -> "Breach of Sanctuary II"
	PAGE_BRIEF_LEVEL2   // Instructions -> "Void Incursion"
};

extern PageState currentPage;
extern int MX;
extern int MY;
extern int bgImage;
extern float pulseTimer;
extern bool isMusicMuted;
extern bool isBGMPlaying;

// --- Level progression / puzzle-door gating (Map Design - Mahdin) ---
extern int currentLevel;        // 1 or 2 - which map/grid is active
extern int keysCollected;       // 0..2 - each solved puzzle grants one
extern bool hasMedkitPickup;    // reward from Puzzle 1 (Circuit Rerouting) - health boost
extern bool hasPistolPickup;    // no longer granted by any puzzle - pistol stays locked
extern bool hasAK47Pickup;      // reward from Puzzle 2 (Waveform Frequency Match) - unlocks the AK47
extern bool isLevel2Complete;   // set true when the hero touches the altar

// --- Scoring ---
// Total points earned this run. Awarded per enemy kill (see
// ENEMY_SCORE_VALUE in Header/enemy_properties.h and healHeroOnKill() in
// Header/enemy.h, which is called from every kill site regardless of
// weapon), shown in the HUD (drawLevel1HUD() in Playercontroller.hpp), and
// zeroed back to 0 on every fresh run/respawn (resetGameState() in
// level.hpp) - same lifetime as keysCollected above.
extern int score;

// --- Weapon unlocks -------------------------------------------------------
// Knife is the only weapon available from the start (key '1'). Every other
// weapon stays locked until the puzzle that hands it over is solved:
//   Level 1 / Puzzle 1 (Circuit)   -> Pistol + Key
//   Level 1 / Puzzle 2 (Waveform)  -> Key + Health
//   Level 2 / Door 1 (Asteroids)   -> AK47 + Key
//   Level 2 / Door 2 (Weather Node)-> Key + Health
//   Level 2 / Door 3 (Side quest)  -> Flamethrower issued on entry
extern bool hasFlamethrowerPickup;

// --- Pause -----------------------------------------------------------------
// ESC during gameplay parks the run (gamePaused = true) and drops to the
// main menu, where a RESUME button appears. ESC again, or RESUME, puts the
// player back on pausedReturnPage exactly where they left off. Nothing that
// advances the world runs while this is true - see fixedUpdate() in iMain.cpp.
extern bool gamePaused;
extern PageState pausedReturnPage;

// --- Level 2 side quest ----------------------------------------------------
extern bool sideQuestComplete;

// --- Space Stone (side quest reward) ---------------------------------------
// Awarded the moment the side quest switch is thrown. Its power is not
// implemented yet - for now it is a collectible + a toast (see
// spaceStoneToastTicks, drawn by GameFlow.hpp's overlay pass).
extern bool hasSpaceStone;
extern int spaceStoneToastTicks;

// --- Level 3 --------------------------------------------------------------
// Entered from the Level 2 exit portal once both puzzles are solved and both
// keys are held. isLevel3Complete flips when Thanos dies (+10000 score, saved to the scoreboard).
extern bool isLevel3Complete;

// --- settings -------------------------------------------------------------
// masterVolume is 0..100 and is mapped onto MCI's 0..1000 scale by
// SoundManager's applyMasterVolume().
extern int masterVolume;
extern int settingsToastTicks;

#endif