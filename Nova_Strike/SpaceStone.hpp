// ===== SpaceStone.hpp =====
#ifndef SPACESTONE_HPP
#define SPACESTONE_HPP

// Space Stone ability - the reward for finishing the Level 2 side quest.
//
// Once hasSpaceStone is true (set in SideQuest.hpp the moment the switch is
// thrown) the player can press 'Q' to TELEPORT 600 px in whichever direction
// the hero is facing. Then the stone recharges for 10 seconds.
//
// This header owns everything about the ability, in three small pieces:
//   1. THE TIMER      spaceStoneCooldownLeftMs - the cooldown count, in ms.
//                     updateSpaceStone() counts it down once per fixed tick.
//   2. THE TELEPORT   spaceStoneTeleport() - where the hero lands.
//   3. THE HUD        drawSpaceStoneHUD() - stone icon + countdown, placed
//                     directly under the top-right weapon readout
//                     (Header/WeaponHUD.hpp).
//
// Wiring (kept to three call sites on purpose):
//   Playercontroller.hpp  -> updateSpaceStone()   inside updatePlayerMovement()
//   iMain.cpp             -> drawSpaceStoneHUD()  inside iDraw()
//   iMain.cpp             -> resetSpaceStone()    wherever hasSpaceStone is
//                                                 cleared for a fresh run
//
// !! INCLUDE ORDER !! Must come AFTER level.hpp, GameFlow.hpp and Level3.hpp
// (it reads the hero, the per-level collision helpers, gameOverActive and
// the stone artwork loaded by GameFlow.hpp). Playercontroller.hpp includes it
// in exactly that position, so nothing else needs to include it.

#include <time.h>
#include <stdio.h>
#include "Variables.h"
#include "level.hpp"
#include "GameFlow.hpp"
#include "Level3.hpp"

// iGraphics' key-poll function (defined once in iGraphics.h) - declared again
// here the same way Playercontroller.hpp does, so this header doesn't care
// what order the includes ended up in.
int isKeyPressed(unsigned char key);

// ------------------------------------------------------------- tuning ------
const int  SPACE_STONE_TELEPORT_DIST = 600;      // px the hero jumps forward
const long SPACE_STONE_COOLDOWN_MS = 10000;    // 10 s before it can be used again

// The timer only counts while the world is actually running (this is only
// ever called from updatePlayerMovement(), which is skipped while paused, in
// a puzzle, on the Game Over screen, or mid level-transition). The first tick
// after any of those gaps would otherwise see a huge time delta and eat the
// cooldown, so a single tick is never allowed to count for more than this.
const long SPACE_STONE_MAX_TICK_MS = 100;

// HUD layout - hangs off the weapon HUD's own constants so the two always
// stay lined up if the weapon readout is ever moved.
const int SPACE_STONE_ICON_SIZE = 56;
const int SPACE_STONE_HUD_GAP_Y = 12;   // gap between the weapon icon and the stone icon

// ------------------------------------------------------------- state -------
// THE TIMER COUNT. Milliseconds left before the stone can be used again;
// 0 means READY. Starts at 0, so the stone is usable the moment it's earned.
__declspec(selectany) long spaceStoneCooldownLeftMs = 0;

__declspec(selectany) long spaceStoneLastTickMs = -1;     // clock() of the previous tick, -1 = none yet
__declspec(selectany) bool spaceStoneQWasDown = false;    // edge detection - one press = one teleport

// HUD icon: Images\UI\SpaceStone.png. If that file is missing it falls back
// to the pickup artwork GameFlow.hpp already loads (Images\Level_2\Space_Stone.png).
__declspec(selectany) unsigned int imgSpaceStoneHUD = (unsigned int)-1;
__declspec(selectany) bool spaceStoneHudArtLoaded = false;

inline void loadSpaceStoneHudArt() {
	if (spaceStoneHudArtLoaded) return;
	spaceStoneHudArtLoaded = true;

	FILE* probe = NULL;
	fopen_s(&probe, "Images\\UI\\SpaceStone.png", "rb");
	if (probe) {
		fclose(probe);
		imgSpaceStoneHUD = iLoadImage((char*)"Images\\UI\\SpaceStone.png");
	}
	else {
		loadSpaceStoneArt();
		imgSpaceStoneHUD = imgSpaceStone;
	}
}

// Back to "ready, nothing pending". Called when a fresh run starts (the same
// place hasSpaceStone itself is cleared).
inline void resetSpaceStone() {
	spaceStoneCooldownLeftMs = 0;
	spaceStoneLastTickMs = -1;
	spaceStoneQWasDown = false;
}

inline long spaceStoneNowMs() {
	return (long)(clock() * 1000L / CLOCKS_PER_SEC);
}

inline bool spaceStoneReady() {
	return hasSpaceStone && spaceStoneCooldownLeftMs <= 0;
}

// -------------------------------------------------------- teleporting ------
// Width of the map the hero is currently walking in. Level 1 and 2 are
// MAX_COLS wide and scroll; Level 3 is a fixed one-screen arena where world
// coordinates and screen coordinates are the same thing. In every case the
// hero's own limits are [0, width - hero.WIDTH] (same clamp
// Hero::moveHorizontal() and heroMoveHorizontalL3() apply), so "the border of
// the screen" and "the edge of the map" are the same line here.
inline int spaceStoneWorldWidth() {
	if (currentLevel == 3) return L3_ARENA_WIDTH;
	if (currentLevel == 2) return L2_MAX_COLS * TILE_SIZE;
	return MAX_COLS * TILE_SIZE;
}

// Would the hero's box overlap a WALL / solid tile if his left edge were at x
// (same Y he has now)? Routed to whichever map is loaded, using the same
// terrain tests that map's own heroMoveHorizontal*() uses. Enemies are
// deliberately NOT part of this - the teleport passes straight through them.
inline bool spaceStoneWallAt(int x, int y) {
	if (currentLevel == 3) return isSolidAtL3(x, y, hero.WIDTH, hero.HEIGHT);
	if (currentLevel == 2) return solidOrMoverL2(x, y, hero.WIDTH, hero.HEIGHT);
	return overlapsSolidTile(x, y, hero.WIDTH, hero.HEIGHT);
}

// Thanos is the one body that still matters as a LANDING spot: the arena
// movement code (heroMoveHorizontalL3) reverts every step that overlaps him,
// so a hero who ended up inside him could never walk out again. The teleport
// may cross him, it just may not finish inside him. Regular enemies never
// block the hero's movement in Level 2, so they need no such check.
inline bool spaceStoneThanosAt(int x, int y) {
	if (currentLevel != 3) return false;
	return thanosBlocksHero(x, y, hero.WIDTH, hero.HEIGHT);
}

// Moves the hero SPACE_STONE_TELEPORT_DIST px the way he's facing.
//   - Past the edge of the map/screen  -> he lands on the border instead.
//   - Enemies and Thanos                -> passed straight through. If the
//     600 px mark falls inside Thanos he lands just past him instead; only
//     if there is no room on his far side (Thanos against the border) does
//     he land just short of him.
//   - Target is inside a wall           -> he lands flush against the wall,
//     never inside it.
// Returns true if he actually moved. When he can't move at all (already on
// the border facing out, or boxed in against a wall) it returns false and the
// caller does NOT start the cooldown - a press that did nothing shouldn't
// cost ten seconds.
inline bool spaceStoneTeleport() {
	int dir = (hero.facing >= 0) ? 1 : -1;
	int maxX = spaceStoneWorldWidth() - hero.WIDTH;

	// Border clamp. hero.posX is always inside [0, maxX], so after this the
	// target is at or beyond posX in the facing direction, never behind it.
	int target = hero.posX + dir * SPACE_STONE_TELEPORT_DIST;
	if (target < 0) target = 0;
	if (target > maxX) target = maxX;

	// 1) Farthest wall-free spot, searching back from the target.
	int x = target;
	while (x != hero.posX && spaceStoneWallAt(x, hero.posY))
		x -= dir;
	if (x == hero.posX) return false;

	// 2) That spot is inside Thanos: come out on his far side...
	if (spaceStoneThanosAt(x, hero.posY)) {
		int f = x;
		bool found = false;
		while (f >= 0 && f <= maxX) {
			if (!spaceStoneWallAt(f, hero.posY) && !spaceStoneThanosAt(f, hero.posY)) { found = true; break; }
			f += dir;
		}

		// ...or, if he's against the border and there is no far side, stop
		// just short of him.
		if (!found) {
			f = x;
			while (f != hero.posX) {
				if (!spaceStoneWallAt(f, hero.posY) && !spaceStoneThanosAt(f, hero.posY)) { found = true; break; }
				f -= dir;
			}
		}
		if (!found) return false;
		x = f;
	}

	if (x == hero.posX) return false;

	hero.posX = x;
	return true;
}

// ---------------------------------------------------------- per tick -------
// Call once per fixed tick, from updatePlayerMovement() (so it is already
// skipped while paused / dead / in a puzzle / mid-transition).
inline void updateSpaceStone() {
	if (!hasSpaceStone) {
		spaceStoneLastTickMs = -1;
		spaceStoneQWasDown = false;
		return;
	}

	// ---- count the cooldown down ----
	long now = spaceStoneNowMs();
	if (spaceStoneLastTickMs < 0) spaceStoneLastTickMs = now;
	long dt = now - spaceStoneLastTickMs;
	spaceStoneLastTickMs = now;
	if (dt < 0) dt = 0;
	if (dt > SPACE_STONE_MAX_TICK_MS) dt = SPACE_STONE_MAX_TICK_MS;

	if (spaceStoneCooldownLeftMs > 0) {
		spaceStoneCooldownLeftMs -= dt;
		if (spaceStoneCooldownLeftMs < 0) spaceStoneCooldownLeftMs = 0;
	}

	// ---- 'Q' pressed? (edge-triggered: holding it is one teleport) ----
	bool qDown = isKeyPressed('q') || isKeyPressed('Q');
	bool qPressed = qDown && !spaceStoneQWasDown;
	spaceStoneQWasDown = qDown;

	if (qPressed && spaceStoneCooldownLeftMs <= 0) {
		if (spaceStoneTeleport())
			spaceStoneCooldownLeftMs = SPACE_STONE_COOLDOWN_MS;
	}
}

// -------------------------------------------------------------- HUD --------
// Small translucent quad - same blend setup drawSpaceStoneToast() uses.
inline void spaceStoneShade(float x, float y, float w, float h, float r, float g, float b, float a) {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(r, g, b, a);
	glBegin(GL_QUADS);
	glVertex2f(x, y);
	glVertex2f(x + w, y);
	glVertex2f(x + w, y + h);
	glVertex2f(x, y + h);
	glEnd();
	glDisable(GL_BLEND);
}

// Stone icon directly below the weapon readout, right-aligned to the same
// column, with the cooldown beside it (same icon-then-label shape as the
// weapon HUD): the seconds left while recharging, "READY" once it's usable.
// A slim bar under the label fills up as the stone recharges.
//
// Drawn from iDraw() AFTER the level, so it sits on top of Level 2's darkness
// overlay the same way the weapon HUD does. Nothing is drawn until the stone
// has been earned, and only while a level is actually on screen (not the side
// quest, a puzzle terminal, the menu or the Game Over screen).
inline void drawSpaceStoneHUD() {
	if (!hasSpaceStone) return;
	if (currentPage != PAGE_PLAYING && currentPage != PAGE_LEVEL3) return;
	if (gameOverActive) return;

	loadSpaceStoneHudArt();

	int weaponIconY = SCREEN_HEIGHT - WEAPON_HUD_MARGIN_Y - WEAPON_ICON_SIZE;
	int iconX = SCREEN_WIDTH - WEAPON_HUD_MARGIN_X - WEAPON_HUD_TEXT_WIDTH - WEAPON_HUD_TEXT_GAP - WEAPON_ICON_SIZE;
	int iconY = weaponIconY - SPACE_STONE_HUD_GAP_Y - SPACE_STONE_ICON_SIZE;
	int textX = iconX + SPACE_STONE_ICON_SIZE + WEAPON_HUD_TEXT_GAP;
	int textCenterY = iconY + SPACE_STONE_ICON_SIZE / 2;

	bool ready = spaceStoneCooldownLeftMs <= 0;

	// ---- icon ----
	if (imgSpaceStoneHUD != (unsigned int)-1) {
		iShowImage(iconX, iconY, SPACE_STONE_ICON_SIZE, SPACE_STONE_ICON_SIZE, imgSpaceStoneHUD);
	}
	else {
		// artwork missing - a plain violet orb so the HUD is still readable
		iSetColor(150, 90, 255);
		iFilledCircle(iconX + SPACE_STONE_ICON_SIZE / 2, textCenterY, SPACE_STONE_ICON_SIZE / 2 - 2, 32);
	}

	// dim the stone while it recharges
	if (!ready)
		spaceStoneShade((float)iconX, (float)iconY, (float)SPACE_STONE_ICON_SIZE, (float)SPACE_STONE_ICON_SIZE,
			0.0f, 0.0f, 0.0f, 0.55f);

	// key hint in the icon's corner
	iSetColor(255, 255, 255);
	iText(iconX + 4, iconY + 4, (char*)"Q", GLUT_BITMAP_HELVETICA_12);

	// ---- label ----
	char label[16];
	if (ready) {
		sprintf_s(label, sizeof(label), "READY");
		iSetColor(120, 230, 255);
	}
	else {
		int secsLeft = (int)((spaceStoneCooldownLeftMs + 999) / 1000);   // round up: 10 .. 1
		sprintf_s(label, sizeof(label), "%d s", secsLeft);
		iSetColor(255, 255, 255);
	}
	iText(textX, textCenterY - 2, label, GLUT_BITMAP_HELVETICA_18);

	// ---- recharge bar ----
	float frac = 1.0f - (float)spaceStoneCooldownLeftMs / (float)SPACE_STONE_COOLDOWN_MS;
	if (frac < 0.0f) frac = 0.0f;
	if (frac > 1.0f) frac = 1.0f;

	int barW = WEAPON_HUD_TEXT_WIDTH - 10;
	int barY = textCenterY - 16;
	iSetColor(40, 40, 60);
	iFilledRectangle(textX, barY, barW, 6);
	if (ready) iSetColor(120, 230, 255);
	else       iSetColor(150, 110, 255);
	iFilledRectangle(textX, barY, barW * frac, 6);
}

#endif
