// ===== Playercontroller.hpp =====
#ifndef PLAYERCONTROLLER_HPP
#define PLAYERCONTROLLER_HPP

// Player input, movement ar weapon switching logic

#include "Variables.h"
#include "level.hpp"
#include "GameFlow.hpp"   // gameOverActive, showLevelTitle
#include "Level3.hpp"     // Level 3 physics + enterLevel3()
#include "SpaceStone.hpp"  // Space Stone teleport (Q) + cooldown timer - must come AFTER level.hpp / GameFlow.hpp / Level3.hpp
#include "LevelTransition.hpp"
#include "SoundManager.hpp"
#include "Puzzle1.hpp"
#include "Puzzle2.hpp"
#include "Puzzle3.hpp"
#include "Puzzle4.hpp"
#include "SideQuest.hpp"

// iGraphics internal functions declaration
int isKeyPressed(unsigned char key);
int isSpecialKeyPressed(unsigned char key);

#define PLAYER_SPEED 4.0f
#define INTERACTION_RADIUS 60

// Pistol unlock na thakle warning show korar timer
__declspec(selectany) int pistolLockedToastTicks = 0;

// AK47 unlock na thakle warning show korar timer
__declspec(selectany) int ak47LockedToastTicks = 0;

// Flamethrower locked warning (unlocked by solving Puzzle 4 - Weather Node,
// see grantFlamethrowerAndKey() in PuzzleCommon.hpp)
__declspec(selectany) int flameLockedToastTicks = 0;

// Plasma Rifle locked warning (unlocked once the Space Stone is secured -
// hasSpaceStone, awarded at the end of the Level 2 side quest)
__declspec(selectany) int plasmaLockedToastTicks = 0;

// "DO NOT ENTER" warning shown near the Level 2 side-quest door
__declspec(selectany) int doNotEnterTicks = 0;

// Screen border check shoho player coordinate update
inline void movePlayer(float dx, float dy) {
	if (dx < 0 && gamePlayer.x > 40.0f) gamePlayer.x += dx;
	if (dx > 0 && gamePlayer.x < (MAX_COLS * TILE_SIZE) - gamePlayer.width) gamePlayer.x += dx;
	if (dy > 0 && gamePlayer.y < (MAX_ROWS * TILE_SIZE) - gamePlayer.height) gamePlayer.y += dy;
	if (dy < 0 && gamePlayer.y > 0.0f) gamePlayer.y += dy;
}

// Every frame execution: player action, weapon attack, cooldown & gravity state update
inline void updatePlayerMovement() {
	if (currentPage != PAGE_PLAYING && currentPage != PAGE_LEVEL3) return;

	// Hero mara gele Game Over screen dekhabe.
	// CHANGED: this used to call resetGameState() immediately, which wiped
	// the run (and the score) on the same tick the hero died - the Game Over
	// screen never got a chance to read the final score. Now death just
	// freezes movement here; GameFlow.hpp's updateDeathWatch() raises the
	// screen, and the SPACE handler in iMain.cpp decides where the player
	// goes next (menu vs. restart at Level 1) per the respawn matrix.
	if (hero.isDead()) return;

	// Game Over screen is up - the world behind it stays frozen.
	if (gameOverActive) return;

	// Level transition fademode e movement stop
	if (isLevelTransitioning()) return;

	// Level 2 dynamic hazards run on this same fixed tick, BEFORE input and
	// gravity: moving platforms have to shift (and carry their rider), the
	// laser gate has to settle solid/open, and gravityDir has to be current
	// before any collision or gravity test happens this tick.
	if (currentLevel == 2) updateLevel2Dynamics();

	// Space Stone (SpaceStone.hpp): ticks the 10s cooldown and handles the Q
	// teleport. Runs before the movement keys below, so a teleport lands first
	// and this same tick's walking/gravity carry on from the new position.
	// Being down here (past the dead / Game Over / level-transition early
	// returns above) is what freezes the cooldown in all of those states.
	updateSpaceStone();

	// =================================================================
	// LEVEL 3 ADRENALINE SURGE
	//
	// Below HERO_LOW_HP_FRACTION (40%) of max HP in the Titan arena, every
	// kind of movement the hero has speeds up by HERO_LOW_HP_SPEED_MULT
	// (1.3x) - walking (Hero::stepPixels()) and the jump impulse
	// (Hero::jumpImpulse()), both of which read speedScale.
	//
	// Recomputed from scratch EVERY tick rather than latched, which is what
	// makes it self-undoing: the moment a score heal or a kill heal pushes
	// him back over 40% the scale drops to 1.0 again with no cleanup code
	// anywhere. It is also written unconditionally on every level, so
	// leaving Level 3 (or dying and redeploying into Level 1) can never
	// leave a stale 1.3x behind.
	//
	// 40% is of CURRENT max HP, so it follows the Space Stone boost
	// automatically: 120 HP on the 300 base, 200 HP once max is 500.
	// =================================================================
	hero.speedScale = (currentLevel == 3 &&
		hero.healthPercent() < HERO_LOW_HP_FRACTION)
		? HERO_LOW_HP_SPEED_MULT : 1.0f;

	// Keyboard keys polling
	bool leftHeld = isKeyPressed('a') || isKeyPressed('A') || isSpecialKeyPressed(GLUT_KEY_LEFT);
	bool rightHeld = isKeyPressed('d') || isKeyPressed('D') || isSpecialKeyPressed(GLUT_KEY_RIGHT);

	// Level mapping check dynamic movement
	if (leftHeld) {
		if (currentLevel == 3)      heroMoveHorizontalL3(-1);
		else if (currentLevel == 2) heroMoveHorizontalL2(-1);
		else                        heroMoveHorizontal(-1);
	}
	if (rightHeld) {
		if (currentLevel == 3)      heroMoveHorizontalL3(1);
		else if (currentLevel == 2) heroMoveHorizontalL2(1);
		else                        heroMoveHorizontal(1);
	}

	// Key chere dile horizontal speed reset
	if (!leftHeld && !rightHeld) hero.stopMoving();

	if (isKeyPressed('w') || isKeyPressed('W') || isSpecialKeyPressed(GLUT_KEY_UP)) {
		// Level 2 jumps have to launch away from whichever surface gravity
		// is currently holding the hero to (floor OR ceiling).
		if (currentLevel == 2) heroStartJumpL2();
		else                   hero.startJump();
	}

	bool spaceHeld = isKeyPressed(' ') != 0;

	// Flamethrower active frame tick handle
	if (hero.weapon.equipped == WEAPON_FLAMETHROWER) {
		FlameThrower& flame = hero.weapon.flamethrower;
		bool wasActive = flame.isActive();
		bool dealsDamageNow = flame.update(spaceHeld);

		// The flame is a sustained stream, so its clip is started when the
		// stream starts and stopped when it stops - re-triggering it every
		// tick would machine-gun the sample and crackle.
		if (!wasActive && flame.isActive()) playFlameSfx();
		if (wasActive && !flame.isActive()) stopFlameSfx();

		hero.isAttacking = flame.isActive();
		hero.atkIndx = flame.frameIndex;

		if (dealsDamageNow) {
			int x1, y1, x2, y2;
			hero.flameHitbox(x1, y1, x2, y2);
			damageEnemiesInBox(x1, y1, x2, y2, FLAMETHROWER_DMG, hero.facing, hero);

			// BUG FIX: the flamethrower never reached Thanos at all - it only
			// ever called damageEnemiesInBox(), which is the regular enemy
			// roster's hit test (Header/enemy.h) and has no idea Thanos
			// exists. Every other weapon that can damage him (the knife's
			// melee hitbox just above in this same function, every bullet in
			// level.hpp's updateBullets()) has its own extra
			// "if (currentLevel == 3) damageThanos(...)" call alongside the
			// regular-enemy hit test - the flamethrower was simply missing
			// that second call, so its stream visibly played and looked
			// like it was hitting him but never touched his HP bar at all.
			if (currentLevel == 3) damageThanos(x1, y1, x2, y2, FLAMETHROWER_DMG);
		}
	}
	else {
		if (spaceHeld) {
			bool justStarted = hero.startAttack();

			// Bullet create & spawn math
			if (justStarted && (hero.weapon.equipped == WEAPON_PISTOL || hero.weapon.equipped == WEAPON_AK47 || hero.weapon.equipped == WEAPON_PLASMARIFLE)) {
				int muzzleY = hero.posY + hero.HEIGHT / 2 - BULLET_HEIGHT / 2 + BULLET_MUZZLE_Y_OFFSET;
				int muzzleX = (hero.facing >= 0) ? (hero.posX + hero.WIDTH) : (hero.posX - BULLET_WIDTH);
				unsigned int img = hero.weapon.bulletImg(hero.facing);
				bool isPlasma = (hero.weapon.equipped == WEAPON_PLASMARIFLE);
				spawnBullet(muzzleX, muzzleY, hero.facing, hero.weapon.damage(), img, isPlasma);

				hero.weapon.registerShotFired();

				// One SFX slice per round actually leaving the barrel - tied
				// to the shot, not to the key being held, so the sound can
				// never drift out of sync with the firing animation.
				if (hero.weapon.equipped == WEAPON_PISTOL)             playPistolSfx();
				else if (hero.weapon.equipped == WEAPON_AK47)          playAk47Sfx();
				// The Plasma Rifle used to fire silently - it now runs the
				// same one-slice-per-bolt path off Audios\PlasmaRifle.mp3
				// (see playPlasmaSfx() in SoundManager.hpp). Same place in
				// the code as the other two, so it is tied to a round
				// actually leaving the barrel rather than to the key being
				// held, and it can never drift out of sync with the firing
				// animation.
				else if (hero.weapon.equipped == WEAPON_PLASMARIFLE)   playPlasmaSfx();
			}
		}

		// Melee / Knife hit collision detect
		if (hero.isAttacking) {
			bool hitsNow = hero.updateAttack();

			if (hitsNow && hero.weapon.equipped == WEAPON_KNIFE) {
				int x1, y1, x2, y2;
				hero.attackHitbox(x1, y1, x2, y2);

				damageEnemiesInBox(x1, y1, x2, y2, hero.weapon.damage(), hero.facing, hero);

				// Thanos only exists in Level 3's boss arena (see
				// Header/thanos.hpp) - the regular enemy roster above never
				// overlaps him, so this is a separate hit check, not a
				// replacement for it.
				if (currentLevel == 3) damageThanos(x1, y1, x2, y2, hero.weapon.damage());
			}
		}
	}

	// Grenade throw key listener - TAB (was 'Q').
	//
	// TAB arrives through the SAME glutKeyboardFunc path every letter key
	// does (it's a printable-range control character, ASCII 9 - '\t'), so
	// it lives in iGraphics' keyPressed[] array exactly like 'q' did and
	// needs no isSpecialKeyPressed() handling. It's edge-detected here,
	// not inside Grenade::startThrow(), so holding TAB throws once rather
	// than spamming a throw every tick.
	//
	// 'Q' was moved off the grenade because it's still read as an
	// alternative door-interaction key in updateDoorAndPuzzleKeys() below;
	// TAB has no other binding anywhere in the game.
	const unsigned char KEY_TAB = 9;
	static bool tabWasDown = false;
	bool tabIsDown = isKeyPressed(KEY_TAB) != 0;
	if (tabIsDown && !tabWasDown && !hero.isAttacking) {
		hero.weapon.grenade.startThrow(hero.facing);
	}
	tabWasDown = tabIsDown;

	// Grenade render offset & trajectory tick update
	{
		int launchX = (hero.facing >= 0) ? (hero.posX + hero.WIDTH) : (hero.posX - GRENADE_ICON_WIDTH);
		int launchY = hero.posY;
		hero.weapon.grenade.updateThrow(launchX, launchY);
	}

	// Attack delay timing sync
	hero.weapon.updateCooldowns();

	// Weapon swap selection keys (1 to 5). Grenade no longer has a number-key
	// slot of its own - it's still thrown with TAB regardless of what's
	// equipped (see the grenade throw key listener above) - so removing its
	// slot here doesn't take grenades away, just the ability to "equip" it.
	// FlameThrower sits at slot 4, unlocked by solving Puzzle 4 (see
	// grantFlamethrowerAndKey() in PuzzleCommon.hpp). Plasma Rifle is the
	// new 5th weapon at slot 5, unlocked once the Space Stone is secured
	// (hasSpaceStone, awarded at the end of the Level 2 side quest - see
	// SideQuest.hpp) - "getting the stone" is what unlocks it, same idea as
	// every other weapon's own pickup flag.
	if (!hero.isAttacking) {
		if (isKeyPressed('1')) hero.weapon.equip(WEAPON_KNIFE);

		if (isKeyPressed('2')) {
			if (hasPistolPickup) hero.weapon.equip(WEAPON_PISTOL);
			else pistolLockedToastTicks = 45;
		}

		if (isKeyPressed('3')) {
			if (hasAK47Pickup) hero.weapon.equip(WEAPON_AK47);
			else ak47LockedToastTicks = 45;
		}

		if (isKeyPressed('4')) {
			if (hasFlamethrowerPickup) hero.weapon.equip(WEAPON_FLAMETHROWER);
			else flameLockedToastTicks = 45;
		}

		if (isKeyPressed('5')) {
			if (hasSpaceStone) hero.weapon.equip(WEAPON_PLASMARIFLE);
			else plasmaLockedToastTicks = 45;
		}
	}

	// 'K' shortcut key toggling
	static bool kWasDown = false;
	bool kIsDown = isKeyPressed('k') != 0 || isKeyPressed('K') != 0;
	if (kIsDown && !kWasDown && !hero.isAttacking) {
		if (hero.weapon.equipped == WEAPON_AK47) {
			hero.weapon.equip(WEAPON_KNIFE);
		}
		else if (hasAK47Pickup) {
			hero.weapon.equip(WEAPON_AK47);
		}
		else {
			ak47LockedToastTicks = 45;
		}
	}
	kWasDown = kIsDown;

	// Fall state update logic
	bool stillSupported = (currentLevel == 3) ? heroIsSupportedL3()
		: (currentLevel == 2) ? heroIsSupportedL2() : heroIsSupported();
	if (hero.isGrounded && !stillSupported)
		hero.isGrounded = false;

	// Gravity acceleration update
	if (!hero.isGrounded) {
		if (currentLevel == 3)      heroUpdateGravityL3();
		else if (currentLevel == 2) heroUpdateGravityL2();
		else                        heroUpdateGravity();
	}

	if (pistolLockedToastTicks > 0) pistolLockedToastTicks--;
	if (ak47LockedToastTicks > 0) ak47LockedToastTicks--;
	if (flameLockedToastTicks > 0) flameLockedToastTicks--;
	if (plasmaLockedToastTicks > 0) plasmaLockedToastTicks--;
	if (doNotEnterTicks > 0) doNotEnterTicks--;
	if (rewardToastTicks > 0) rewardToastTicks--;
	if (rewardToast2Ticks > 0) rewardToast2Ticks--;
	tickWeaponSfx();
}

// =====================================================================
// DOOR / TERMINAL INTERACTION  ([E] everywhere, exactly like Level 1)
//
// E is EDGE-triggered here, not polled. Holding E used to re-open a terminal
// the instant ESC closed it (the key was still down on the next tick), which
// made a puzzle impossible to leave. One press = one interaction.
// =====================================================================
inline void updateDoorAndPuzzleKeys() {
	static bool eWasDown = false;
	static bool qWasDownDoor = false;

	if (currentPage != PAGE_PLAYING) {
		// Keep the edge state honest while a puzzle page is open, so the key
		// has to be physically released before it can trigger again.
		eWasDown = isKeyPressed('e') || isKeyPressed('E');
		return;
	}
	if (isLevelTransitioning() || gamePaused || gameOverActive) return;

	bool eDown = isKeyPressed('e') || isKeyPressed('E');
	bool qDown = isKeyPressed('q') || isKeyPressed('Q');
	bool ePressed = eDown && !eWasDown;
	bool qPressed = qDown && !qWasDownDoor;
	eWasDown = eDown;
	qWasDownDoor = qDown;

	if (currentLevel == 1) {
		// --- Level 1: two terminals + the exit door ---
		if (ePressed && heroNearTileID(7, INTERACTION_RADIUS)) {
			currentPage = PAGE_PUZZLE1;
			return;
		}
		if (ePressed && heroNearTileID(8, INTERACTION_RADIUS)) {
			currentPage = PAGE_PUZZLE2;
			return;
		}
		if ((ePressed || qPressed) && heroNearTileID(6, INTERACTION_RADIUS) && keysCollected >= 2) {
			startLevel2Transition();
		}
		return;
	}

	// --- Level 2: three image_8 doors ---
	if (heroNearDoorL2(L2_DOOR_SIDEQUEST, INTERACTION_RADIUS))
		doNotEnterTicks = 12; // refreshed every tick he stands there

	if (!ePressed) return;

	// Re-enterable: the solved-flag gate used to lock these forever after
	// one clear. Rewards are boolean flags (hasAK47Pickup etc.), so replaying
	// cannot stack anything - only keysCollected could, and Puzzle3/4 guard
	// that with their own solved flags internally.
	if (heroNearDoorL2(L2_DOOR_PUZZLE_A, INTERACTION_RADIUS)) {
		currentPage = PAGE_PUZZLE3;   // Clear Asteroids -> AK47 + Key
		return;
	}
	if (heroNearDoorL2(L2_DOOR_PUZZLE_B, INTERACTION_RADIUS)) {
		currentPage = PAGE_PUZZLE4;   // Fix Weather Node -> Key + Health
		return;
	}
	if (heroNearDoorL2(L2_DOOR_SIDEQUEST, INTERACTION_RADIUS)) {
		startSideQuest();             // ...he was warned
		return;
	}

	// --- the Titan rift: Level 2's exit portal ---
	// Both Level 2 doors hand over a key, so keysCollected >= 2 is the same
	// as "both puzzles solved" (see portalUnlocked() in Level2.hpp). The
	// locked case is not silent - drawExitPortal() already paints the
	// "RIFT SEALED" prompt whenever he is standing near it.
	if (heroNearPortal() && portalUnlocked()) {
		enterLevel3();
		return;
	}
}

// =====================================================================
// ESC
//   gameplay / side quest -> PAUSE (drops to the menu, RESUME appears)
//   paused on the menu     -> resume exactly where it left off
//   a puzzle terminal      -> close it and go back to gameplay, no reward
// =====================================================================
inline void pauseGame() {
	if (currentPage != PAGE_PLAYING && currentPage != PAGE_SIDEQUEST) return;
	pausedReturnPage = currentPage;
	gamePaused = true;
	stopBGM();
	playMenuBGM();
	currentPage = PAGE_HOME;
}

inline void resumeGame() {
	if (!gamePaused) return;
	gamePaused = false;
	currentPage = pausedReturnPage;
	stopBGM();
	playCurrentLevelBGM();
}

inline void updateEscKey() {
	static bool escWasDown = false;
	bool escIsDown = isKeyPressed(27) != 0;

	if (escIsDown && !escWasDown) {
		// Leaving a puzzle NEVER grants its reward - only solving it does.
		if (currentPage == PAGE_PUZZLE1 || currentPage == PAGE_PUZZLE2 ||
			currentPage == PAGE_PUZZLE3 || currentPage == PAGE_PUZZLE4) {
			currentPage = PAGE_PLAYING;
			escWasDown = escIsDown;
			return;
		}

		// Walking out of the side quest: the flood clock keeps running, the
		// quest can be re-entered from the same door.
		if (currentPage == PAGE_SIDEQUEST) {
			exitSideQuest();
			escWasDown = escIsDown;
			return;
		}

		if (gamePaused && currentPage == PAGE_HOME) {
			resumeGame();
			escWasDown = escIsDown;
			return;
		}

		if (currentPage == PAGE_PLAYING) {
			pauseGame();
			escWasDown = escIsDown;
			return;
		}

		if (currentPage != PAGE_HOME && currentPage != PAGE_COVER)
			currentPage = PAGE_HOME;
	}
	escWasDown = escIsDown;
}

// =====================================================================
// HUD - shared by both levels, with the level 2 extras (flood clock and the
// door prompts) layered on top.
// =====================================================================
inline void drawLevel1HUD() {
	if (currentPage != PAGE_PLAYING) return;

	char line[80];
	iSetColor(255, 220, 60);
	sprintf_s(line, sizeof(line), "KEYS: %d", keysCollected);
	iText(SCREEN_WIDTH - 160, SCREEN_HEIGHT - 40, line, GLUT_BITMAP_HELVETICA_18);

	// REMOVED: the top-right "SCORE: %d" readout - it duplicated the
	// centered "SCORE  %d" plate GameFlow.hpp's drawCenterHUD() already
	// draws every tick, so the score was showing twice on screen.

	const int PROMPT_RADIUS = INTERACTION_RADIUS;

	if (currentLevel == 1) {
		iSetColor(0, 255, 220);
		if (heroNearTileID(7, PROMPT_RADIUS)) {
			iText(SCREEN_WIDTH / 2 - 160, 60, (char*)"[E] ACCESS SECURITY TERMINAL", GLUT_BITMAP_HELVETICA_18);
		}
		else if (heroNearTileID(8, PROMPT_RADIUS)) {
			iText(SCREEN_WIDTH / 2 - 150, 60, (char*)"[E] ACCESS ARMORY LOCKER", GLUT_BITMAP_HELVETICA_18);
		}
		else if (heroNearTileID(6, PROMPT_RADIUS)) {
			if (keysCollected >= 2) {
				iText(SCREEN_WIDTH / 2 - 130, 60, (char*)"[E] BREACH THE Q-SHIP CORE", GLUT_BITMAP_HELVETICA_18);
			}
			else {
				sprintf_s(line, sizeof(line), "DOOR LOCKED - %d MORE KEY(S) NEEDED", 2 - keysCollected);
				iSetColor(220, 60, 60);
				iText(SCREEN_WIDTH / 2 - 170, 60, line, GLUT_BITMAP_HELVETICA_18);
			}
		}
	}
	else {
		// --- Level 2: flood countdown ---
		int secs = floodSecondsLeft();
		bool critical = secs <= 30;
		iSetColor(critical ? 255 : 120, critical ? 70 : 220, critical ? 70 : 255);
		sprintf_s(line, sizeof(line), "FLOOD IN  %d:%02d", secs / 60, secs % 60);
		iText(SCREEN_WIDTH / 2 - 70, SCREEN_HEIGHT - 40, line, GLUT_BITMAP_TIMES_ROMAN_24);

		iSetColor(0, 255, 220);
		if (heroNearDoorL2(L2_DOOR_PUZZLE_A, PROMPT_RADIUS))
			iText(SCREEN_WIDTH / 2 - 160, 60, (char*)"[E] CLEAR ASTEROIDS  (AK47 + KEY)", GLUT_BITMAP_HELVETICA_18);
		else if (heroNearDoorL2(L2_DOOR_PUZZLE_B, PROMPT_RADIUS))
			iText(SCREEN_WIDTH / 2 - 160, 60, (char*)"[E] FIX WEATHER NODE  (FLAMETHROWER + KEY)", GLUT_BITMAP_HELVETICA_18);
		else if (doNotEnterTicks > 0)
			iText(SCREEN_WIDTH / 2 - 120, 60, (char*)"[E] OPEN ANYWAY", GLUT_BITMAP_HELVETICA_18);

		// The side-quest door's warning, pulsing across the top of the screen.
		if (doNotEnterTicks > 0 && !sideQuestComplete) {
			float p = fxPulse();
			fxFilledRectA(0, (float)SCREEN_HEIGHT - 110.0f, (float)SCREEN_WIDTH, 60.0f,
				180, 20, 20, 0.25f + 0.30f * p);
			iSetColor(255, 120 + (int)(100 * p), 120 + (int)(100 * p));
			iText(SCREEN_WIDTH / 2 - 110, SCREEN_HEIGHT - 90, (char*)"DO NOT ENTER", GLUT_BITMAP_TIMES_ROMAN_24);
		}
	}

	if (pistolLockedToastTicks > 0) {
		iSetColor(220, 60, 60);
		iText(SCREEN_WIDTH / 2 - 160, 100, (char*)"PISTOL LOCKED - SOLVE THE CIRCUIT TERMINAL", GLUT_BITMAP_HELVETICA_18);
	}
	if (ak47LockedToastTicks > 0) {
		iSetColor(220, 60, 60);
		iText(SCREEN_WIDTH / 2 - 160, 130, (char*)"AK47 LOCKED - CLEAR THE ASTEROID SCOPE (LEVEL 2)", GLUT_BITMAP_HELVETICA_18);
	}
	if (flameLockedToastTicks > 0) {
		iSetColor(220, 60, 60);
		iText(SCREEN_WIDTH / 2 - 160, 160, (char*)"FLAMETHROWER LOCKED - FIX THE WEATHER NODE (LEVEL 2)", GLUT_BITMAP_HELVETICA_18);
	}
	if (plasmaLockedToastTicks > 0) {
		iSetColor(220, 60, 60);
		iText(SCREEN_WIDTH / 2 - 320, 250, (char*)"PLASMA RIFLE LOCKED - COMPLETE THE LEVEL 2 SIDE QUEST TO CLAIM THE SPACE STONE", GLUT_BITMAP_HELVETICA_18);
	}
	if (rewardToastTicks > 0) {
		float p = fxPulse();
		fxFilledRectA((float)(SCREEN_WIDTH / 2 - 260), 190.0f, 520.0f, 44.0f, 20, 90, 70, 0.45f + 0.15f * p);
		iSetColor(160, 255, 200);
		iText(SCREEN_WIDTH / 2 - 230, 205, (char*)rewardToastText, GLUT_BITMAP_HELVETICA_18);
	}
	// second line, in its own box right under the first (190 - 8 gap - 44 = 138)
	if (rewardToast2Ticks > 0) {
		float p = fxPulse();
		fxFilledRectA((float)(SCREEN_WIDTH / 2 - 260), 138.0f, 520.0f, 44.0f, 20, 90, 70, 0.45f + 0.15f * p);
		iSetColor(160, 255, 200);
		iText(SCREEN_WIDTH / 2 - 230, 153, (char*)rewardToast2Text, GLUT_BITMAP_HELVETICA_18);
	}
}

#endif