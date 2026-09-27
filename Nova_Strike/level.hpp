// ===== level.hpp =====
#define _CRT_SECURE_NO_WARNINGS
#ifndef LEVEL_HPP
#define LEVEL_HPP

// Owner ami:)

#include <stdio.h>
#include <math.h>   // sqrtf() - plasma bullet range falloff, see updateBullets() below
#include "Variables.h"
#include "Header\Hero.hpp"

#define MAX_ROWS (42)
#define MAX_COLS 64
#define TILE_SIZE 40

unsigned int iLoadImage(char filename[]);
void iShowImage(int x, int y, int width, int height, unsigned int textureID);
void iSetColor(double r, double g, double b);
void iFilledRectangle(double left, double bottom, double dx, double dy);

struct Player {
	float x, y;
	float width, height;
};

// Global grid, camera position, texture IDs, and player definitions
__declspec(selectany) int levelGrid[MAX_ROWS][MAX_COLS];
__declspec(selectany) int levelGrid2[MAX_ROWS][MAX_COLS];   // Level 2's grid - filled by loadLevel2FromFile() in Level2.hpp; declared up here so enemy.h can read it
__declspec(selectany) float cameraX = 0.0f;
__declspec(selectany) float cameraY = 0.0f;
__declspec(selectany) Player gamePlayer = { 80.0f, 60.0f, 30.0f, 40.0f };
__declspec(selectany) Hero hero;

__declspec(selectany) unsigned int imgBackground = (unsigned int)-1;
__declspec(selectany) unsigned int imgPlatformLeft = (unsigned int)-1;
__declspec(selectany) unsigned int imgPlatformMid = (unsigned int)-1;
__declspec(selectany) unsigned int imgPlatformRight = (unsigned int)-1;
__declspec(selectany) unsigned int imgDoor = (unsigned int)-1;
__declspec(selectany) unsigned int imgPuzzleObj = (unsigned int)-1;

__declspec(selectany) unsigned int imgPlatform1 = (unsigned int)-1;
__declspec(selectany) unsigned int imgPlatform2 = (unsigned int)-1;
__declspec(selectany) unsigned int imgPlatform3 = (unsigned int)-1;
__declspec(selectany) unsigned int imgPlatform4 = (unsigned int)-1;

__declspec(selectany) unsigned int imgground1 = (unsigned int)-1;
__declspec(selectany) unsigned int imgground2 = (unsigned int)-1;
__declspec(selectany) unsigned int imgground3 = (unsigned int)-1;
__declspec(selectany) unsigned int imgground4 = (unsigned int)-1;
__declspec(selectany) unsigned int imgground5 = (unsigned int)-1;

// Dependent header includes
#include "Header\enemy.h"
#include "Header\Bullet.hpp"
#include "Header\Health.hpp"
#include "Header\WeaponHUD.hpp"
#include "Header\thanos.hpp"   // Level 3 boss - see updateBullets() below

// =====================================================================
// SHARED FX LAYER (added): alpha-blended overlay helper, hit-flash +
// camera-shake feedback, and the global gravity direction sign.
//
// iSetColor() only reaches glColor3f(), and iShowImage() binds textures
// with GL_REPLACE - so neither can tint or fade anything. fxFilledRectA()
// below is the one place that turns GL blending on, draws a translucent
// quad, and turns it straight back off, which is what every new glow /
// flash / darkness effect is built from.
// =====================================================================
inline void fxFilledRectA(float x, float y, float w, float h,
	float r, float g, float b, float a) {
	if (a <= 0.0f) return;
	if (a > 1.0f) a = 1.0f;
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(r / 255.0f, g / 255.0f, b / 255.0f, a);
	glBegin(GL_QUADS);
	glVertex2f(x, y);
	glVertex2f(x + w, y);
	glVertex2f(x + w, y + h);
	glVertex2f(x, y + h);
	glEnd();
	glDisable(GL_BLEND);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

// Gravity sign: +1 = normal (pulls down), -1 = flipped (pulls up).
// Level 1 physics ignores this; Level 2's gravity-flip zone drives it.
__declspec(selectany) int gravityDir = 1;

// ---- Hit feedback (red flash + camera shake) ----
// Nothing calls "onHeroHurt()" anywhere in the codebase - damage lands in
// half a dozen places (enemy contact, plasma, bullets). So instead of
// patching every one, we watch hero.currentHP once per fixed tick and fire
// the feedback whenever it drops. Any current or future damage source gets
// the flash for free.
#define HIT_FLASH_TICKS 7
#define HIT_SHAKE_TICKS 9
#define HIT_SHAKE_MAX_PX 7.0f

__declspec(selectany) int hitFlashTicks = 0;
__declspec(selectany) int hitFlashPeak = HIT_FLASH_TICKS;
__declspec(selectany) int hitShakeTicks = 0;
__declspec(selectany) int hitPrevHP = HERO_BASE_HP;
__declspec(selectany) float shakeOffX = 0.0f;
__declspec(selectany) float shakeOffY = 0.0f;

inline void triggerHitFeedback(int dmg) {
	int ticks = HIT_FLASH_TICKS + (dmg >= 25 ? 5 : 0);
	if (ticks > hitFlashTicks) { hitFlashTicks = ticks; hitFlashPeak = ticks; }
	if (hitShakeTicks < HIT_SHAKE_TICKS) hitShakeTicks = HIT_SHAKE_TICKS;
}

inline void resetHitFeedback() {
	hitFlashTicks = 0;
	hitShakeTicks = 0;
	hitPrevHP = hero.currentHP;
	shakeOffX = shakeOffY = 0.0f;
}

// One call per fixed tick (see fixedUpdate() in iMain.cpp).
inline void updateHitFeedback() {
	if (hero.currentHP < hitPrevHP) triggerHitFeedback(hitPrevHP - hero.currentHP);
	hitPrevHP = hero.currentHP;

	if (hitFlashTicks > 0) hitFlashTicks--;

	if (hitShakeTicks > 0) {
		hitShakeTicks--;
		// Magnitude decays to 0 so the shake settles instead of snapping.
		float mag = HIT_SHAKE_MAX_PX * (hitShakeTicks / (float)HIT_SHAKE_TICKS);
		shakeOffX = (float)sin(hitShakeTicks * 2.1) * mag;
		shakeOffY = (float)cos(hitShakeTicks * 3.3) * mag * 0.6f;
	}
	else {
		shakeOffX = shakeOffY = 0.0f;
	}
}

// Drawn last, over everything, in both levels.
inline void drawHitFlashOverlay() {
	if (hitFlashTicks <= 0) return;
	float t = hitFlashTicks / (float)hitFlashPeak; // 1 -> 0, smooth fade out
	float a = 0.34f * t;
	fxFilledRectA(0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 190, 25, 25, a);
	// Brighter bands top/bottom read as a damage vignette rather than a
	// flat wash over the whole screen.
	fxFilledRectA(0, 0, (float)SCREEN_WIDTH, 95.0f, 255, 45, 45, a * 0.75f);
	fxFilledRectA(0, (float)SCREEN_HEIGHT - 95.0f, (float)SCREEN_WIDTH, 95.0f, 255, 45, 45, a * 0.75f);
}


// Level asset textures loading
inline void initLevelAssets() {
	imgBackground = iLoadImage((char*)"Images\\Level_1\\background.png");
	imgPlatformLeft = iLoadImage((char*)"Images\\Level_1\\block 2.jpg");
	imgPlatformMid = iLoadImage((char*)"Images\\Level_1\\block 3.jpg");
	imgPlatformRight = iLoadImage((char*)"Images\\Level_1\\block 4.jpg");
	imgDoor = iLoadImage((char*)"Images\\Level_1\\door 6.png");
	imgPuzzleObj = iLoadImage((char*)"Images\\Level_1\\puzzle 7.jpg");

	imgPlatform1 = iLoadImage((char*)"Images\\Level_1\\Platform_1.png");
	imgPlatform2 = iLoadImage((char*)"Images\\Level_1\\Platform_2.png");
	imgPlatform3 = iLoadImage((char*)"Images\\Level_1\\Platform_3.png");
	imgPlatform4 = iLoadImage((char*)"Images\\Level_1\\Platform_4.png");

	imgground1 = iLoadImage((char*)"Images\\Level_1\\Ground_1.png");
	imgground2 = iLoadImage((char*)"Images\\Level_1\\Ground_2.png");
	imgground3 = iLoadImage((char*)"Images\\Level_1\\Ground_3.png");
	imgground4 = iLoadImage((char*)"Images\\Level_1\\Ground_4.png");
	imgground5 = iLoadImage((char*)"Images\\Level_1\\Ground_5.png");
}

// Map file grid reader & parser
//Mandatory for game
inline void loadLevelFromFile(const char* filename) {
	FILE* file = NULL;
	errno_t err = fopen_s(&file, filename, "r");

	if (err != 0 || !file) {
		for (int r = 0; r < MAX_ROWS; r++)
		for (int c = 0; c < MAX_COLS; c++)
			levelGrid[r][c] = 0;
		return;
	}
	for (int r = 0; r < MAX_ROWS; r++) {
		for (int c = 0; c < MAX_COLS; c++) {
			if (fscanf_s(file, "%d", &levelGrid[r][c]) != 1) {
				levelGrid[r][c] = 0;
			}
		}
	}
	fclose(file);
}

// Reset character position, weapons, and game variables
inline void resetGameState() {
	currentLevel = 1;
	keysCollected = 0;
	hasMedkitPickup = false;
	hasPistolPickup = false;
	hasAK47Pickup = false;
	isLevel2Complete = false;
	score = 0;
	hasFlamethrowerPickup = false;
	sideQuestComplete = false;

	gravityDir = 1;

	gamePlayer.x = 80.0f;
	gamePlayer.y = 60.0f;

	hero.posX = TILE_SIZE;
	hero.posY = 160;
	hero.velocityY = 0;
	hero.isGrounded = true;
	hero.facing = 1;
	resetHeroMaxHP();   // Space Stone boost (500) does not carry into a new run
	hero.currentHP = heroMaxHP;
	hero.isMoving = false;
	hero.isAttacking = false;
	hero.runIndx = 0;
	hero.atkIndx = 0;
	hero.animTick = 0;
	hero.atkTimer = 0;
	hero.hasHit = false;

	hero.weapon.pistol.bulletsFired = 0;
	hero.weapon.pistol.inCooldown = false;
	hero.weapon.ak47.refill(AK47_BASE_MAX_ROUNDS);
	hero.weapon.flamethrower.reset();
	hero.weapon.equipped = WEAPON_KNIFE;

	for (int i = 0; i < MAX_BULLETS; i++)
		bulletList[i].active = false;

	hero.weapon.grenade.reset();
	resetFlyingGrenades();

	initEnemies();
	spawnEnemiesFromLevelGrid(MAX_ROWS, MAX_COLS, TILE_SIZE);
	spawnSpecialEnemies(MAX_ROWS, TILE_SIZE);

	resetHitFeedback();
}

// Full level initialization routine
inline void initGameLevel() {
	initLevelAssets();
	loadLevelFromFile("data/levels/level1.txt");
	hero.loadAssets();
	loadHealthAssets();
	loadWeaponHUDAssets();

	resetGameState();
}

// Cell coordinate converter for world space
inline void gridCellRect(int row, int col, float& x, float& y) {
	x = (float)(col * TILE_SIZE);
	y = (float)((MAX_ROWS - 1 - row) * TILE_SIZE);
}

// Collision detection with non-zero grid blocks
inline bool overlapsSolidTile(int x, int y, int w, int h) {
	for (int r = 0; r < MAX_ROWS; r++) {
		for (int c = 0; c < MAX_COLS; c++) {
			if (levelGrid[r][c] == 0) continue;

			float tileX, tileY;
			gridCellRect(r, c, tileX, tileY);

			if (rectsOverlap(x, y, w, h, (int)tileX, (int)tileY, TILE_SIZE, TILE_SIZE))
				return true;
		}
	}
	return false;
}

// Target tile ID collision check routine
inline bool overlapsTileId(int x, int y, int w, int h, int tileId) {
	for (int r = 0; r < MAX_ROWS; r++) {
		for (int c = 0; c < MAX_COLS; c++) {
			if (levelGrid[r][c] != tileId) continue;

			float tileX, tileY;
			gridCellRect(r, c, tileX, tileY);

			int tileSizeW = (tileId == 6 || tileId == 7 || tileId == 8) ? TILE_SIZE * 2 : TILE_SIZE;
			int tileSizeH = (tileId == 6 || tileId == 7 || tileId == 8) ? TILE_SIZE * 2 : TILE_SIZE;

			if (rectsOverlap(x, y, w, h, (int)tileX, (int)tileY, tileSizeW, tileSizeH))
				return true;
		}
	}
	return false;
}

// Hero proximity checker for interactive objects
inline bool heroNearTileID(int tileId, int radiusPx) {
	return overlapsTileId(hero.posX - radiusPx, hero.posY - radiusPx,
		hero.WIDTH + radiusPx * 2, hero.HEIGHT + radiusPx * 2,
		tileId);
}

// Check ground support below hero
inline bool heroIsSupported() {
	return overlapsSolidTile(hero.posX, hero.posY - 1, hero.WIDTH, 1);
}

// Gravity tick updates & vertical collision handling
inline void heroUpdateGravity() {
	hero.posY += hero.velocityY;

	if (hero.velocityY > 0 && overlapsSolidTile(hero.posX, hero.posY + hero.HEIGHT - 1, hero.WIDTH, 1)) {
		while (overlapsSolidTile(hero.posX, hero.posY, hero.WIDTH, hero.HEIGHT))
			hero.posY--;
		hero.velocityY = 0;
	}

	hero.velocityY -= GRAVITY;

	if (hero.velocityY <= 0 && overlapsSolidTile(hero.posX, hero.posY - 1, hero.WIDTH, 1)) {
		while (overlapsSolidTile(hero.posX, hero.posY, hero.WIDTH, hero.HEIGHT))
			hero.posY++;

		hero.velocityY = 0;
		hero.isGrounded = true;
	}

	if (hero.posY < 0) {
		hero.posY = 0;
		hero.velocityY = 0;
		hero.isGrounded = true;
	}
}

// Live enemy bounding box collision overlap check
inline bool overlapsEnemy(int x, int y, int w, int h) {
	for (int i = 0; i < MAX_ENEMIES; i++) {
		if (!enemyList[i].active) continue;

		int type = enemyList[i].type;
		int ex = enemyList[i].x;
		int ey = enemyList[i].y;
		int ew = ENEMY_TYPE_WIDTH[type];
		int eh = ENEMY_TYPE_HEIGHT[type];

		if (!rectsOverlap(x, y, w, h, ex, ey, ew, eh)) continue;

		int overlap = overlapWidth(x, w, ex, ew);

		if (overlap > ENEMY_WALL_OVERLAP_PX[type])
			return true;
	}
	return false;
}

// Horizontal movement handling with solid block & enemy collision
inline void heroMoveHorizontal(int dir) {
	int prevX = hero.posX;

	hero.moveHorizontal(dir, MAX_COLS * TILE_SIZE);

	if (overlapsSolidTile(hero.posX, hero.posY, hero.WIDTH, hero.HEIGHT) ||
		overlapsEnemy(hero.posX, hero.posY, hero.WIDTH, hero.HEIGHT))
		hero.posX = prevX;
}

// =====================================================================
// PROJECTILE TERRAIN COLLISION - LEVEL AWARE
//
// BUG FIX (this is what broke the plasma bolt in Level 2): updateBullets()
// and updateFlyingGrenades() below both used to call overlapsSolidTile()
// unconditionally. That function only ever reads levelGrid[][] - the
// LEVEL 1 map. Level 2 has a completely separate grid (levelGrid2[][],
// see overlapsSolidTileL2() in Level2.hpp) and Level 3 has no grid at all
// (one solid slab, isSolidAtL3() in Level3.hpp).
//
// So in Level 2 every projectile was being collision-tested against
// Level 1's geometry, which is still sitting in levelGrid[][] from the
// previous level. The plasma bolt spawns at the hero's chest height, and
// wherever that happened to line up with a Level 1 wall the bolt was
// killed on its very first tick - it went inactive before drawBullets()
// ever ran, which is exactly the "the plasma bullet doesn't show properly"
// symptom: the shot animation plays, but the bolt itself flickers for a
// frame or never appears at all, and never travels. Level 3 had the same
// problem for the same reason.
//
// Both L2 and L3 helpers are DEFINED further down the include chain
// (Level2.hpp is #included at the bottom of this file; Level3.hpp comes
// after it in iMain.cpp), so they're forward-declared here. They're
// inline functions defined later in the same translation unit, which is
// legal and links fine - the same "declare early, define later" shape
// spawnFlyingGrenade() already uses in Grenade.hpp.
inline bool overlapsSolidTileL2(int x, int y, int w, int h);
inline bool isSolidAtL3(int x, int y, int w, int h);

// The one terrain test every projectile (bullets AND grenades) should use,
// routed to whichever map is actually loaded.
inline bool projectileHitsTerrain(int x, int y, int w, int h) {
	if (currentLevel == 3) return isSolidAtL3(x, y, w, h);
	if (currentLevel == 2) return overlapsSolidTileL2(x, y, w, h);
	return overlapsSolidTile(x, y, w, h);
}

// Active bullet physics & hit-test updates
inline void updateBullets() {
	for (int i = 0; i < MAX_BULLETS; i++) {
		if (!bulletList[i].active) continue;

		// Level 3 scales the step by real frame time (see l3BeginFrame() in
		// Header/thanos.hpp) so bullets fly at the same speed at any frame
		// rate. Levels 1/2 are untouched.
		float bulletStep = (float)BULLET_SPEED;
		if (currentLevel == 3) bulletStep *= l3Step(L3_BULLET_SPEED_SCALE);
		bulletList[i].x += bulletStep * bulletList[i].dir;

		int bx = (int)bulletList[i].x;
		int by = (int)bulletList[i].y;

		// Plasma bolts don't deal a flat amount - see plasmaDamageAtRange()
		// in Header/PlasmaRifle.hpp. Every other bullet keeps the flat dmg
		// that was baked in at spawn time (see spawnBullet()).
		int dmg = bulletList[i].dmg;
		if (bulletList[i].isPlasma) {
			int heroCenterX = hero.posX + hero.WIDTH / 2;
			int heroCenterY = hero.posY + hero.HEIGHT / 2;
			int bulletCenterX = bx + BULLET_WIDTH / 2;
			int bulletCenterY = by + BULLET_HEIGHT / 2;
			int dx = bulletCenterX - heroCenterX;
			int dy = bulletCenterY - heroCenterY;
			int dist = (int)sqrtf((float)(dx * dx + dy * dy));
			dmg = plasmaDamageAtRange(dist);
		}

		if (damageEnemiesInBox(bx, by, bx + BULLET_WIDTH, by + BULLET_HEIGHT, dmg, bulletList[i].dir, hero)) {
			bulletList[i].active = false;
			continue;
		}

		// Thanos only exists in Level 3's boss arena (see Header/thanos.hpp)
		// - the regular enemy roster above never overlaps him.
		if (currentLevel == 3 && damageThanos(bx, by, bx + BULLET_WIDTH, by + BULLET_HEIGHT, dmg)) {
			bulletList[i].active = false;
			continue;
		}

		// LEVEL-AWARE now - see projectileHitsTerrain() above. This used to
		// be overlapsSolidTile() (Level 1's grid) in every level, which is
		// what killed plasma bolts on spawn in Level 2.
		if (projectileHitsTerrain(bx, by, BULLET_WIDTH, BULLET_HEIGHT)) {
			bulletList[i].active = false;
			continue;
		}

		if (bulletList[i].x < cameraX - BULLET_WIDTH || bulletList[i].x > cameraX + SCREEN_WIDTH)
			bulletList[i].active = false;
	}
}

// Active grenade trajectory and blast radius calculations
inline void updateFlyingGrenades() {
	for (int i = 0; i < MAX_FLYING_GRENADES; i++) {
		FlyingGrenade& g = flyingGrenades[i];
		if (!g.active) continue;

		if (g.bursting) {
			g.burstTicker++;
			if (g.burstTicker >= GRENADE_BURST_FRAME_TICKS) {
				g.burstTicker = 0;
				g.burstFrame++;
				if (g.burstFrame >= GRENADE_BURST_FRAMES)
					g.active = false;
			}
			continue;
		}

		float steppedX = g.x + g.vx;
		// Same level-aware routing as the bullets above - a grenade thrown
		// in Level 2 was being bounced off Level 1's walls.
		if (g.vx != 0 && projectileHitsTerrain((int)steppedX, (int)g.y, GRENADE_ICON_WIDTH, GRENADE_ICON_HEIGHT))
			g.vx = 0;
		else
			g.x = steppedX;

		g.vy -= GRENADE_GRAVITY;
		g.y += g.vy;

		bool landed = (g.vy <= 0 && projectileHitsTerrain((int)g.x, (int)g.y - 1, GRENADE_ICON_WIDTH, 1));
		bool fellOffMap = (g.y < 0);

		if (landed || fellOffMap) {
			while (projectileHitsTerrain((int)g.x, (int)g.y, GRENADE_ICON_WIDTH, GRENADE_ICON_HEIGHT))
				g.y += 1;
			if (g.y < 0) g.y = 0;

			g.vx = 0;
			g.vy = 0;
			g.bursting = true;
			g.burstFrame = 0;
			g.burstTicker = 0;

			int blastCenterX = (int)g.x + GRENADE_ICON_WIDTH / 2;
			int blastCenterY = (int)g.y + GRENADE_ICON_HEIGHT / 2;
			damageEnemiesInRadius(blastCenterX, blastCenterY, GRENADE_BLAST_RADIUS, GRENADE_DMG, hero);

			// Same missing-call bug as the flamethrower (see the
			// "BUG FIX" comment on the flamethrower's damage tick in
			// Playercontroller.hpp) - the grenade blast only ever tested
			// against the regular enemy roster via damageEnemiesInRadius(),
			// which has no concept of Thanos, so a grenade landing right on
			// top of him in Level 3 did nothing. damageThanos() only takes a
			// rectangle, not a circle, so the blast radius is converted into
			// its bounding square - the same "close enough" approximation
			// every other rectangle-vs-Thanos hit test in this project uses.
			if (currentLevel == 3) {
				damageThanos(blastCenterX - GRENADE_BLAST_RADIUS, blastCenterY - GRENADE_BLAST_RADIUS,
				             blastCenterX + GRENADE_BLAST_RADIUS, blastCenterY + GRENADE_BLAST_RADIUS,
				             GRENADE_DMG);
			}
		}
	}
}

// Dynamic 2D camera updating logic
inline void updateCamera() {
	float halfScreenW = SCREEN_WIDTH / 2.0f;
	float halfScreenH = SCREEN_HEIGHT / 2.0f;
	float maxMapWidth = MAX_COLS * TILE_SIZE;
	float maxMapHeight = MAX_ROWS * TILE_SIZE;

	if (hero.posX > halfScreenW) {
		cameraX = hero.posX - halfScreenW;
		if (cameraX > maxMapWidth - SCREEN_WIDTH) cameraX = maxMapWidth - SCREEN_WIDTH;
	}
	else {
		cameraX = 0.0f;
	}

	if (hero.posY > halfScreenH) {
		cameraY = hero.posY - halfScreenH;
		if (cameraY > maxMapHeight - SCREEN_HEIGHT) cameraY = maxMapHeight - SCREEN_HEIGHT;
	}
	else {
		cameraY = 0.0f;
	}

	// Hit-shake offset, applied AFTER the normal follow+clamp so the shake
	// can never scroll the camera past the edges of the map.
	if (shakeOffX != 0.0f || shakeOffY != 0.0f) {
		cameraX += shakeOffX;
		cameraY += shakeOffY;
		if (cameraX < 0.0f) cameraX = 0.0f;
		if (cameraX > maxMapWidth - SCREEN_WIDTH) cameraX = maxMapWidth - SCREEN_WIDTH;
		if (cameraY < 0.0f) cameraY = 0.0f;
		if (cameraY > maxMapHeight - SCREEN_HEIGHT) cameraY = maxMapHeight - SCREEN_HEIGHT;
	}
}

inline void drawLevel2();

// Main draw routine for level 1 & HUD components
inline void drawGameLevel() {
	if (currentLevel == 2) {
		drawLevel2();
		return;
	}

	if (imgBackground != (unsigned int)-1) {
		iShowImage(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, imgBackground);
	}
	else if (bgImage != -1) {
		iShowImage(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, bgImage);
	}
	else {
		iSetColor(10, 10, 20);
		iFilledRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	}

	updateCamera();

	int startCol = (int)(cameraX / TILE_SIZE);
	int endCol = startCol + (SCREEN_WIDTH / TILE_SIZE) + 2;
	if (endCol > MAX_COLS) endCol = MAX_COLS;

	for (int r = 0; r < MAX_ROWS; r++) {
		for (int c = startCol; c < endCol; c++) {
			int tileID = levelGrid[r][c];
			if (tileID == 0 || tileID == 1) continue;

			float screenX = (c * TILE_SIZE) - cameraX;
			float screenY = (MAX_ROWS - 1 - r) * TILE_SIZE - cameraY;

			if (tileID == 2 && imgPlatformLeft != (unsigned int)-1) {
				iShowImage(screenX, screenY, TILE_SIZE, TILE_SIZE, imgPlatformLeft);
			}
			else if (tileID == 3 && imgPlatformMid != (unsigned int)-1) {
				iShowImage(screenX, screenY, TILE_SIZE, TILE_SIZE, imgPlatformMid);
			}
			else if (tileID == 4 && imgPlatformRight != (unsigned int)-1) {
				iShowImage(screenX, screenY, TILE_SIZE, TILE_SIZE, imgPlatformRight);
			}
			else if (tileID == 6 && imgDoor != (unsigned int)-1) {
				iShowImage(screenX, screenY, TILE_SIZE * 2, TILE_SIZE * 2, imgDoor);
			}
			else if (tileID == 7 && imgPuzzleObj != (unsigned int)-1) {
				iShowImage(screenX, screenY, TILE_SIZE * 2, TILE_SIZE * 2, imgPuzzleObj);
			}
			else if (tileID == 8 && imgPuzzleObj != (unsigned int)-1) {
				iSetColor(255, 180, 60);
				iShowImage(screenX, screenY, TILE_SIZE * 2, TILE_SIZE * 2, imgPuzzleObj);
			}
			else if (tileID == 11 && imgPlatform1 != (unsigned int)-1) {
				iShowImage(screenX, screenY, TILE_SIZE, TILE_SIZE, imgPlatform1);
			}
			else if (tileID == 12 && imgPlatform2 != (unsigned int)-1) {
				iShowImage(screenX, screenY, TILE_SIZE, TILE_SIZE, imgPlatform2);
			}
			else if (tileID == 13 && imgPlatform3 != (unsigned int)-1) {
				iShowImage(screenX, screenY, TILE_SIZE, TILE_SIZE, imgPlatform3);
			}
			else if (tileID == 14 && imgPlatform4 != (unsigned int)-1) {
				iShowImage(screenX, screenY, TILE_SIZE, TILE_SIZE, imgPlatform4);
			}
			else if (tileID == 15 && imgground1 != (unsigned int)-1) {
				iShowImage(screenX, screenY, TILE_SIZE, TILE_SIZE, imgground1);
			}
			else if (tileID == 16 && imgground2 != (unsigned int)-1) {
				iShowImage(screenX, screenY, TILE_SIZE, TILE_SIZE, imgground2);
			}
			else if (tileID == 17 && imgground3 != (unsigned int)-1) {
				iShowImage(screenX, screenY, TILE_SIZE, TILE_SIZE, imgground3);
			}
			else if (tileID == 18 && imgground4 != (unsigned int)-1) {
				iShowImage(screenX, screenY, TILE_SIZE, TILE_SIZE, imgground4);
			}
			else if (tileID == 19 && imgground5 != (unsigned int)-1) {
				iShowImage(screenX, screenY, TILE_SIZE, TILE_SIZE, imgground5);
			}
		}
	}

	updateEnemies(hero);
	drawEnemies();

	updateBullets();
	drawBullets(cameraX, cameraY);

	updateFlyingGrenades();
	drawFlyingGrenades(cameraX, cameraY, hero.weapon.grenade);

	float heroScreenX = hero.posX - cameraX;
	float heroScreenY = hero.posY - cameraY;

	int heroImg = hero.currentImage();
	if (heroImg != -1) {
		iShowImage(heroScreenX, heroScreenY, hero.WIDTH, hero.HEIGHT, heroImg);
	}
	else {
		iSetColor(0, 255, 255);
		iFilledRectangle(heroScreenX, heroScreenY, hero.WIDTH, hero.HEIGHT);
	}

	drawHealthBar(hero);
	drawGrenadeHUD(hero);
	drawWeaponHUD(hero);

	drawHitFlashOverlay();
}

#include "Level2.hpp"

#endif