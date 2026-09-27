// =====================================================
// Level2.hpp
// Owner: Mahdin (Map Design)
//
// Level 2 - "The Q-Ship Core"
// Own grid, own tile meanings, own draw pass. currentLevel (Variables.h)
// decides whether iMain draws this or Level 1.
//
// TILE ID MEANINGS (Level 2 ONLY):
//   0  = empty space (parallax background shows through)
//   1  = image_2.png (Purple Tech Structure) -> MAP BOUNDARY, solid wall
//   2  = image_4.png (Broken Debris/Rocks)   -> solid obstacle / floor cover
//   5  = image_5.png (Plasma Acid)           -> NOT solid; -10 HP per full
//        second the hero's box overlaps it (continuous, not a one-shot hit)
//   6  = image_6.png (Energy Vent/Jump Pad)  -> solid pad; launches the hero
//        straight up (velocityY = +32) the instant he lands on it
//   7  = (not a grid tile) Puzzle Terminal - image_7.png drawn as a pure
//        decorative overlay at THREE fixed world spots (see
//        puzzleTerminalSpots below). No collision, no "press E" logic yet -
//        puzzle function is being built later, per instruction. Reward plan
//        for when that's wired up: terminal 1 -> AK47, terminal 2 ->
//        Flamethrower (these two sit close together), terminal 3 -> Power
//        Stone (the level's win item, sits further off).
//   8  = image_8.jpg (Boss Door)             -> solid; touching it wins Lv2
//   11 = block_1.png (platform left end-cap) -> solid
//   12 = block_2.png (platform middle, repeat to stretch any width) -> solid
//   13 = block_3.png (platform right end-cap)-> solid
//
// INTEGRATION NOTE for whoever owns WeaponControl.hpp / Grenade.hpp: bullets
// and thrown grenades need to bounce/stop off the same solid tiles the hero
// does. overlapsSolidTileL2(x, y, w, h) below is the function to call for
// that (mirrors the Level 1 pattern) - it isn't wired into the weapon code
// from this file since that system isn't owned here.
// =====================================================
#ifndef LEVEL2_HPP
#define LEVEL2_HPP
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include "Variables.h"
#include "Header\Hero.hpp"

#define L2_MAX_ROWS 42
#define L2_MAX_COLS 64
// TILE_SIZE (40) already #defined by level.hpp before this file is included.

// levelGrid2[][] itself is declared in level.hpp (right under levelGrid[][]),
// NOT here - Header/enemy.h has to read it for the Level 2 enemy spawner, and
// enemy.h is included long before this file. Same size as this file's
// L2_MAX_ROWS x L2_MAX_COLS (both 42 x 64).

// --- Level 2 art assets ---
__declspec(selectany) unsigned int imgL2Background = (unsigned int)-1; // image_1.jpg
__declspec(selectany) unsigned int imgL2Boundary = (unsigned int)-1; // image_2.png (tile 1)
__declspec(selectany) unsigned int imgL2Debris = (unsigned int)-1; // image_4.png (tile 2)
__declspec(selectany) unsigned int imgL2Plasma = (unsigned int)-1; // image_5.png (tile 5)
__declspec(selectany) unsigned int imgL2JumpPad = (unsigned int)-1; // image_6.png (tile 6)
__declspec(selectany) unsigned int imgL2Terminal = (unsigned int)-1; // image_7.png (overlay)
__declspec(selectany) unsigned int imgL2BossDoor = (unsigned int)-1; // image_8.jpg (tile 8)
__declspec(selectany) unsigned int imgL2PlatformLeft = (unsigned int)-1; // block_1.png (tile 11)
__declspec(selectany) unsigned int imgL2PlatformMid = (unsigned int)-1; // block_2.png (tile 12)
__declspec(selectany) unsigned int imgL2PlatformRight = (unsigned int)-1; // block_3.png (tile 13)
__declspec(selectany) unsigned int imgL2GroundLeft = (unsigned int)-1; // ground_1.png (tile 21)
__declspec(selectany) unsigned int imgL2GroundMid = (unsigned int)-1; // ground_2.png (tile 22)
__declspec(selectany) unsigned int imgL2GroundRight = (unsigned int)-1; // ground_3.png (tile 23)

#define VENT_LAUNCH_SPEED 32
#define PLASMA_DAMAGE_PER_TICK 10
#define PLASMA_TICK_INTERVAL 33 // ~1 second at a 30ms fixedUpdate rate

// Three fixed world-space spots where the puzzle terminal art is drawn on
// top of a platform - purely decorative for now, no tile ID, no collision,
// no "press E" logic yet. Terminals 1+2 sit close together on purpose.
// ===== PUZZLE DOORS (tiles 41 / 42 / 43) =====
// These replace the old decorative "puzzleTerminalSpots" consoles, which were
// drawn floating in mid-air with no collision and no interaction - which is
// exactly why they could be jumped straight through as if they weren't there.
// Each door is now a real grid tile, drawn 2x2 from image_8.jpg with the
// crystal console (image_7.png) sitting on top of it, and each one answers
// to [E] at close range, exactly like the Level 1 doors:
//
//   41 -> Clear Asteroids   (PAGE_PUZZLE3) -> AK47 + Key
//   42 -> Fix Weather Node  (PAGE_PUZZLE4) -> Key + Health
//   43 -> Side quest        (PAGE_SIDEQUEST) - shows "DO NOT ENTER" first
//
// They are deliberately NOT solid: the hero walks up to the face of the door
// and presses E, he doesn't climb on it.
#define L2_DOOR_PUZZLE_A 41
#define L2_DOOR_PUZZLE_B 42
#define L2_DOOR_SIDEQUEST 43
#define L2_DOOR_TILES_W 2
#define L2_DOOR_TILES_H 2

// iGraphics's iLoadImage() has no failure detection of its own - a missing
// or bad file still returns a "valid" texture handle bound to empty GPU
// memory. We check the file actually opens BEFORE calling iLoadImage, so
// the "!= (unsigned int)-1" fallback-color checks below mean something.
inline unsigned int safeLoadImage(const char* path) {
	FILE* probe = NULL;
	fopen_s(&probe, path, "rb");
	if (!probe) return (unsigned int)-1;
	fclose(probe);
	return iLoadImage((char*)path);
}

inline void initLevel2Assets() {
	imgL2Background = safeLoadImage("Images\\Level_2\\image_1.jpg");
	imgL2Boundary = safeLoadImage("Images\\Level_2\\image_2.png");
	imgL2Debris = safeLoadImage("Images\\Level_2\\image_4.png");
	imgL2Plasma = safeLoadImage("Images\\Level_2\\image_5.png");
	imgL2JumpPad = safeLoadImage("Images\\Level_2\\image_6.png");
	imgL2Terminal = safeLoadImage("Images\\Level_2\\image_7.png");
	imgL2BossDoor = safeLoadImage("Images\\Level_2\\image_8.jpg");
	imgL2PlatformLeft = safeLoadImage("Images\\Level_2\\block_1.png");
	imgL2PlatformMid = safeLoadImage("Images\\Level_2\\block_2.png");
	imgL2PlatformRight = safeLoadImage("Images\\Level_2\\block_3.png");
	imgL2GroundLeft = safeLoadImage("Images\\Level_2\\ground_1.png");
	imgL2GroundMid = safeLoadImage("Images\\Level_2\\ground_2.png");
	imgL2GroundRight = safeLoadImage("Images\\Level_2\\ground_3.png");
}

inline void loadLevel2FromFile(const char* filename) {
	FILE* file = NULL;
	errno_t err = fopen_s(&file, filename, "r");

	if (err != 0 || !file) {
		for (int r = 0; r < L2_MAX_ROWS; r++)
		for (int c = 0; c < L2_MAX_COLS; c++)
			levelGrid2[r][c] = (r == 0 || r == L2_MAX_ROWS - 1 || c == 0 || c == L2_MAX_COLS - 1) ? 1 : 0;
		return;
	}

	for (int r = 0; r < L2_MAX_ROWS; r++)
	for (int c = 0; c < L2_MAX_COLS; c++)
	if (fscanf_s(file, "%d", &levelGrid2[r][c]) != 1)
		levelGrid2[r][c] = 0;

	fclose(file);
}

inline void gridCellRect2(int row, int col, float& x, float& y) {
	x = (float)(col * TILE_SIZE);
	y = (float)((L2_MAX_ROWS - 1 - row) * TILE_SIZE);
}

// 1 (boundary), 2 (debris), 6 (jump pad), 8 (boss door), 11/12/13 (platform
// chain) are all solid. 5 (plasma) is deliberately NOT solid - the hero
// sinks through it and takes damage over time rather than standing on it.

// =====================================================================
// LEVEL 2 DYNAMIC HAZARDS (added)
//
// New tile IDs, all Level 2 only:
//   31 = moving platform, HORIZONTAL oscillation  (solid, hero rides it)
//   32 = moving platform, VERTICAL oscillation    (solid, hero rides it)
//   33 = gravity-flip field  (NOT solid; flips gravityDir while inside)
//   34 = strobing laser gate (solid <-> passable on a timer)
//   35 = darkness field      (NOT solid; blacks the screen out except a
//                             lit circle around the hero)
//
// 31/32 are lifted OUT of levelGrid2 at load time and become objects in
// movingPlats[] - a moving tile can't live in a fixed grid cell. Every
// solid test (gravity, landing, horizontal blocking) routes through
// overlapsSolidTileL2(), so adding the platform sweep in there is all it
// takes for them to behave like real ground.
// =====================================================================
#define MAX_MOVING_PLATS 24

struct MovingPlatL2 {
	float x, y;          // live world position
	float baseX, baseY;  // oscillation centre (where the tile was authored)
	float amp;           // half-travel in px
	float speed;         // radians per tick
	float phase;         // per-platform offset so they don't move in lockstep
	float dx, dy;        // movement applied THIS tick (used to carry the hero)
	int axis;            // 0 = horizontal, 1 = vertical
	bool active;
};

__declspec(selectany) MovingPlatL2 movingPlats[MAX_MOVING_PLATS];
__declspec(selectany) int movingPlatCount = 0;
__declspec(selectany) float movePlatClock = 0.0f;
__declspec(selectany) float platCarryResX = 0.0f; // sub-pixel carry remainder
__declspec(selectany) float platCarryResY = 0.0f;

// ---- Strobing laser gate (tile 34) ----
#define STROBE_PERIOD_TICKS 150
#define STROBE_SOLID_TICKS  90   // solid for 90, open for 60
__declspec(selectany) int strobeTick = 0;
__declspec(selectany) bool strobeSolid = true;

// ---- Gravity-flip field (tile 33) ----
__declspec(selectany) int gravFlipFxTicks = 0;

// ---- Darkness field (tile 35) ----
__declspec(selectany) float darkAmount = 0.0f; // 0 = lit, 1 = fully dark

// 0..1 breathing value shared by every plasma/energy visual. pulseTimer is
// advanced 0.05 per 30ms by updateFX() in iMain.cpp, so this completes a
// cycle in a little over 2 seconds - a breath, not a flicker.
inline float fxPulse() {
	return 0.5f + 0.5f * (float)sin(pulseTimer * 1.6f);
}

inline bool overlapsMovingPlatL2(int x, int y, int w, int h) {
	for (int i = 0; i < movingPlatCount; i++) {
		if (!movingPlats[i].active) continue;
		if (rectsOverlap(x, y, w, h, (int)movingPlats[i].x, (int)movingPlats[i].y,
			TILE_SIZE, TILE_SIZE))
			return true;
	}
	return false;
}

inline bool isDoorTileL2(int id) {
	return id == L2_DOOR_PUZZLE_A || id == L2_DOOR_PUZZLE_B || id == L2_DOOR_SIDEQUEST;
}

// Doors occupy a 2x2 block, so their hit test needs its own size instead of
// overlapsTileIdL2()'s single-tile assumption.
inline bool overlapsDoorL2(int x, int y, int w, int h, int doorId) {
	for (int r = 0; r < L2_MAX_ROWS; r++) {
		for (int c = 0; c < L2_MAX_COLS; c++) {
			if (levelGrid2[r][c] != doorId) continue;
			float tx, ty;
			gridCellRect2(r, c, tx, ty);
			if (rectsOverlap(x, y, w, h, (int)tx, (int)ty,
				TILE_SIZE * L2_DOOR_TILES_W, TILE_SIZE * L2_DOOR_TILES_H))
				return true;
		}
	}
	return false;
}

inline bool heroNearDoorL2(int doorId, int radiusPx) {
	return overlapsDoorL2(hero.posX - radiusPx, hero.posY - radiusPx,
		hero.WIDTH + radiusPx * 2, hero.HEIGHT + radiusPx * 2, doorId);
}

inline bool isSolidTileL2(int id) {
	// Tile 34 (laser gate) is solid only on the half of its cycle where the
	// beam is up - that single condition is what makes the timing puzzle
	// work for the hero, bullets and grenades all at once.
	if (id == 34) return strobeSolid;
	return id == 1 || id == 2 || id == 6 || id == 8 || id == 11 || id == 12 || id == 13
		|| id == 21 || id == 22 || id == 23;
}

inline bool overlapsSolidTileL2(int x, int y, int w, int h) {
	if (overlapsMovingPlatL2(x, y, w, h)) return true;

	for (int r = 0; r < L2_MAX_ROWS; r++) {
		for (int c = 0; c < L2_MAX_COLS; c++) {
			if (!isSolidTileL2(levelGrid2[r][c])) continue;
			float tileX, tileY;
			gridCellRect2(r, c, tileX, tileY);
			if (rectsOverlap(x, y, w, h, (int)tileX, (int)tileY, TILE_SIZE, TILE_SIZE))
				return true;
		}
	}
	return false;
}

inline bool overlapsTileIdL2(int x, int y, int w, int h, int tileId) {
	for (int r = 0; r < L2_MAX_ROWS; r++) {
		for (int c = 0; c < L2_MAX_COLS; c++) {
			if (levelGrid2[r][c] != tileId) continue;
			float tileX, tileY;
			gridCellRect2(r, c, tileX, tileY);
			if (rectsOverlap(x, y, w, h, (int)tileX, (int)tileY, TILE_SIZE, TILE_SIZE))
				return true;
		}
	}
	return false;
}

// Grounded checks use a narrower, CENTERED band of the hero's hitbox
// (60% of his width) instead of the full width. Using the full width let
// the hero clip just the corner of a platform - a sliver of his hitbox
// overlapping the block's edge while the rest of him hung in open air -
// and still register as "grounded", freezing gravity and making him look
// stuck floating beside the block instead of falling past it. A tighter,
// centered check means he has to genuinely be OVER the platform, not just
// brushing its corner, before gravity lets him rest.
#define GROUNDED_CHECK_WIDTH_FRACTION 0.6f

// =====================================================================
// LEVEL 3 EXIT PORTAL
//
// A tall ellipse on the far-right platform. Locked until both puzzles are
// solved AND both keys are held; [E] near an unlocked portal jumps to the
// Titan arena.
// =====================================================================
// Sits on the top-right platform (row 7, cols 57-62 in level2.txt), whose
// top face is world Y 1400. This is the highest, furthest-right ledge in
// the map - the natural "end of the climb" spot. Previously it was parked
// on the ground floor at Y=44, which is why it was never found: the player
// reaches the exit by climbing, not by walking back along the bottom.
#define PORTAL_WORLD_X 2360.0f
#define PORTAL_WORLD_Y 1400.0f
#define PORTAL_W 96.0f
#define PORTAL_H 168.0f
#define PORTAL_INTERACT_RADIUS 90

// Both Level 2 puzzle doors hand over one key each, so "both puzzles solved"
// and "both keys collected" are the same condition. Gating on keysCollected
// keeps this header independent of Puzzle3/4.hpp, which are included after
// level.hpp (and therefore after this file) in iMain.cpp - referencing
// pz3Solved/pz4Solved directly from here would not compile.
inline bool portalUnlocked() {
	return keysCollected >= 2;
}

inline bool heroNearPortal() {
	return rectsOverlap(hero.posX - PORTAL_INTERACT_RADIUS, hero.posY - PORTAL_INTERACT_RADIUS,
		hero.WIDTH + PORTAL_INTERACT_RADIUS * 2, hero.HEIGHT + PORTAL_INTERACT_RADIUS * 2,
		(int)PORTAL_WORLD_X, (int)PORTAL_WORLD_Y, (int)PORTAL_W, (int)PORTAL_H);
}

// Drawn from iGraphics primitives - concentric ellipses whose radii and
// alpha ride the pulse, so it breathes. Locked = dull red and static,
// unlocked = bright blue and swirling.
inline void drawExitPortal() {
	float sx = PORTAL_WORLD_X - cameraX;
	float sy = PORTAL_WORLD_Y - cameraY;
	if (sx + PORTAL_W < -100 || sx > SCREEN_WIDTH + 100) return;

	float cx = sx + PORTAL_W * 0.5f;
	float cy = sy + PORTAL_H * 0.5f;
	float p = fxPulse();
	bool open = portalUnlocked();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Outer halo -> inner core, 7 rings. Y radius stays much larger than X
	// throughout, which is what gives the tall doorway silhouette.
	for (int ring = 6; ring >= 0; ring--) {
		float f = (float)ring / 6.0f;
		float rx = (PORTAL_W * 0.5f) * (0.35f + 0.65f * f) * (1.0f + 0.05f * p);
		float ry = (PORTAL_H * 0.5f) * (0.35f + 0.65f * f) * (1.0f + 0.05f * p);

		float a = open ? (0.10f + 0.13f * (1.0f - f)) : (0.06f + 0.07f * (1.0f - f));
		if (open) glColor4f(0.25f + 0.35f * (1.0f - f), 0.55f + 0.35f * (1.0f - f), 1.0f, a);
		else      glColor4f(0.45f, 0.10f, 0.12f, a);

		glBegin(GL_TRIANGLE_FAN);
		glVertex2f(cx, cy);
		for (int i = 0; i <= 40; i++) {
			float ang = (float)i / 40.0f * 6.2831853f;
			glVertex2f(cx + rx * (float)cos(ang), cy + ry * (float)sin(ang));
		}
		glEnd();
	}

	// Rim
	if (open) glColor4f(0.60f, 0.85f, 1.0f, 0.75f + 0.25f * p);
	else      glColor4f(0.70f, 0.25f, 0.25f, 0.55f);
	glLineWidth(2.5f);
	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < 48; i++) {
		float ang = (float)i / 48.0f * 6.2831853f;
		glVertex2f(cx + (PORTAL_W * 0.5f) * (float)cos(ang),
			cy + (PORTAL_H * 0.5f) * (float)sin(ang));
	}
	glEnd();
	glLineWidth(1.0f);

	glDisable(GL_BLEND);

	// Prompt, only when he's close enough to act on it.
	if (heroNearPortal()) {
		if (open) {
			iSetColor(120, 220, 255);
			iText((int)(cx - 105), (int)(sy + PORTAL_H + 16),
				(char*)"[E] ENTER THE TITAN RIFT", GLUT_BITMAP_HELVETICA_18);
		}
		else {
			iSetColor(230, 80, 80);
			iText((int)(cx - 135), (int)(sy + PORTAL_H + 16),
				(char*)"RIFT SEALED - SOLVE BOTH NODES, TAKE BOTH KEYS", GLUT_BITMAP_HELVETICA_18);
		}
	}
}

// Movers count as ground too - overlapsSolidTileL2 only knows about the
// static grid, so both checks are OR'd everywhere the hero's footing is
// tested.
inline bool solidOrMoverL2(int x, int y, int w, int h) {
	return overlapsSolidTileL2(x, y, w, h) || overlapsMovingPlatL2(x, y, w, h);
}

inline bool heroIsSupportedL2() {
	int narrowW = (int)(hero.WIDTH * GROUNDED_CHECK_WIDTH_FRACTION);
	int narrowX = hero.posX + (hero.WIDTH - narrowW) / 2;
	// With gravity flipped, "the ground" is whatever is above his head.
	if (gravityDir > 0)
		return solidOrMoverL2(narrowX, hero.posY - 1, narrowW, 1);
	return solidOrMoverL2(narrowX, hero.posY + hero.HEIGHT, narrowW, 1);
}

// Gravity, landing and head-bumps all written in terms of gravityDir, so
// the exact same code walks on floors (gravityDir = +1) and on ceilings
// (gravityDir = -1). velocityY stays "positive = upward" in both cases;
// only which way it is pulled, and which surface counts as ground, change.
inline void heroUpdateGravityL2() {
	hero.posY += hero.velocityY;

	// Push back out of anything we moved into, along the travel direction.
	if (solidOrMoverL2(hero.posX, hero.posY, hero.WIDTH, hero.HEIGHT)) {
		int step = (hero.velocityY > 0) ? -1 : 1;
		int guard = 0;
		while (solidOrMoverL2(hero.posX, hero.posY, hero.WIDTH, hero.HEIGHT) && guard++ < 200)
			hero.posY += step;

		bool landed = (hero.velocityY * gravityDir <= 0); // moving WITH gravity = a landing
		hero.velocityY = 0;
		if (landed) hero.isGrounded = true;
	}

	hero.velocityY -= GRAVITY * gravityDir;

	if (hero.velocityY * gravityDir <= 0 && heroIsSupportedL2()) {
		int guard = 0;
		while (solidOrMoverL2(hero.posX, hero.posY, hero.WIDTH, hero.HEIGHT) && guard++ < 200)
			hero.posY += (gravityDir > 0) ? 1 : -1;
		hero.velocityY = 0;
		hero.isGrounded = true;
	}

	if (hero.posY < 0) {
		hero.posY = 0;
		hero.velocityY = 0;
		hero.isGrounded = true;
	}
}

// Level 2 jump: same rules as Hero::startJump(), but launches AWAY from
// whatever surface gravity is currently holding him to.
inline void heroStartJumpL2() {
	if (!hero.isGrounded) return;
	if (hero.isAttacking && hero.weapon.isMelee()) return;
	hero.isGrounded = false;
	hero.velocityY = JUMP_SPEED * gravityDir;
}

inline void heroMoveHorizontalL2(int dir) {
	int prevX = hero.posX;
	hero.moveHorizontal(dir, L2_MAX_COLS * TILE_SIZE);
	// overlapsEnemy() (level.hpp) - same "can't walk through a live enemy"
	// rule Level 1's heroMoveHorizontal() has; now that Level 2 has enemies
	// of its own, it needs it too.
	if (solidOrMoverL2(hero.posX, hero.posY, hero.WIDTH, hero.HEIGHT) ||
		overlapsEnemy(hero.posX, hero.posY, hero.WIDTH, hero.HEIGHT))
		hero.posX = prevX;
}

#define L2_SPAWN_COL 5
#define L2_SPAWN_ROW (L2_MAX_ROWS - 5) // one tile above the new ground floor row

inline void respawnHeroLevel2() {
	gravityDir = 1;
	gravFlipFxTicks = 0;
	hero.posX = L2_SPAWN_COL * TILE_SIZE;
	hero.posY = (L2_MAX_ROWS - 1 - L2_SPAWN_ROW) * TILE_SIZE;
	hero.velocityY = 0;
	hero.isGrounded = true;
}

// ===== Rising Plasma Flood - 4 MINUTE GLOBAL TIMER =====
// The flood is no longer a per-tick nudge (which drifted with frame rate and
// took ~5.5 minutes to cover the map). It is driven by one authoritative
// clock: levelClockTicks. The surface is derived from that clock, so the map
// is fully flooded at exactly FLOOD_TOTAL_SECONDS.
//
// The clock only advances while the world is actually running - see
// fixedUpdate() in iMain.cpp. Opening a puzzle terminal, entering the side
// quest's own paused views, or pausing the game stops it dead, which is what
// "entering any puzzle view pauses all level timers" means in practice.
#define FLOOD_TOTAL_SECONDS 240   // 4 minutes, matches Level 1 oxygen clock
#define TICKS_PER_SECOND 33                                   // 30ms fixedUpdate
#define FLOOD_TOTAL_TICKS (FLOOD_TOTAL_SECONDS * TICKS_PER_SECOND)

__declspec(selectany) float plasmaSurfaceY = 0.0f; // current world Y of the flood's top
__declspec(selectany) int levelClockTicks = 0;     // THE level 2 clock

inline float floodProgress() {
	float p = (float)levelClockTicks / (float)FLOOD_TOTAL_TICKS;
	if (p < 0.0f) p = 0.0f;
	if (p > 1.0f) p = 1.0f;
	return p;
}

inline int floodSecondsLeft() {
	int left = (FLOOD_TOTAL_TICKS - levelClockTicks) / TICKS_PER_SECOND;
	return left < 0 ? 0 : left;
}

inline void resetPlasmaRise() {
	levelClockTicks = 0;
	plasmaSurfaceY = (float)TILE_SIZE; // starts right at the floor row
}

// One call per fixed tick while the level is live.
inline void updateLevel2Clock() {
	if (levelClockTicks < FLOOD_TOTAL_TICKS) levelClockTicks++;

	float top = (float)(L2_MAX_ROWS * TILE_SIZE);
	plasmaSurfaceY = (float)TILE_SIZE + floodProgress() * (top - (float)TILE_SIZE);
}

// hero.posY is the BOTTOM of his hitbox - below the flood surface means
// he's standing/sinking in it.
inline bool heroIsInRisingPlasma() {
	return hero.posY < plasmaSurfaceY;
}

// =====================================================================
// TOXIC GAS FLOOD (replaces the old purple plasma/liquid look)
//
// The hazard is now a dense ash-gray chemical fog rather than a liquid.
// Liquids read as a flat colored rectangle with a hard surface line; gas
// has to look SOFT and VOLUMETRIC or it just looks like a gray box. Four
// things sell it, layered back to front:
//
//   1. BODY        - an opaque-ish base that gets denser toward the floor
//                    (heavy gas sinks), built from stacked bands rather
//                    than one quad so the density gradient is visible.
//   2. SURFACE     - the top edge is NOT a straight line. It is a strip of
//                    quads whose individual heights ride two sine waves at
//                    different frequencies/speeds, so the boundary rolls
//                    and churns instead of sitting flat.
//   3. WISPS       - slow translucent blobs drifting sideways just under
//                    the surface, at a different speed from the surface
//                    itself, which is what creates the illusion of
//                    internal motion rather than a moving solid.
//   4. HAZE        - a faint full-width wash just ABOVE the surface, so the
//                    gas appears to bleed into clear air instead of
//                    stopping dead at a line.
//
// All of it is procedural - no new art assets needed.
// =====================================================================
#define GAS_BAND_COUNT 14
#define GAS_SURFACE_SEGMENTS 48
#define GAS_WISP_COUNT 7

// Ash-gray, very slightly green (chemical rather than smoke).
#define GAS_R 128.0f
#define GAS_G 132.0f
#define GAS_B 124.0f

// Toxicity drives opacity: early gas is thin and survivable-looking, late
// gas is a near-solid wall. Reads directly off the level clock so the
// visual can never disagree with the HUD's toxicity percentage.
inline float gasDensity() {
	return 0.55f + 0.40f * floodProgress();
}

inline void drawToxicGas() {
	float screenTop = plasmaSurfaceY - cameraY;
	if (screenTop <= 0) return;                 // hasn't risen into view yet
	if (screenTop > SCREEN_HEIGHT + 80.0f) screenTop = (float)SCREEN_HEIGHT + 80.0f;

	float t = pulseTimer;
	float density = gasDensity();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// ---- 1. BODY: stacked bands, densest at the floor ----
	for (int i = 0; i < GAS_BAND_COUNT; i++) {
		float f0 = (float)i / (float)GAS_BAND_COUNT;
		float f1 = (float)(i + 1) / (float)GAS_BAND_COUNT;
		float y0 = screenTop * f0;
		float y1 = screenTop * f1;

		// Opacity falls off toward the top of the cloud.
		float a = density * (1.0f - f0 * 0.55f);

		// Each band breathes on its own phase so the mass churns internally.
		a *= 0.88f + 0.12f * (float)sin(t * 1.3f + i * 0.7f);

		// Slightly darker down low, as if light can't reach the bottom.
		float shade = 0.72f + 0.28f * f0;

		glColor4f((GAS_R * shade) / 255.0f, (GAS_G * shade) / 255.0f,
			(GAS_B * shade) / 255.0f, a);
		glBegin(GL_QUADS);
		glVertex2f(0, y0); glVertex2f(SCREEN_WIDTH, y0);
		glVertex2f(SCREEN_WIDTH, y1); glVertex2f(0, y1);
		glEnd();
	}

	// ---- 2. SURFACE: churning, non-flat top edge ----
	float segW = (float)SCREEN_WIDTH / (float)GAS_SURFACE_SEGMENTS;
	for (int i = 0; i < GAS_SURFACE_SEGMENTS; i++) {
		float x0 = i * segW;
		float x1 = x0 + segW + 1.0f;           // +1 closes seams between segments

		// Two waves at different frequency AND speed - a single sine looks
		// mechanical, two beating against each other looks turbulent.
		float phase = (float)i * 0.42f;
		float h = 16.0f * (float)sin(t * 1.7f + phase)
			+ 9.0f * (float)sin(t * 2.9f + phase * 1.9f);

		float capTop = screenTop + h;
		float capBot = screenTop - 26.0f;

		glColor4f(GAS_R / 255.0f, GAS_G / 255.0f, GAS_B / 255.0f, density * 0.85f);
		glBegin(GL_QUADS);
		glVertex2f(x0, capBot); glVertex2f(x1, capBot);
		glVertex2f(x1, capTop); glVertex2f(x0, capTop);
		glEnd();

		// A brighter lip right at the crest catches the "light" and makes
		// the rolling motion readable at a glance.
		glColor4f(0.78f, 0.80f, 0.76f, density * 0.30f);
		glBegin(GL_QUADS);
		glVertex2f(x0, capTop - 5.0f); glVertex2f(x1, capTop - 5.0f);
		glVertex2f(x1, capTop);        glVertex2f(x0, capTop);
		glEnd();
	}

	// ---- 3. WISPS: drifting internal blobs ----
	for (int i = 0; i < GAS_WISP_COUNT; i++) {
		float speed = 18.0f + i * 7.0f;
		// Wrap horizontally across a span wider than the screen so wisps
		// enter and leave instead of popping at the edges.
		float wx = fmod(t * speed + i * 317.0f, (float)(SCREEN_WIDTH + 400)) - 200.0f;
		float wy = screenTop - 30.0f - (float)(i * 34) - 14.0f * (float)sin(t * 1.1f + i);
		float ww = 150.0f + 60.0f * (float)sin(t * 0.6f + i * 2.1f);
		float wh = 40.0f + 16.0f * (float)cos(t * 0.8f + i);

		if (wy + wh < 0.0f) continue;

		glColor4f(0.74f, 0.76f, 0.72f, 0.10f + 0.06f * (float)sin(t * 1.4f + i));
		glBegin(GL_QUADS);
		glVertex2f(wx, wy); glVertex2f(wx + ww, wy);
		glVertex2f(wx + ww, wy + wh); glVertex2f(wx, wy + wh);
		glEnd();
	}

	// ---- 4. HAZE: gas bleeding into clear air above the surface ----
	for (int i = 0; i < 5; i++) {
		float y = screenTop + 12.0f + i * 15.0f;
		float a = (0.14f - i * 0.026f) * density;
		if (a <= 0.0f) break;
		glColor4f(GAS_R / 255.0f, GAS_G / 255.0f, GAS_B / 255.0f, a);
		glBegin(GL_QUADS);
		glVertex2f(0, y); glVertex2f(SCREEN_WIDTH, y);
		glVertex2f(SCREEN_WIDTH, y + 15.0f); glVertex2f(0, y + 15.0f);
		glEnd();
	}

	glDisable(GL_BLEND);
}

// Kept under the old name so every existing call site in this file keeps
// working - the flood is simply drawn as gas now.
inline void drawRisingPlasma() { drawToxicGas(); }

// Static hazard pools painted into the grid get a matching gray wash rather
// than the old violet glow, so tile-5 pools and the rising flood read as the
// same substance.
inline void drawPlasmaGlow(float x, float y, float w, float h, float extraAlpha) {
	float p = fxPulse();
	fxFilledRectA(x, y, w, h,
		GAS_R * (0.85f + 0.15f * p),
		GAS_G * (0.85f + 0.15f * p),
		GAS_B * (0.85f + 0.15f * p),
		0.34f + 0.16f * p + extraAlpha);
}

// ===== Airborne hook for flying enemies (design idea #2) =====
// Flying enemies should punish the hero specifically while he's mid-air
// (jump-pad arcs, gaps between platforms) - a moment ground enemies can't
// threaten. This function is the integration point: whoever owns enemy.h
// can call isHeroAirborneL2() and, when true, make flying enemies shoot
// more aggressively / prioritize the hero. Not wiring this into enemy.h
// directly since that file isn't mine to edit blindly.
inline bool isHeroAirborneL2() {
	return !hero.isGrounded;
}

// Jump-pad launch, plasma damage-over-time, and boss-door win trigger.
inline void updateLevel2Hazards() {
	static bool wasOnPad = false;
	bool onPadNow = hero.isGrounded && overlapsTileIdL2(hero.posX, hero.posY - 1, hero.WIDTH, 1, 6);
	if (onPadNow && !wasOnPad) {
		hero.velocityY = VENT_LAUNCH_SPEED;
		hero.isGrounded = false;
	}
	wasOnPad = onPadNow;

	// Plasma: -10 HP every full second of continuous contact, not a single
	// hit. Checks BOTH the static plasma tiles (if any are placed in the
	// grid) AND the rising flood surface - either counts as "in plasma".
	static int plasmaTickCounter = 0;
	bool inPlasmaNow = overlapsTileIdL2(hero.posX, hero.posY, hero.WIDTH, hero.HEIGHT, 5)
		|| heroIsInRisingPlasma();
	if (inPlasmaNow) {
		plasmaTickCounter++;
		if (plasmaTickCounter >= PLASMA_TICK_INTERVAL) {
			hero.takeDamage(PLASMA_DAMAGE_PER_TICK);
			plasmaTickCounter = 0;
		}
	}
	else {
		plasmaTickCounter = 0;
	}

	if (!isLevel2Complete && overlapsTileIdL2(hero.posX, hero.posY, hero.WIDTH, hero.HEIGHT, 8)) {
		isLevel2Complete = true;
	}
}


// =====================================================================
// MOVING PLATFORMS (tiles 31 / 32)
// =====================================================================

// Called once per level load: pull every 31/32 tile out of the static grid
// and turn it into an oscillating object.
inline void initMovingPlatsFromGridL2() {
	movingPlatCount = 0;
	movePlatClock = 0.0f;
	platCarryResX = platCarryResY = 0.0f;

	for (int r = 0; r < L2_MAX_ROWS; r++) {
		for (int c = 0; c < L2_MAX_COLS; c++) {
			int id = levelGrid2[r][c];
			if (id != 31 && id != 32) continue;
			if (movingPlatCount >= MAX_MOVING_PLATS) continue;

			MovingPlatL2& pl = movingPlats[movingPlatCount++];
			gridCellRect2(r, c, pl.baseX, pl.baseY);
			pl.x = pl.baseX;
			pl.y = pl.baseY;
			pl.dx = pl.dy = 0.0f;
			pl.axis = (id == 31) ? 0 : 1;
			pl.amp = (float)(TILE_SIZE * 3);          // 3 tiles either side
			pl.speed = (pl.axis == 0) ? 0.030f : 0.024f; // vertical a touch slower
			pl.phase = (float)c * 0.7f + (float)r * 0.4f; // de-sync the set
			pl.active = true;

			levelGrid2[r][c] = 0; // it lives in movingPlats[] now, not the grid
		}
	}
}

// Sinusoidal travel: position = centre + amp * sin(t). Speed eases to zero
// at both ends of the run and peaks in the middle, so a platform never
// jerks direction - that smoothness is the whole point of using sin here
// instead of a bounce-at-the-edge counter.
inline void updateMovingPlatsL2() {
	movePlatClock += 1.0f;
	for (int i = 0; i < movingPlatCount; i++) {
		MovingPlatL2& pl = movingPlats[i];
		if (!pl.active) continue;

		float prevX = pl.x, prevY = pl.y;
		float s = (float)sin(pl.phase + movePlatClock * pl.speed);
		if (pl.axis == 0) { pl.x = pl.baseX + pl.amp * s; pl.y = pl.baseY; }
		else { pl.y = pl.baseY + pl.amp * s; pl.x = pl.baseX; }

		pl.dx = pl.x - prevX;
		pl.dy = pl.y - prevY;
	}
}

// Is the hero standing on this particular platform? Same narrow, centred
// probe the grounded check uses, offset to whichever side gravity calls
// "down" right now.
inline bool heroRidesPlatL2(const MovingPlatL2& pl) {
	int narrowW = (int)(hero.WIDTH * GROUNDED_CHECK_WIDTH_FRACTION);
	int narrowX = hero.posX + (hero.WIDTH - narrowW) / 2;
	int probeY = (gravityDir > 0) ? (hero.posY - 3) : (hero.posY + hero.HEIGHT);
	return rectsOverlap(narrowX, probeY, narrowW, 3,
		(int)pl.x, (int)pl.y, TILE_SIZE, TILE_SIZE);
}

// hero.posX/posY are ints but a platform moves by fractions of a pixel per
// tick, so the leftover is banked in platCarryRes* and applied as soon as it
// adds up to a whole pixel. Without that the hero slowly slides off a slow
// platform instead of riding it.
inline void carryHeroOnPlatsL2() {
	// Moving away from gravity means he's mid-jump, not riding anything -
	// carrying him here would zero the jump the tick after it started.
	if (hero.velocityY * gravityDir > 0) { platCarryResX = platCarryResY = 0.0f; return; }

	for (int i = 0; i < movingPlatCount; i++) {
		MovingPlatL2& pl = movingPlats[i];
		if (!pl.active) continue;
		if (!heroRidesPlatL2(pl)) continue;

		platCarryResX += pl.dx;
		platCarryResY += pl.dy;

		int stepX = (int)platCarryResX;
		int stepY = (int)platCarryResY;
		platCarryResX -= (float)stepX;
		platCarryResY -= (float)stepY;

		if (stepX != 0) {
			int prev = hero.posX;
			hero.posX += stepX;
			if (overlapsSolidTileL2(hero.posX, hero.posY, hero.WIDTH, hero.HEIGHT))
				hero.posX = prev; // pinched against a wall - platform slides out from under him
		}
		if (stepY != 0) {
			int prev = hero.posY;
			hero.posY += stepY;
			if (overlapsSolidTileL2(hero.posX, hero.posY, hero.WIDTH, hero.HEIGHT))
				hero.posY = prev;
		}

		hero.isGrounded = true;
		hero.velocityY = 0;
		return; // one platform can carry him at a time
	}
	platCarryResX = platCarryResY = 0.0f;
}

inline void drawMovingPlatsL2() {
	for (int i = 0; i < movingPlatCount; i++) {
		MovingPlatL2& pl = movingPlats[i];
		if (!pl.active) continue;

		float sx = pl.x - cameraX;
		float sy = pl.y - cameraY;
		if (sx < -TILE_SIZE || sx > SCREEN_WIDTH) continue;

		if (imgL2PlatformMid != (unsigned int)-1)
			iShowImage((int)sx, (int)sy, TILE_SIZE, TILE_SIZE, imgL2PlatformMid);
		else {
			iSetColor(70, 180, 230);
			iFilledRectangle(sx, sy, TILE_SIZE, TILE_SIZE);
		}

		// Glowing rail on the leading face: tells the player at a glance
		// that this block is powered and moving, not scenery.
		float p = fxPulse();
		fxFilledRectA(sx, sy + TILE_SIZE - 5.0f, (float)TILE_SIZE, 5.0f,
			90, 220, 255, 0.30f + 0.35f * p);
	}
}

// =====================================================================
// STROBING LASER GATE (tile 34)
// =====================================================================
inline void updateStrobeGateL2() {
	strobeTick++;
	if (strobeTick >= STROBE_PERIOD_TICKS) strobeTick = 0;

	bool wasSolid = strobeSolid;
	strobeSolid = (strobeTick < STROBE_SOLID_TICKS);

	// If the beam comes up while the hero is standing inside it, nudge him
	// back the way he came instead of trapping him inside a solid tile.
	if (strobeSolid && !wasSolid) {
		int guard = 0;
		int push = (hero.facing >= 0) ? -2 : 2;
		while (overlapsTileIdL2(hero.posX, hero.posY, hero.WIDTH, hero.HEIGHT, 34)
			&& guard++ < 60) {
			hero.posX += push;
		}
	}
}

inline void drawStrobeTileL2(float sx, float sy) {
	if (strobeSolid) {
		// Live beam: bright core + hot edges, humming with sin(time).
		float p = 0.5f + 0.5f * (float)sin(pulseTimer * 5.0f);
		fxFilledRectA(sx, sy, (float)TILE_SIZE, (float)TILE_SIZE, 255, 40, 70, 0.45f + 0.30f * p);
		fxFilledRectA(sx + TILE_SIZE * 0.35f, sy, TILE_SIZE * 0.30f, (float)TILE_SIZE,
			255, 220, 230, 0.55f + 0.30f * p);
	}
	else {
		// Powered down: faint emitter outline so the gap is readable and the
		// player knows the beam is coming back.
		float fade = 1.0f - (float)(strobeTick - STROBE_SOLID_TICKS)
			/ (float)(STROBE_PERIOD_TICKS - STROBE_SOLID_TICKS);
		fxFilledRectA(sx, sy, (float)TILE_SIZE, 4.0f, 255, 60, 90, 0.20f + 0.25f * fade);
		fxFilledRectA(sx, sy + TILE_SIZE - 4.0f, (float)TILE_SIZE, 4.0f, 255, 60, 90, 0.20f + 0.25f * fade);
	}
}

// =====================================================================
// GRAVITY-FLIP FIELD (tile 33)
// =====================================================================
inline void updateGravityZoneL2() {
	bool inZone = overlapsTileIdL2(hero.posX, hero.posY, hero.WIDTH, hero.HEIGHT, 33);
	int want = inZone ? -1 : 1;

	if (want != gravityDir) {
		gravityDir = want;
		hero.isGrounded = false;
		// Keep half his momentum through the flip: gravity now works against
		// it, so he arcs over and falls the other way instead of snapping.
		hero.velocityY = hero.velocityY / 2;
		gravFlipFxTicks = 20;
	}

	if (gravFlipFxTicks > 0) gravFlipFxTicks--;
}

inline void drawGravityZoneTileL2(float sx, float sy) {
	float p = fxPulse();
	fxFilledRectA(sx, sy, (float)TILE_SIZE, (float)TILE_SIZE,
		60 + 40 * p, 230, 170 + 60 * p, 0.14f + 0.14f * p);
	// A band that slides along the field in the direction gravity will pull.
	float slide = (float)TILE_SIZE * p;
	float bandY = (gravityDir > 0) ? (sy + TILE_SIZE - slide) : (sy + slide);
	fxFilledRectA(sx, bandY, (float)TILE_SIZE, 5.0f, 180, 255, 220, 0.35f + 0.25f * p);
}

// Brief full-screen tint on the moment of a flip, so the change of
// orientation registers instead of just happening.
inline void drawGravityFlipFlashL2() {
	if (gravFlipFxTicks <= 0) return;
	float a = 0.22f * (gravFlipFxTicks / 20.0f);
	fxFilledRectA(0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 90, 255, 200, a);
}

// =====================================================================
// DARKNESS FIELD (tile 35)
// =====================================================================
inline bool heroInDarknessL2() {
	return overlapsTileIdL2(hero.posX, hero.posY, hero.WIDTH, hero.HEIGHT, 35);
}

// Exponential ease toward the target, so entering/leaving the dark section
// fades over ~half a second instead of hard-cutting to black.
inline void updateDarknessL2() {
	float target = heroInDarknessL2() ? 1.0f : 0.0f;
	darkAmount += (target - darkAmount) * 0.08f;
	if (darkAmount < 0.003f) darkAmount = 0.0f;
	if (darkAmount > 0.997f) darkAmount = 1.0f;
}

// Drawn as two blended rings centred on the hero: an inner ring fading from
// fully transparent (his lit bubble) out to full black, then an outer ring
// of solid black wide enough to cover the screen corners. That gives a
// hole-in-the-darkness with a soft edge without needing a stencil buffer.
inline void drawDarknessOverlayL2() {
	if (darkAmount <= 0.0f) return;

	float cx = (hero.posX - cameraX) + hero.WIDTH / 2.0f;
	float cy = (hero.posY - cameraY) + hero.HEIGHT / 2.0f;

	float rLit = 105.0f + 10.0f * (float)sin(pulseTimer * 1.3); // lamp flicker
	float rDark = rLit + 95.0f;
	float a = 0.94f * darkAmount;

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	const int SLICES = 72;
	const float TWO_PI = 6.28318530718f;

	glBegin(GL_TRIANGLE_STRIP);
	for (int i = 0; i <= SLICES; i++) {
		float t = TWO_PI * (float)i / (float)SLICES;
		float cs = (float)cos(t), sn = (float)sin(t);
		glColor4f(0, 0, 0, 0.0f);
		glVertex2f(cx + rLit * cs, cy + rLit * sn);
		glColor4f(0, 0, 0, a);
		glVertex2f(cx + rDark * cs, cy + rDark * sn);
	}
	glEnd();

	glBegin(GL_TRIANGLE_STRIP);
	for (int i = 0; i <= SLICES; i++) {
		float t = TWO_PI * (float)i / (float)SLICES;
		float cs = (float)cos(t), sn = (float)sin(t);
		glColor4f(0, 0, 0, a);
		glVertex2f(cx + rDark * cs, cy + rDark * sn);
		glColor4f(0, 0, 0, a);
		glVertex2f(cx + 2600.0f * cs, cy + 2600.0f * sn);
	}
	glEnd();

	glDisable(GL_BLEND);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

// One entry point for everything that has to advance on the FIXED 30ms tick
// rather than per drawn frame. Called from updatePlayerMovement() BEFORE the
// hero's own movement and gravity run, because:
//   1. platforms must move first, then carry their rider,
//   2. the gate must settle solid/open before any collision test,
//   3. gravityDir must be current before gravity is applied this tick.
inline void updateLevel2Dynamics() {
	updateMovingPlatsL2();
	carryHeroOnPlatsL2();
	updateStrobeGateL2();
	updateGravityZoneL2();
	updateDarknessL2();
}

inline void resetLevel2Dynamics() {
	strobeTick = 0;
	strobeSolid = true;
	gravityDir = 1;
	gravFlipFxTicks = 0;
	darkAmount = 0.0f;
	initMovingPlatsFromGridL2();
}

inline void initLevel2() {
	initLevel2Assets();
	loadLevel2FromFile("data/levels/level2.txt");
	respawnHeroLevel2();
	hero.facing = 1;
	isLevel2Complete = false;
	currentLevel = 2;

	// The two Level 1 keys were spent breaching this door - Level 2's own
	// two doors issue a fresh pair, which is what unlocks the Titan rift.
	keysCollected = 0;

	resetPlasmaRise();
	resetLevel2Dynamics();

	// Enemy 7 / Enemy 8 - the spawner functions live in Header/enemy.h
	clearEnemies();
	spawnEnemiesFromLevelGrid2(L2_MAX_ROWS, L2_MAX_COLS, TILE_SIZE);
	spawnSpecialEnemiesL2(L2_MAX_ROWS, TILE_SIZE);

	resetHitFeedback();
}

inline void drawLevel2Background() {
	float parallaxX = cameraX * 0.2f;
	float parallaxY = cameraY * 0.2f;
	if (imgL2Background != (unsigned int)-1) {
		iShowImage((int)(-parallaxX), (int)(-parallaxY), SCREEN_WIDTH + 200, SCREEN_HEIGHT + 200, imgL2Background);
	}
	else {
		iSetColor(6, 4, 14);
		iFilledRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	}
}

// A puzzle door: image_8 plate (2x2 tiles) with the crystal console sitting
// on it, plus a breathing halo so it reads as powered and interactive. The
// side-quest door burns red instead of cyan - it is meant to look wrong.
inline void drawDoorL2(float sx, float sy, int doorId) {
	if (imgL2BossDoor != (unsigned int)-1)
		iShowImage((int)sx, (int)sy, TILE_SIZE * L2_DOOR_TILES_W, TILE_SIZE * L2_DOOR_TILES_H, imgL2BossDoor);
	else {
		iSetColor(120, 90, 40);
		iFilledRectangle(sx, sy, TILE_SIZE * L2_DOOR_TILES_W, TILE_SIZE * L2_DOOR_TILES_H);
	}

	if (imgL2Terminal != (unsigned int)-1)
		iShowImage((int)(sx + TILE_SIZE / 2), (int)(sy + TILE_SIZE * L2_DOOR_TILES_H),
		TILE_SIZE, TILE_SIZE, imgL2Terminal);

	float p = fxPulse();
	if (doorId == L2_DOOR_SIDEQUEST)
		fxFilledRectA(sx, sy, (float)(TILE_SIZE * L2_DOOR_TILES_W), (float)(TILE_SIZE * L2_DOOR_TILES_H),
		255, 40, 40, 0.16f + 0.26f * p);
	else
		fxFilledRectA(sx, sy, (float)(TILE_SIZE * L2_DOOR_TILES_W), (float)(TILE_SIZE * L2_DOOR_TILES_H),
		60, 220, 255, 0.12f + 0.20f * p);
}

inline void drawLevel2Tiles() {
	int startCol = (int)(cameraX / TILE_SIZE);
	int endCol = startCol + (SCREEN_WIDTH / TILE_SIZE) + 2;
	if (endCol > L2_MAX_COLS) endCol = L2_MAX_COLS;

	for (int r = 0; r < L2_MAX_ROWS; r++) {
		for (int c = startCol; c < endCol; c++) {
			int id = levelGrid2[r][c];
			// Boundary tiles (1) are kept SOLID for collision but no longer
			// drawn - the tiled border art didn't line up cleanly edge-to-
			// edge and looked visually disconnected. Invisible walls still
			// stop the hero from walking off the map.
			if (id == 0 || id == 1) continue;

			float screenX = (c * TILE_SIZE) - cameraX;
			float screenY = (L2_MAX_ROWS - 1 - r) * TILE_SIZE - cameraY;

			// Procedurally drawn hazard tiles - no art needed, and they have
			// to animate every frame anyway.
			if (isDoorTileL2(id)) { drawDoorL2(screenX, screenY, id); continue; }
			if (id == 33) { drawGravityZoneTileL2(screenX, screenY); continue; }
			if (id == 34) { drawStrobeTileL2(screenX, screenY); continue; }
			if (id == 35) continue; // darkness marker: invisible by design

			unsigned int img = (unsigned int)-1;
			switch (id) {
			case 1:  img = imgL2Boundary;      break;
			case 2:  img = imgL2Debris;        break;
			case 5:  img = imgL2Plasma;        break;
			case 6:  img = imgL2JumpPad;       break;
			case 8:  img = imgL2BossDoor;      break;
			case 11: img = imgL2PlatformLeft;  break;
			case 12: img = imgL2PlatformMid;   break;
			case 13: img = imgL2PlatformRight; break;
			case 21: img = imgL2GroundLeft;    break;
			case 22: img = imgL2GroundMid;     break;
			case 23: img = imgL2GroundRight;   break;
			}

			if (img != (unsigned int)-1) {
				iShowImage((int)screenX, (int)screenY, TILE_SIZE, TILE_SIZE, img);
			}
			else {
				if (id == 1) iSetColor(90, 90, 100);
				else if (id == 2) iSetColor(110, 90, 70);
				else if (id == 5) iSetColor(160, 40, 200);
				else if (id == 6) iSetColor(255, 20, 40);
				else if (id == 8) iSetColor(255, 200, 0);
				else iSetColor(255, 0, 255); // 11/12/13 with a genuinely missing texture
				iFilledRectangle(screenX, screenY, TILE_SIZE, TILE_SIZE);
			}

			// Static plasma pools breathe with the same pulse as the flood.
			if (id == 5)
				drawPlasmaGlow(screenX, screenY, (float)TILE_SIZE, (float)TILE_SIZE, 0.0f);
		}
	}

}

inline void drawLevel2WarpOverlay() {
	if (!isLevel2Complete) return;
	iSetColor(255, 255, 255);
	iFilledRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	iSetColor(0, 0, 0);
	iText(SCREEN_WIDTH / 2 - 160, SCREEN_HEIGHT / 2, (char*)"BOSS CHAMBER UNLOCKED", GLUT_BITMAP_TIMES_ROMAN_24);
}

inline void drawLevel2() {
	drawLevel2Background();
	updateCamera();
	drawLevel2Tiles();
	drawMovingPlatsL2();

	// Level 2's enemies (Enemy 7 / 8 - see spawnEnemiesFromLevelGrid2()).
	// Same two calls Level 1's drawGameLevel() makes; drawLevel2() never
	// made them, so nothing here ever moved, attacked or drew.
	updateEnemies(hero);
	drawEnemies();

	drawExitPortal();
	drawRisingPlasma();

	if (!isLevel2Complete) {
		updateLevel2Hazards();
	}

	// FIX: same missing calls as Level 3's arena - pistol/AK47 spawn their
	// bullets from Playercontroller.hpp regardless of level (it gates on
	// PAGE_PLAYING, which Level 2 runs under), but drawLevel2() never
	// advanced or drew them, so shots fired here spawned into bulletList
	// and just sat there, frozen and invisible, instead of flying across
	// the screen. Same two calls level.hpp's drawGameLevel() makes for
	// Level 1.
	updateBullets();
	drawBullets(cameraX, cameraY);

	float heroScreenX = hero.posX - cameraX;
	float heroScreenY = hero.posY - cameraY;
	int heroImg = hero.currentImage();
	if (heroImg != -1) {
		if (gravityDir < 0) {
			// Gravity is inverted, so draw him upside down. iShowImage() maps
			// fixed texture coords onto the quad it builds from (y .. y+height),
			// so feeding it the top edge plus a NEGATIVE height mirrors the
			// sprite vertically - no extra art and no flipped asset set needed.
			iShowImage((int)heroScreenX, (int)(heroScreenY + hero.HEIGHT),
				hero.WIDTH, -hero.HEIGHT, heroImg);
		}
		else {
			iShowImage((int)heroScreenX, (int)heroScreenY, hero.WIDTH, hero.HEIGHT, heroImg);
		}
	}
	else {
		iSetColor(0, 255, 255);
		iFilledRectangle(heroScreenX, heroScreenY, hero.WIDTH, hero.HEIGHT);
	}

	// Darkness goes on AFTER the hero so his lit bubble reveals him while
	// everything further out stays blacked out. HUD and flashes go on top of
	// the darkness so they stay readable.
	drawDarknessOverlayL2();
	drawGravityFlipFlashL2();

	drawHealthBar(hero);
	// FIX: drawLevel2() never called drawWeaponHUD() either - the top-right
	// weapon readout (icon + round counter) only ever showed up in Level 1.
	drawWeaponHUD(hero);
	drawHitFlashOverlay();
	drawLevel2WarpOverlay();
}

#endif