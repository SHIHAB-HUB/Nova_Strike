// ===== iMain.cpp =====
// STB_IMAGE_IMPLEMENTATION must be defined exactly ONCE in the whole
// project, right here, before iGraphics.h (which needs it for jpg/png
#define STB_IMAGE_IMPLEMENTATION
#include "iGraphics.h"
#include <time.h>

#include "Variables.h"
#include "SoundManager.hpp"
#include "level.hpp"
#include "GameFlow.hpp"      // must come AFTER level.hpp (uses floodProgress)
#include "Level3.hpp"        // must come AFTER level.hpp (uses TILE_SIZE/camera)
#include "LevelTransition.hpp"
#include "PlayerController.hpp"
#include "UIComponents.hpp"
#include "Puzzle1.hpp"
#include "Puzzle2.hpp"
#include "Puzzle3.hpp"
#include "Puzzle4.hpp"
#include "SideQuest.hpp"

// ---------------- Definition of extern globals (ONCE, here) ----------------
PageState currentPage = PAGE_COVER;
int MX = 0, MY = 0, bgImage = -1;
float pulseTimer = 0.0f;
bool isMusicMuted = false, isBGMPlaying = false;

// --- Level progression / puzzle-door state (Map Design - Mahdin) ---
int currentLevel = 1;
int keysCollected = 0;
bool hasMedkitPickup = false;
bool hasPistolPickup = false;
bool hasAK47Pickup = false;
bool isLevel2Complete = false;
int score = 0;

bool hasFlamethrowerPickup = false;
bool gamePaused = false;
PageState pausedReturnPage = PAGE_PLAYING;
bool sideQuestComplete = false;
bool hasSpaceStone = false;
int spaceStoneToastTicks = 0;
bool isLevel3Complete = false;
int masterVolume = 80;          // 0..100, mapped onto MCI's 0..1000
int settingsToastTicks = 0;

void updateFX() {
	if (gamePaused) return; // freeze every breathing/pulsing visual as well
	pulseTimer += 0.05f;
}

// fixedUpdate() is the ONLY function this iGraphics version calls on a
// timer for keyboard purposes (see iInitialize's internal SetTimer call).
// startFreshRun() is defined further down (it needs the puzzle resets);
// forward-declared here so the Game Over handler can call it.
void startFreshRun();
int isKeyPressed(unsigned char key);

// Game Over is dismissed with SPACE. Edge-detected so holding the fire
// button through a death does not instantly skip the screen.
inline void updateGameOverKey() {
	static bool spaceWasDown = false;
	bool spaceDown = isKeyPressed(' ') != 0;
	bool pressed = spaceDown && !spaceWasDown;
	spaceWasDown = spaceDown;

	if (!gameOverActive || !pressed) return;
	// Small grace period so the screen is actually readable.
	if (gameOverTicks < 20) return;

	int from = deathOriginLevel;
	clearGameOver();

	if (from == 2) {
		// Level 2 death -> redeploy from the start of Level 1.
		startFreshRun();
	}
	else {
		// Level 1 and Level 3 deaths -> back to the main menu.
		stopBGM();
		gamePaused = false;
		currentPage = PAGE_HOME;
		resetGameState();
		playMenuBGM();
	}
}

void fixedUpdate() {
	// ESC is always live - it is what pauses and unpauses.
	updateEscKey();

	// SPACE on the Game Over screen, before the gamePaused early-out so it
	// still works with the world frozen behind it.
	updateGameOverKey();

	// ESC from a briefing page steps back to the instructions index rather
	// than all the way out to the menu.
	{
		static bool escWasDownBrief = false;
		bool escDown = isKeyPressed(27) != 0;
		if (escDown && !escWasDownBrief &&
			(currentPage == PAGE_BRIEF_LEVEL1 || currentPage == PAGE_BRIEF_LEVEL2)) {
			currentPage = PAGE_INSTRUCTIONS;
		}
		escWasDownBrief = escDown;
	}

	// While the game is parked, nothing that advances the world may run:
	// no movement, no enemies, no flood, no oxygen, no animation clocks.
	// This is what makes RESUME pick up from the exact same frame.
	if (gamePaused) return;

	// Keeps Level 3's mp3 looping (MCI sometimes drops the repeat flag).
	// Above the puzzle guards so the boss theme keeps playing behind them.
	tickLevel3Track();

	// =================================================================
	// WORLD FREEZE WHILE A PUZZLE OR THE SIDE QUEST IS OPEN
	//
	// Opening a puzzle terminal or stepping into the side quest is a HARD
	// PAUSE on the main level, exactly like ESC. While either is on screen
	// the level behind it must not advance by a single tick:
	//
	//   - enemies do not move, chase or attack   (updateEnemies runs inside
	//     drawGameLevel, which only PAGE_PLAYING reaches)
	//   - bullets and grenades do not travel     (same call chain)
	//   - the hero does not move or fall         (updatePlayerMovement is
	//     gated on PAGE_PLAYING / PAGE_LEVEL3)
	//   - the Level 1 oxygen clock is frozen     (updateGameFlow, below)
	//   - the Level 2 flood + toxicity clock is frozen (updateLevel2Clock)
	//
	// isWorldRunning() is the single place that decides this, so a new page
	// can never accidentally leave half the simulation ticking.
	// =================================================================
	const bool worldRunning = (currentPage == PAGE_PLAYING || currentPage == PAGE_LEVEL3);

	// Cross-level systems: banners, oxygen clock, score-healing, death
	// watch, Space Stone toast. Runs before movement so a death this tick
	// freezes the world immediately rather than one frame late.
	if (settingsToastTicks > 0) settingsToastTicks--;
	updateGameFlow();
	if (gameOverActive) return;

	updatePlayerMovement();
	updateDoorAndPuzzleKeys();
	updateLevelTransition();

	// Side quest is its own little world with its own physics loop.
	if (currentPage == PAGE_SIDEQUEST) updateSideQuest();

	// The hit flash + camera shake is a LEVEL 2 effect only, per design. It
	// watches hero.currentHP rather than hooking any single damage source,
	// so it already covers every current and future enemy - anything that
	// calls hero.takeDamage() triggers it with no extra wiring.
	if ((currentPage == PAGE_PLAYING || currentPage == PAGE_SIDEQUEST) && currentLevel == 2)
		updateHitFeedback();

	// THE level 2 clock (flood + air toxicity). Only advances while the main
	// level is actually on screen, so a puzzle terminal, the side quest and
	// ESC all freeze the flood, the toxicity bar and every animated hazard
	// together. PAGE_SIDEQUEST used to be allowed through here, which meant
	// the water kept rising in Level 2 while the player was somewhere else
	// entirely - that is the bug this guard closes.
	if (currentLevel == 2 && !isLevelTransitioning() && worldRunning)
		updateLevel2Clock();

	// THE level 3 clock (Thanos-arrives-in wave timer + enemy top-up). Same
	// fixed-tick reasoning as updateLevel2Clock() just above - see the
	// bug-fix comment on updateLevel3Wave() itself (Level3.hpp) for why this
	// moved here instead of being ticked once per rendered frame.
	if (currentPage == PAGE_LEVEL3 && !isLevelTransitioning() && worldRunning)
		updateLevel3Wave();

	// Health drain stays a LEVEL 1 terminal mechanic. The two Level 2
	// puzzles are fully paused screens - nothing ticks against the player
	// while they are solving them, exactly as designed.
	if (currentPage == PAGE_PUZZLE1 || currentPage == PAGE_PUZZLE2) {
		updatePuzzleHealthDrain();
	}
	if (currentPage == PAGE_PUZZLE2) updatePuzzle2Input();
	if (currentPage == PAGE_PUZZLE3) updatePuzzle3();
	if (currentPage == PAGE_PUZZLE4) updatePuzzle4();
}

void iDraw() {
	iClear();
	switch (currentPage) {
	case PAGE_COVER:        drawCoverPage(); break;
	case PAGE_HOME:         drawHomePage(); break;
	case PAGE_SCORES:       drawScoresPage(); break;
	case PAGE_INSTRUCTIONS: drawInstructionsPage(); break;
	case PAGE_BRIEF_LEVEL1: drawBriefingLevel1(); break;
	case PAGE_BRIEF_LEVEL2: drawBriefingLevel2(); break;
	case PAGE_SETTINGS:     drawSettingsPage(); break;
	case PAGE_CREDITS:      drawCreditsPage(); break;
	case PAGE_PLAYING:      drawGameplayPage(); break;
	case PAGE_PUZZLE1:      drawPuzzle1Page(); break;
	case PAGE_PUZZLE2:      drawPuzzle2Page(); break;
	case PAGE_PUZZLE3:      drawPuzzle3Page(); break;
	case PAGE_PUZZLE4:      drawPuzzle4Page(); break;
	case PAGE_SIDEQUEST:    drawSideQuestPage(); break;
	case PAGE_LEVEL3:       drawLevel3(); break;
	}

	drawLevel1HUD();

	// Space Stone icon + cooldown, just under the top-right weapon readout.
	// Before the overlay pass so the title card / Game Over screen cover it.
	drawSpaceStoneHUD();

	// Everything GameFlow owns, drawn on top of the level but under the
	// transition fade: centered score + oxygen/toxicity bar, hazard banner,
	// level title card, Space Stone toast, Game Over screen.
	drawGameFlowOverlays();

	drawLevelTransitionOverlay();
}

// Fires while a button is HELD and the mouse moves - this is the drag feed
// the weather-node maze runs on.
void iMouseMove(int mx, int my) {
	MX = mx; MY = my;
	if (currentPage == PAGE_PUZZLE4) handlePuzzle4Drag(mx, my);
}
void iPassiveMouseMove(int mx, int my) { MX = mx; MY = my; }

// Shared by every "start a fresh run" entry point in the menu.
void startFreshRun() {
	stopBGM();
	gamePaused = false;
	currentPage = PAGE_PLAYING;
	playGameBGM();
	initGameLevel();

	pz1Solved = false; pz1Initialized = false;
	pz2Solved = false; pz2Amplitude = 25.0f; pz2Frequency = 0.6f; pz2Phase = 0.0f;
	resetPuzzle3();
	resetPuzzle4();

	// Cross-level flow state has to be rewound too, or a second run
	// inherits the first run's oxygen clock and heal milestones.
	clearGameOver();
	resetLevel1Clock();
	resetScoreHealing();
	hasSpaceStone = false;
	spaceStoneToastTicks = 0;
	resetSpaceStone();   // ability comes back ready if the stone is earned again
	isLevel3Complete = false;
	showLevelTitle(1);

	levelTransitionState = TRANS_NONE; transitionAlpha = 0.0f;
	pistolLockedToastTicks = 0;
	ak47LockedToastTicks = 0;
	flameLockedToastTicks = 0;
	rewardToastTicks = 0;
	rewardToast2Ticks = 0;
}

void iMouse(int button, int state, int mx, int my) {
	if (button != GLUT_LEFT_BUTTON) return;

	// The maze needs press AND release, so it is handled before the
	// press-only early-out below.
	if (currentPage == PAGE_PUZZLE4) {
		if (state == GLUT_DOWN) handlePuzzle4Press(mx, my);
		else                    handlePuzzle4Release(mx, my);
		return;
	}

	if (state != GLUT_DOWN) return;

	if (currentPage == PAGE_PUZZLE1) { handlePuzzle1Click(mx, my); return; }
	if (currentPage == PAGE_PUZZLE3) { handlePuzzle3Click(mx, my); return; }

	if (currentPage == PAGE_COVER) { currentPage = PAGE_HOME; return; }

	// ---- INSTRUCTIONS: pick a briefing ----
	if (currentPage == PAGE_INSTRUCTIONS) {
		for (int i = 0; i < 2; i++) {
			int y = INSTR_ENTRY_Y - i * (INSTR_ENTRY_H + 18);
			if (mx >= INSTR_ENTRY_X && mx <= INSTR_ENTRY_X + INSTR_ENTRY_W &&
				my >= y && my <= y + INSTR_ENTRY_H) {
				currentPage = (i == 0) ? PAGE_BRIEF_LEVEL1 : PAGE_BRIEF_LEVEL2;
				return;
			}
		}
		return;
	}

	// ---- SCORES: read-only ----
	// The RESET control deliberately does NOT live here. Wiping the record
	// is a settings action, not something that should sit one stray click
	// away from the page whose whole job is to display that record.
	if (currentPage == PAGE_SCORES) return;

	// ---- SETTINGS: mute, volume, reset ----
	if (currentPage == PAGE_SETTINGS) {
		int y = SETTINGS_FIRST_Y;

		if (mx >= SETTINGS_BTN_X && mx <= SETTINGS_BTN_X + SETTINGS_BTN_W &&
			my >= y && my <= y + SETTINGS_BTN_H) {
			isMusicMuted = !isMusicMuted;
			if (isMusicMuted) stopBGM();
			else if (currentPage == PAGE_SETTINGS) playMenuBGM();
			applyMasterVolume();
			return;
		}

		y -= SETTINGS_ROW_GAP;
		if (my >= y && my <= y + SETTINGS_BTN_H) {
			if (mx >= SETTINGS_VOL_DOWN_X && mx <= SETTINGS_VOL_DOWN_X + SETTINGS_STEP_W) {
				masterVolume -= 10;
				if (masterVolume < 0) masterVolume = 0;
				applyMasterVolume();
				return;
			}
			if (mx >= SETTINGS_VOL_UP_X && mx <= SETTINGS_VOL_UP_X + SETTINGS_STEP_W) {
				masterVolume += 10;
				if (masterVolume > 100) masterVolume = 100;
				applyMasterVolume();
				return;
			}
		}

		y -= SETTINGS_ROW_GAP;
		if (mx >= SETTINGS_RESET_X && mx <= SETTINGS_RESET_X + SETTINGS_BTN_W &&
			my >= y && my <= y + SETTINGS_BTN_H) {
			resetHighScoreRecord();   // clears HIGHSCORE in highscore.txt
			settingsToastTicks = 90;
			return;
		}
		return;
	}

	if (currentPage == PAGE_HOME) {
		// RESUME - only present while a run is parked.
		if (gamePaused && mx >= RESUME_BTN_X && mx <= RESUME_BTN_X + RESUME_BTN_W &&
			my >= RESUME_BTN_Y && my <= RESUME_BTN_Y + RESUME_BTN_H) {
			resumeGame();
			return;
		}

		if (mx >= 540 && mx <= 740 && my >= 480 && my <= 530) {
			startFreshRun();
		}
		else if (mx >= 540 && mx <= 740 && my >= 400 && my <= 450) currentPage = PAGE_SCORES;
		else if (mx >= 540 && mx <= 740 && my >= 320 && my <= 370) currentPage = PAGE_INSTRUCTIONS;
		else if (mx >= 540 && mx <= 740 && my >= 240 && my <= 290) currentPage = PAGE_SETTINGS;
		else if (mx >= 540 && mx <= 740 && my >= 160 && my <= 210) currentPage = PAGE_CREDITS;
		else if (mx >= 540 && mx <= 740 && my >= 80 && my <= 130) exit(0);

		// --- subpage widgets ---------------------------------------------
		// (handled before the debug skips so a stray click on the menu
		// coordinates cannot fall through into a level jump)
		// --- debug skips, bottom-right (geometry lives in UIComponents.hpp) ---
		else if (mx >= DEBUG_L2_X && mx <= DEBUG_L2_X + DEBUG_BTN_W &&
			my >= DEBUG_L2_Y && my <= DEBUG_L2_Y + DEBUG_BTN_H) {
			startFreshRun();
			keysCollected = 2;
			hasMedkitPickup = true;
			hasPistolPickup = true;
			startLevel2Transition();
		}
		else if (mx >= DEBUG_L3_X && mx <= DEBUG_L3_X + DEBUG_BTN_W &&
			my >= DEBUG_L3_Y && my <= DEBUG_L3_Y + DEBUG_BTN_H) {
			// Straight into the Titan arena, fully kitted, so the boss fight
			// can be tested without replaying two levels of puzzles.
			startFreshRun();
			keysCollected = 2;
			hasMedkitPickup = true;
			hasPistolPickup = true;
			hasAK47Pickup = true;
			hasFlamethrowerPickup = true;
			enterLevel3();
		}
	}
}

int main() {
	iInitialize(SCREEN_WIDTH, SCREEN_HEIGHT, "NovaStrike: Ascension");
	iSetTimer(30, updateFX);
	// Seed once at boot so the randomized weather-node maze (Puzzle4) and
	// the circuit scramble (Puzzle1) differ between sessions instead of
	// replaying the same "random" layout every launch.
	srand((unsigned int)time(NULL));

	initWeaponAudio();
	applyMasterVolume();   // opens pistol/ak47/flame mp3 once, under MCI aliases
	bgImage = iLoadImage((char*)"Images\\bg1.jpg");
	playMenuBGM();
	iStart();
	return 0;
}