#pragma once

// Ported from the old Nova_Strike codebase (5 enemy types, real-time-based
// movement/animation, block-based spawning). The version that was here
// before this port was an earlier, simpler 2-enemy-type/tick-based draft
// that predates all of that work and was never wired into level.hpp - this
// file replaces it wholesale.

#include "iGraphics.h"
#include "Utility.hpp"          // rectsOverlap() - used by killEnemiesInBox
#include "enemy_properties.h"   // per-type tuning data (speed, size, animation, move type...)
#include "Hero.hpp"             // Hero::takeDamage() - called from updateEnemies() below
#include <cstdlib>
#include <ctime>

// healHeroOnKill() below reads/writes the extern int score declared in
// Variables.h. Not #included directly here (same as Hero.hpp/Utility.hpp
// don't include it either) - this file is only ever reached via level.hpp,
// which already #includes "Variables.h" before it #includes this file, so
// the extern is always in scope by the time healHeroOnKill() is compiled.

// Constants
// Level 1's grid alone has ~90 "2 3...3 4" platform spawn points (see
// spawnEnemiesFromLevelGrid() below). The old cap of 12 meant every spawn
// past the 12th silently did nothing (findEmptySlot() returns -1 and the
// caller just drops it) - most patterned platforms ended up with no enemy
// on them at all, which is what looked like enemies "disappearing".
#define MAX_ENEMIES        128

// How close (vertically, in pixels) the hero has to be to an enemy's own
// height before the enemy will notice him at all - stops an enemy from
// aggroing onto a hero standing on a completely different platform/floor
// just because he's within horizontal ENEMY_ATTACK_RANGE_PX.
#define ENEMY_VERTICAL_DETECT_PX  (TILE_SIZE * 2)

// Enemy behavior state, on top of the walk/fly ENEMY_MOVE_TYPE:
//   PATROL - the original walk-out-and-back behavior.
//   CHASE  - hero is within ENEMY_ATTACK_RANGE_PX, walk toward him.
//   ATTACK - hero is right next to the enemy, play the attack animation.
// Only types with ENEMY_ATTACK_RANGE_PX > 0 (currently 1/2/3/4/5) ever leave
// PATROL - see updateEnemies() below.
enum EnemyState { ENEMY_STATE_PATROL = 0, ENEMY_STATE_CHASE = 1, ENEMY_STATE_ATTACK = 2 };

// Enemy Properties
struct Enemy
{
	int  type;

	int  homeX;
	int  y;
	int  x;

	int  health;                    // current HP, starts at ENEMY_MAX_HEALTH[type]

	bool active;              // slot free?
	bool goingBack;             // true = walking back home
	int  direction;               // -1 = walking left, +1 = walking right

	float xPos;                    // sub-pixel x position (for smooth, frame-rate-independent movement)
	long  walkStartMs;              // clock() time (ms) this outbound trip started
	int   frame;                     // picture index
	long  lastFrameChangeMs;          // clock() time (ms) frame was last advanced

	long  hitFlashUntilMs;            // clock() time (ms) until which this enemy is
	// hidden as a "hit landed" flash - see
	// damageEnemiesInBox() below and
	// drawEnemies() in enemy_animation.h

	bool  hasHitHero;                 // true once THIS swing has already damaged the
	// hero - stops one attack from hitting every
	// frame it's on ENEMY_ATTACK_HIT_FRAME. Reset
	// to false on every state change (same place
	// frame/lastFrameChangeMs reset - see the
	// BUGFIX comment in updateEnemies() below).

	float walkMinX, walkMaxX;         // left/right limits of the outbound leg - the
	// platform run's own edges for grid-spawned
	// walkers, the screen edges for everyone else

	EnemyState state;                 // patrol / chase / attack - see enum above
};

static Enemy enemyList[MAX_ENEMIES];

// Sprite loading, frame-advance timing (nowMs()), and drawing all live in
// enemy_animation.h now - split out on purpose so this file stays about
// spawning/updating enemies, not showing them. It's included here (rather
// than up at the top with the other includes) because it needs struct
// Enemy, enum EnemyState, enemyList and MAX_ENEMIES already declared - see
// enemy_animation.h's own top-of-file comment.
#include "enemy_animation.h"


int findEmptySlot()
{
	for (int i = 0; i < MAX_ENEMIES; i++)
	{
		if (enemyList[i].active == false)
			return i;
	}
	return -1;
}


// Deactivates every enemy WITHOUT reloading their pictures (initEnemies()
// below also reloads them) - used when Level 2 loads, to get rid of Level 1's
// leftover enemies.
void clearEnemies()
{
	for (int i = 0; i < MAX_ENEMIES; i++)
	{
		enemyList[i].active = false;
	}
}


void initEnemies()
{
	loadEnemyPictures();
	srand((unsigned int)time(NULL));

	for (int i = 0; i < MAX_ENEMIES; i++)
	{
		enemyList[i].active = false;
	}
}


// 0-> Enemy 1 (walk)
// 1-> Enemy 2 (walk)
// 2-> Enemy 3 (walk)
// 3-> Enemy 4 (fly)
// 4-> Enemy 5 (walk - a slow, heavy tank; chases the hero once noticed,
//     then swings a slow telegraphed attack once he's close)

// minX/maxX bound the outbound leg (see walkMinX/walkMaxX on Enemy above).
// maxX < minX (the default) means "no specific range given" - fall back to
// the full screen width so old call sites that don't pass bounds still work.
int spawnEnemyAt(int x, int y, int type = 0, int minX = 0, int maxX = -1)
{
	int i = findEmptySlot();
	if (i == -1) return -1;   // slot full

	if (maxX < minX)
		maxX = iScreenWidth - ENEMY_TYPE_WIDTH[type];

	enemyList[i].type = type;
	enemyList[i].homeX = x;
	enemyList[i].x = x;
	enemyList[i].xPos = (float)x;
	enemyList[i].y = y;
	enemyList[i].health = ENEMY_MAX_HEALTH[type];
	enemyList[i].active = true;
	enemyList[i].goingBack = false;
	enemyList[i].state = ENEMY_STATE_PATROL;
	enemyList[i].hasHitHero = false;
	enemyList[i].walkMinX = (float)minX;
	enemyList[i].walkMaxX = (float)maxX;

	// direction - random
	if (rand() % 2 == 0)
		enemyList[i].direction = -1;
	else
		enemyList[i].direction = 1;

	enemyList[i].walkStartMs = nowMs();
	enemyList[i].frame = 0;
	enemyList[i].lastFrameChangeMs = nowMs();
	enemyList[i].hitFlashUntilMs = 0;   // not flashing at spawn

	return i;
}


// Spawns an enemy on a specific grid block (the platform blocks drawn from
// Images\Level_1\block 2.jpg, block 3.jpg, block 4.jpg - imgPlatformLeft/
// Mid/Right in level.hpp), standing on TOP of that block like it's solid
// ground. row/col match levelGrid[row][col] from level.hpp. row 0 is the
// TOP of the map (drawGameLevel() flips rows: screenY uses
// (totalRows - 1 - row)), so we flip the same way here. iShowImage draws
// from (x, y) as the sprite's bottom-left corner upward, so standing "on" a
// block means the enemy's feet (y) must sit at that block's TOP edge, one
// block higher than the block's own bottom - not at the block's bottom edge
// (which would sink the enemy into it). totalRows/blockSize have NO
// hardcoded default here - level.hpp includes this file, so it always has
// MAX_ROWS/TILE_SIZE in scope and passes them explicitly at every call
// site; this keeps enemy.h itself free of any dependency on level.hpp's
// grid size (avoids the circular include: level.hpp -> enemy.h).
// leftBoundCol/rightBoundColInclusive (optional) are the columns of the run
// this block belongs to (e.g. the '2' and '4' of a "2 3 3 4" run) - when
// given, the enemy's outbound leg is clamped to that run instead of the
// whole screen. Left them out (the defaults) to get the old screen-wide
// behavior.
int spawnEnemyAtBlock(int row, int col, int type, int totalRows, int blockSize,
	int leftBoundCol = -1, int rightBoundColInclusive = -1)
{
	int x = col * blockSize;
	int blockBottomY = (totalRows - 1 - row) * blockSize;
	int y = blockBottomY + blockSize;   // stand on top of the block

	int minX = 0, maxX = -1;   // -1 -> spawnEnemyAt falls back to full screen width
	if (leftBoundCol >= 0 && rightBoundColInclusive >= leftBoundCol)
	{
		minX = leftBoundCol * blockSize;
		maxX = (rightBoundColInclusive + 1) * blockSize - ENEMY_TYPE_WIDTH[type];
		if (maxX < minX)
			maxX = minX;   // run is narrower than the sprite - stand still instead of flickering at the edge
	}

	return spawnEnemyAt(x, y, type, minX, maxX);
}


// Scans a level row for platform runs written as "2 3...3 4" (block 2 =
// left edge, one or more 3s = mid tiles, block 4 = right edge - see
// level.hpp's drawGameLevel()) and spawns one enemy on the FIRST mid tile
// and one on the LAST mid tile of each run, each independently either
// Enemy 1 or Enemy 2 (type 0/1, picked with rand()). A run with only one
// mid tile spawns just that single enemy (first == last).
//
// Only a literal 2 opens a run and only a literal 4 (immediately after the
// 3s, no gap) closes it, so unrelated bracket-shaped markers elsewhere in
// the grid - e.g. the door/puzzle frames written as "1 7 1" / "1 6 1" -
// never match and are skipped automatically; nothing special has to be
// done to exclude them.
static void spawnEnemiesOnPlatformRun(int row, int runStartCol, int totalCols, int totalRows, int blockSize)
{
	int col = runStartCol + 1;   // first tile after the opening '2'
	int firstMid = -1, lastMid = -1;

	while (col < totalCols && levelGrid[row][col] == 3)
	{
		if (firstMid == -1) firstMid = col;
		lastMid = col;
		col++;
	}

	// must be at least one '3' and an immediate closing '4' - otherwise
	// this isn't a real platform run (e.g. a lone '2' with nothing after it)
	if (firstMid == -1 || col >= totalCols || levelGrid[row][col] != 4)
		return;

	int runEndCol = col;   // column of the closing '4'

	int type = (rand() % 2 == 0) ? 0 : 1;   // Enemy 1 or Enemy 2
	spawnEnemyAtBlock(row, firstMid, type, totalRows, blockSize, runStartCol, runEndCol);

	if (lastMid != firstMid)
	{
		type = (rand() % 2 == 0) ? 0 : 1;   // re-roll independently for the other end
		spawnEnemyAtBlock(row, lastMid, type, totalRows, blockSize, runStartCol, runEndCol);
	}
}

// The platform runs directly under a "1 7 1"/"1 6 1" door or puzzle marker
// get their own hardcoded Enemy 3 / Enemy 5 placement instead (see
// spawnSpecialEnemies() below), so the general scan must not also drop a
// random Enemy 1/2 onto them. Identified by the column of their opening
// '2' on their own row, since a row can hold other, unrelated runs too
// (e.g. row 8 also has ordinary runs elsewhere that DO want Enemy 1/2).
static bool isReservedForSpecialEnemy(int row, int col)
{
	return (row == 8 && col == 6) ||   // "2 3 3 3 3 3 4",    under the "1 7 1" at rows 6-7,  cols 10-12
		(row == 24 && col == 26) ||   // "2 3 3 3 3 3 3 4",  under the "1 7 1" at rows 22-23, cols 31-33
		(row == 12 && col == 55);     // "2 3 3 3 3 3 4",    under the "1 6 1" at rows 10-11, cols 59-61 (run widened from cols 57-61 to 55-61)
}

// Walks the whole level grid and calls spawnEnemiesOnPlatformRun() on every
// "2 3...3 4" platform run it finds (see that function for exactly what
// counts as a run, and why "1 7 1"/"1 6 1" markers are skipped for free),
// except the 3 runs reserved for spawnSpecialEnemies() (see
// isReservedForSpecialEnemy() above).
void spawnEnemiesFromLevelGrid(int totalRows, int totalCols, int blockSize)
{
	for (int row = 0; row < totalRows; row++)
	{
		for (int col = 0; col < totalCols; col++)
		{
			if (levelGrid[row][col] == 2 && !isReservedForSpecialEnemy(row, col))
				spawnEnemiesOnPlatformRun(row, col, totalCols, totalRows, blockSize);
		}
	}
}

// Hardcoded placements for the 3 platform runs that sit right under a
// "1 7 1" (puzzle) or "1 6 1" (door) marker - Enemy 3 stands on the
// platform itself, under each marker, patrolling within that run's own
// bounds (not the whole screen - see the note just below).
void spawnSpecialEnemies(int totalRows, int blockSize)
{
	// Bounds below match the "2 3...4" platform run each enemy stands on
	// (see isReservedForSpecialEnemy()'s comments for where each run's own
	// columns come from). Without leftBoundCol/rightBoundColInclusive,
	// spawnEnemyAtBlock falls back to the FULL SCREEN WIDTH as the walk
	// range - that's what was sending these enemies walking straight off
	// their platform into empty air (and far enough off that they looked
	// like they'd "disappeared").
	spawnEnemyAtBlock(8, 7, 2, totalRows, blockSize, 6, 12);   // "1 7 1" at rows 6-7,   cols 10-12; run cols 6-12
	spawnEnemyAtBlock(8, 7, 3, totalRows, blockSize, 6, 12);
	spawnEnemyAtBlock(24, 28, 2, totalRows, blockSize, 26, 33);   // "1 7 1" at rows 22-23, cols 31-33; run cols 26-33
	spawnEnemyAtBlock(12, 56, 2, totalRows, blockSize, 55, 61);   // "1 6 1" at rows 10-11, cols 59-61; run cols 55-61
	spawnEnemyAtBlock(12, 56, 4, totalRows, blockSize, 55, 61);

	//test
	spawnEnemyAtBlock(40, 26, 5, totalRows, blockSize, 20, 32);
	spawnEnemyAtBlock(40, 36, 6, totalRows, blockSize, 30, 42);
	spawnEnemyAtBlock(40, 46, 7, totalRows, blockSize, 40, 52);
}

// =====================================================================
// LEVEL 2 SPAWNING (Enemy 7 / Enemy 8)
//
// Same idea as Level 1's spawner above, but it reads levelGrid2 (declared in
// level.hpp, filled by loadLevel2FromFile() in Level2.hpp). Level 2's
// platform runs are written "11 12...12 13" instead of "2 3...3 4".
//
// Enemy 7 (type 6) is 147x147 px and Enemy 8 (type 7) is 112x90 - a lot
// bigger than Enemy 1/2 - so on top of Level 1's logic every spawn first
// checks that the enemy FITS on the run (enemyFitsOnRunL2() below).
// =====================================================================

// The 3 runs right under the door tiles (41 / 42 / 43) get hand-placed guards
// (spawnSpecialEnemiesL2() below), so the general scan skips them. Identified
// by the column of their opening '11', same as Level 1.
static bool isReservedForSpecialEnemyL2(int row, int col)
{
	return (row == 10 && col == 6) ||    // under door 41, run cols 6-12
		(row == 10 && col == 31) ||      // under door 42, run cols 31-35
		(row == 10 && col == 41) ||        // under door 43, run cols 41-48
		(row == 37 && col == 1) ||
		(row == 37 && col == 2) ||
		(row == 37 && col == 3) ||
		(row == 37 && col == 4);
}

// Does an enemy of this type fit on the run (leftCol..rightCol on 'row')?
// Two checks: the run is at least as wide as the sprite, and every tile in
// the rows above the run - as many rows as the sprite is tall - is empty, so
// its head doesn't poke into the platform overhead.
static bool enemyFitsOnRunL2(int type, int row, int leftCol, int rightCol, int blockSize)
{
	int runWidth = (rightCol - leftCol + 1) * blockSize;
	if (ENEMY_TYPE_WIDTH[type] > runWidth)
		return false;

	int rowsNeeded = (ENEMY_TYPE_HEIGHT[type] + blockSize - 1) / blockSize;   // round up

	for (int r = row - 1; r >= row - rowsNeeded; r--)
	{
		if (r < 0)
			return false;   // ran into the top of the map

		for (int col = leftCol; col <= rightCol; col++)
		{
			if (levelGrid2[r][col] != 0)
				return false;
		}
	}
	return true;
}

// Puts one enemy on top of a run, walking only within that run's own edges.
// side: -1 = left end of the run, 0 = middle, 1 = right end.
static int spawnEnemyOnRunL2(int type, int row, int leftCol, int rightCol, int side, int totalRows, int blockSize)
{
	int minX = leftCol * blockSize;
	int maxX = (rightCol + 1) * blockSize - ENEMY_TYPE_WIDTH[type];
	if (maxX < minX)
		maxX = minX;

	int x = minX;
	if (side == 1)
		x = maxX;
	else if (side == 0)
		x = (minX + maxX) / 2;

	// row 0 is the TOP of the map, so flip it, then add one block so the
	// enemy's feet sit on top of the run instead of inside it
	int y = (totalRows - 1 - row) * blockSize + blockSize;

	return spawnEnemyAt(x, y, type, minX, maxX);
}

// Picks Enemy 7 or Enemy 8 at random and spawns it on the run. If the pick
// doesn't fit (Enemy 7 is the big one) it tries Enemy 8 instead, and if that
// doesn't fit either the run just stays empty.
static void spawnRandomEnemyOnRunL2(int row, int leftCol, int rightCol, int side, int totalRows, int blockSize)
{
	int type = (rand() % 2 == 0) ? 6 : 7;   // 6 = Enemy 7, 7 = Enemy 8

	if (!enemyFitsOnRunL2(type, row, leftCol, rightCol, blockSize))
		type = 7;

	if (!enemyFitsOnRunL2(type, row, leftCol, rightCol, blockSize))
		return;

	spawnEnemyOnRunL2(type, row, leftCol, rightCol, side, totalRows, blockSize);
}

// Same run detection as Level 1's spawnEnemiesOnPlatformRun(): a '11', one or
// more '12', then a '13' right after them. Every run gets one enemy on its
// left end; runs 8+ tiles long get a second one on the right end (any
// shorter and two big sprites wouldn't fit side by side).
static void spawnEnemiesOnPlatformRunL2(int row, int runStartCol, int totalCols, int totalRows, int blockSize)
{
	int col = runStartCol + 1;   // first tile after the opening '11'

	while (col < totalCols && levelGrid2[row][col] == 12)
		col++;

	// must be at least one '12' and an immediate closing '13'
	if (col == runStartCol + 1 || col >= totalCols || levelGrid2[row][col] != 13)
		return;

	int runEndCol = col;   // column of the closing '13'

	spawnRandomEnemyOnRunL2(row, runStartCol, runEndCol, -1, totalRows, blockSize);

	if (runEndCol - runStartCol + 1 >= 8)
		spawnRandomEnemyOnRunL2(row, runStartCol, runEndCol, 1, totalRows, blockSize);
}

// Walks the whole of levelGrid2 and spawns on every run except the reserved
// door runs.
void spawnEnemiesFromLevelGrid2(int totalRows, int totalCols, int blockSize)
{
	for (int row = 0; row < totalRows; row++)
	{
		for (int col = 0; col < totalCols; col++)
		{
			if (levelGrid2[row][col] == 11 && !isReservedForSpecialEnemyL2(row, col))
				spawnEnemiesOnPlatformRunL2(row, col, totalCols, totalRows, blockSize);
		}
	}
}

// Hand-placed guards for the 3 reserved runs (all on row 10, under the doors).
// Change the types or add/remove lines freely: 6 = Enemy 7, 7 = Enemy 8.
void spawnSpecialEnemiesL2(int totalRows, int blockSize)
{
	// door 41 (AK47 puzzle) - run cols 6-12: two Enemy 8
	spawnEnemyOnRunL2(5, 10, 6, 12, -1, totalRows, blockSize);
	spawnEnemyOnRunL2(6, 10, 6, 12, 1, totalRows, blockSize);

	// door 42 (key + health) - run cols 31-35, only 200 px wide: one Enemy 7
	spawnEnemyOnRunL2(5, 10, 31, 35, 0, totalRows, blockSize);

	// door 43 (side quest / Space Stone) - run cols 41-48: Enemy 7 + Enemy 8
	spawnEnemyOnRunL2(5, 10, 41, 48, -1, totalRows, blockSize);
	spawnEnemyOnRunL2(6, 10, 41, 48, 1, totalRows, blockSize);
}


int spawnEnemyRandom(int type = 0)
{
	int maxX = iScreenWidth - ENEMY_TYPE_WIDTH[type];
	int maxY = iScreenHeight - ENEMY_TYPE_HEIGHT[type];
	if (maxX < 1) maxX = 1;
	if (maxY < 1) maxY = 1;

	int randomX = rand() % maxX;
	int randomY = rand() % maxY;

	return spawnEnemyAt(randomX, randomY, type);
}


// Killing enemy 6 (type index 5) heals the hero 5% of max HP, killing
// enemy 7 (type index 6) heals 10% - simple reward for the tougher enemies.
// Every other enemy, Enemy 8 included, heals nothing (Enemy 8 used to fully
// restore HP on kill; that was removed). Clamped so
// currentHP never goes above heroMaxHP (max). Also awards the player
// ENEMY_SCORE_VALUE[type] points (Header/enemy_properties.h) toward the
// run's score (Variables.h) - same "every kill, no matter the source"
// reasoning applies. Called from every place below that actually
// deactivates an enemy (killEnemy/killEnemiesInBox/damageEnemiesInBox/
// damageEnemiesInRadius), so both the heal and the score apply no matter
// which weapon (knife, pistol, AK47, flamethrower, or grenade splash)
// lands the kill.
void healHeroOnKill(int type, Hero& hero)
{
	float healPercent = 0.0f;
	if (type == 5) healPercent = 0.05f;        // enemy 6
	else if (type == 6) healPercent = 0.10f;   // enemy 7

	if (healPercent > 0.0f)
	{
		hero.currentHP += (int)(heroMaxHP * healPercent);
		if (hero.currentHP > heroMaxHP) hero.currentHP = heroMaxHP;
	}

	score += ENEMY_SCORE_VALUE[type];
}

void killEnemy(int index, Hero& hero)
{
	if (index >= 0 && index < MAX_ENEMIES)
	{
		healHeroOnKill(enemyList[index].type, hero);
		enemyList[index].active = false;
	}
}

// Kills every active enemy whose box overlaps the given box (e.g. for the
// hero's Knife/Pistol attacks - see Header/Knife.hpp, Header/Pistol.hpp).
void killEnemiesInBox(int x1, int y1, int x2, int y2, Hero& hero)
{
	for (int i = 0; i < MAX_ENEMIES; i++)
	{
		if (enemyList[i].active == false)
			continue;

		int type = enemyList[i].type;
		if (rectsOverlap(x1, y1, x2 - x1, y2 - y1,
			enemyList[i].x, enemyList[i].y,
			ENEMY_TYPE_WIDTH[type], ENEMY_TYPE_HEIGHT[type]))
		{
			healHeroOnKill(type, hero);
			enemyList[i].active = false;
		}
	}
}

// Damages (rather than instantly killing) every active enemy whose box
// overlaps the given box by dmg HP - used by both the knife's melee hitbox
// and pistol bullets (see Playercontroller.hpp and updateBullets() in
// level.hpp) so either weapon chips away at an enemy's health instead of
// one-shotting like killEnemiesInBox() above. Any enemy whose health drops
// to 0 or below is deactivated - it disappears from the map.
//
// dir is which way to knock the hit enemy back (+1 right, -1 left) - the
// knife passes hero.facing, a bullet passes its own flight direction, so
// the enemy gets shoved further the way the hit was already traveling.
// Every landed hit also briefly hides the enemy (ENEMY_HIT_FLASH_MS) as a
// quick "that landed" flash - see hitFlashUntilMs on Enemy above and
// drawEnemies() in enemy_animation.h.
//
// Returns true if at least one enemy was hit, so a caller (a bullet) can
// react - e.g. stop right there like it hit a wall.
bool damageEnemiesInBox(int x1, int y1, int x2, int y2, int dmg, int dir, Hero& hero)
{
	bool hitAny = false;

	for (int i = 0; i < MAX_ENEMIES; i++)
	{
		if (enemyList[i].active == false)
			continue;

		int type = enemyList[i].type;
		if (rectsOverlap(x1, y1, x2 - x1, y2 - y1,
			enemyList[i].x, enemyList[i].y,
			ENEMY_TYPE_WIDTH[type], ENEMY_TYPE_HEIGHT[type]))
		{
			hitAny = true;
			enemyList[i].health -= dmg;

			if (enemyList[i].health <= 0)
			{
				healHeroOnKill(type, hero);
				enemyList[i].active = false;   // dead - disappears from the map
				continue;
			}

			// knockback - nudge both x (what's actually drawn/collided
			// against) and xPos (the sub-pixel position updateEnemies()
			// moves from every frame) so the shove sticks instead of being
			// overwritten by the next movement tick.
			int kb = (dir >= 0) ? ENEMY_KNOCKBACK_PX : -ENEMY_KNOCKBACK_PX;
			enemyList[i].x += kb;
			enemyList[i].xPos += (float)kb;

			// hit-flash - drawEnemies() skips drawing this enemy until this
			// timestamp passes, reading as a quick "that landed" blink.
			enemyList[i].hitFlashUntilMs = nowMs() + ENEMY_HIT_FLASH_MS;
		}
	}

	return hitAny;
}

// Same idea as damageEnemiesInBox() above, but for a circular blast (the
// grenade's splash - see GRENADE_BLAST_RADIUS/GRENADE_DMG in
// Header/Grenade.hpp, called from updateFlyingGrenades() in level.hpp the
// instant a thrown grenade lands) instead of a rectangular hitbox.
//
// Distance is measured from the blast center to the CLOSEST point on each
// enemy's own box (the standard circle-vs-rectangle test), not just its
// center, so a blast still reaches an enemy whose box merely pokes into the
// radius even if its center sits outside it.
void damageEnemiesInRadius(int centerX, int centerY, int radius, int dmg, Hero& hero)
{
	for (int i = 0; i < MAX_ENEMIES; i++)
	{
		if (enemyList[i].active == false)
			continue;

		int type = enemyList[i].type;
		int ex = enemyList[i].x, ey = enemyList[i].y;
		int ew = ENEMY_TYPE_WIDTH[type], eh = ENEMY_TYPE_HEIGHT[type];

		int closestX = centerX;
		if (closestX < ex) closestX = ex;
		if (closestX > ex + ew) closestX = ex + ew;

		int closestY = centerY;
		if (closestY < ey) closestY = ey;
		if (closestY > ey + eh) closestY = ey + eh;

		int dx = centerX - closestX;
		int dy = centerY - closestY;

		if ((dx * dx + dy * dy) > (radius * radius))
			continue; // out of blast range

		enemyList[i].health -= dmg;

		if (enemyList[i].health <= 0)
		{
			healHeroOnKill(type, hero);
			enemyList[i].active = false; // dead - disappears from the map
			continue;
		}

		// knock back AWAY from the blast center (rather than a fixed
		// direction like damageEnemiesInBox()'s dir param, since a blast
		// has no single travel direction of its own).
		int kb = (ex + ew / 2 >= centerX) ? ENEMY_KNOCKBACK_PX : -ENEMY_KNOCKBACK_PX;
		enemyList[i].x += kb;
		enemyList[i].xPos += (float)kb;

		enemyList[i].hitFlashUntilMs = nowMs() + ENEMY_HIT_FLASH_MS;
	}
}


// hero: passed by reference (rather than just his box, like before) so
// that landed attacks can call hero.takeDamage() directly from here - see
// the ENEMY_STATE_ATTACK branch below. Also drives the chase/attack state
// machine same as before. Only enemy types with ENEMY_ATTACK_RANGE_PX > 0
// (currently 1/2/3 - see enemy_properties.h) ever leave ENEMY_STATE_PATROL.
void updateEnemies(Hero& hero)
{
	int heroX = hero.posX, heroY = hero.posY, heroWidth = hero.WIDTH, heroHeight = hero.HEIGHT;

	long now = nowMs();

	// Real time elapsed since the last updateEnemies() call, shared by all
	// enemies this frame. Computed once here (not per-enemy) so it reflects
	// actual frame time regardless of how many enemies are active.
	static long lastUpdateMs = now;
	float deltaSeconds = (now - lastUpdateMs) / 1000.0f;
	if (deltaSeconds < 0.0f) deltaSeconds = 0.0f;
	if (deltaSeconds > 0.1f) deltaSeconds = 0.1f; // clamp huge gaps (e.g. window drag)
	lastUpdateMs = now;

	int heroCenterX = heroX + heroWidth / 2;
	int heroCenterY = heroY + heroHeight / 2;

	for (int i = 0; i < MAX_ENEMIES; i++)
	{
		if (enemyList[i].active == false)
			continue;   // skip if slot free

		int   type = enemyList[i].type;
		float step = ENEMY_SPEED_PX_PER_SEC[type] * deltaSeconds;   // per-type speed

		// --- aggro state machine (chase/attack) ---
		// Recomputed fresh from the hero's current distance every frame
		// instead of storing "why" an enemy is aggroed - simpler, and it
		// self-corrects the instant the hero moves away, no extra bookkeeping.
		// ENEMY_ATTACK_RANGE_PX[type] is 0 for types that don't support this
		// yet (only enemy_4 now), so it always computes PATROL here and the
		// branch below behaves exactly as before for it.
		EnemyState desiredState = ENEMY_STATE_PATROL;

		int enemyCenterX = enemyList[i].x + ENEMY_TYPE_WIDTH[type] / 2;
		int enemyCenterY = enemyList[i].y + ENEMY_TYPE_HEIGHT[type] / 2;

		int dx = heroCenterX - enemyCenterX;
		int absDx = (dx < 0) ? -dx : dx;
		int dy = heroCenterY - enemyCenterY;
		int absDy = (dy < 0) ? -dy : dy;

		// gap between the two sprites' edges - <= 0 means they're
		// touching/overlapping, i.e. close enough to actually attack.
		int meleeGap = absDx - (ENEMY_TYPE_WIDTH[type] / 2 + heroWidth / 2);

		if (currentLevel == 3)
		{
			// LEVEL 3 BOSS-ARENA WAVE: every enemy is aggro'd on the hero the
			// instant it spawns, no matter where either of them is on screen -
			// unlike Levels 1/2 there's no ENEMY_VERTICAL_DETECT_PX gate and
			// no ENEMY_ATTACK_RANGE_PX cutoff to lose the hero at, so it's
			// just "close the gap, then attack" for the whole arena, the
			// entire time. Same meleeGap/ENEMY_ATTACK_TRIGGER_GAP_PX trigger
			// as Levels 1/2 use for the actual swing - only the "did it even
			// notice him" gate is removed here.
			if (meleeGap <= ENEMY_ATTACK_TRIGGER_GAP_PX[type])
				desiredState = ENEMY_STATE_ATTACK;
			else
				desiredState = ENEMY_STATE_CHASE;
		}
		else if (ENEMY_ATTACK_RANGE_PX[type] > 0)
		{
			if (absDy <= ENEMY_VERTICAL_DETECT_PX)
			{
				// ENEMY_ATTACK_TRIGGER_GAP_PX is <= 0, so this asks for
				// meleeGap to go past plain "touching" (0) by that many
				// more pixels - i.e. the enemy visibly closes the last bit
				// of distance into the hero before the attack animation
				// starts, instead of swinging from a step away.
				if (meleeGap <= ENEMY_ATTACK_TRIGGER_GAP_PX[type])
					desiredState = ENEMY_STATE_ATTACK;
				else if (absDx <= ENEMY_ATTACK_RANGE_PX[type])
					desiredState = ENEMY_STATE_CHASE;
			}
		}

		if (desiredState != enemyList[i].state)
		{
			if (desiredState == ENEMY_STATE_PATROL)
			{
				// lost the hero - resume patrolling fresh from wherever
				// the enemy currently is, instead of forcing it back home first
				enemyList[i].goingBack = false;
				enemyList[i].walkStartMs = now;
			}

			// BUGFIX: frame used to only get reset when ENTERING attack
			// state, never when LEAVING it. Attack clips can have more
			// frames than the walk cycle (enemy_1: 4 attack frames vs only
			// 2 walk frames) - so an enemy that attacked the hero (frame
			// left sitting at, say, 2 or 3) and then went back to
			// PATROL/CHASE kept that same out-of-range frame index into
			// leftPic/rightPic, which only has walk frames loaded that
			// high up. That unloaded texture slot is what rendered as a
			// plain untextured square hovering on the enemy - i.e. exactly
			// the "pass through enemy_1 and a square box appears" bug.
			// Resetting frame on EVERY state change (not just ATTACK-entry)
			// means a walk/chase cycle always starts from a valid frame 0.
			enemyList[i].frame = 0;
			enemyList[i].lastFrameChangeMs = now;
			enemyList[i].hasHitHero = false; // fresh swing (or no swing) - clear last one's hit

			enemyList[i].state = desiredState;
		}

		// Remember where this enemy stood before it moves this frame, so a
		// step that would walk it straight through the hero can be undone
		// below - the enemy-side half of the same "can't pass through each
		// other" rule; the hero's own half (he can't step into/through a
		// live enemy either) is overlapsEnemy()/heroMoveHorizontal() in
		// level.hpp.
		int   prevX = enemyList[i].x;
		float prevXPos = enemyList[i].xPos;

		if (enemyList[i].state == ENEMY_STATE_ATTACK)
		{
			// stand still and face the hero.
			enemyList[i].direction = (heroCenterX >= enemyList[i].x + ENEMY_TYPE_WIDTH[type] / 2) ? 1 : -1;

			// land the hit once this swing reaches its hit frame - hasHitHero
			// stops it landing again every frame the animation sits on/past
			// that frame, so one swing chips exactly ENEMY_ATTACK_DMG[type]
			// off the hero, same as the hero's own hasHit/attackHitTick.
			if (!enemyList[i].hasHitHero && enemyList[i].frame >= ENEMY_ATTACK_HIT_FRAME[type])
			{
				enemyList[i].hasHitHero = true;
				hero.takeDamage(ENEMY_ATTACK_DMG[type]);
			}
		}
		else if (enemyList[i].state == ENEMY_STATE_CHASE)
		{
			// walk toward the hero, still clamped to this enemy's own
			// patrol bounds (walkMinX/walkMaxX) so it can't chase off its platform
			int dir = (heroCenterX >= enemyList[i].x + ENEMY_TYPE_WIDTH[type] / 2) ? 1 : -1;
			enemyList[i].direction = dir;
			enemyList[i].xPos += step * dir;

			if (enemyList[i].xPos < enemyList[i].walkMinX)
				enemyList[i].xPos = enemyList[i].walkMinX;
			else if (enemyList[i].xPos > enemyList[i].walkMaxX)
				enemyList[i].xPos = enemyList[i].walkMaxX;

			enemyList[i].x = (int)(enemyList[i].xPos + 0.5f);
		}
		else if (enemyList[i].goingBack == false)
		{
			// move away from home, clamped to this enemy's own walk range
			// (walkMinX/walkMaxX - the platform run's edges for grid-spawned
			// walkers, the screen edges for everyone else, set once back in
			// spawnEnemyAt()). A fixed range only ever flips direction once,
			// right when it reaches an edge - unlike re-probing the ground
			// every frame, which could fail on BOTH sides in the same frame
			// on a platform barely wider than the sprite and flip the
			// direction (and so the left/right picture) every single frame,
			// which is what looked like a facing/animation glitch.
			enemyList[i].xPos += step * enemyList[i].direction;

			if (enemyList[i].xPos <= enemyList[i].walkMinX)
			{
				enemyList[i].xPos = enemyList[i].walkMinX;
				enemyList[i].direction = 1;
			}
			else if (enemyList[i].xPos >= enemyList[i].walkMaxX)
			{
				enemyList[i].xPos = enemyList[i].walkMaxX;
				enemyList[i].direction = -1;
			}

			if (now - enemyList[i].walkStartMs >= ENEMY_WALK_MS[type])
			{
				// time to turn back home
				enemyList[i].goingBack = true;
			}

			enemyList[i].x = (int)(enemyList[i].xPos + 0.5f);
		}
		else
		{
			// move back home
			float distanceToHome = enemyList[i].homeX - enemyList[i].xPos;

			if (distanceToHome > -step && distanceToHome < step)
			{
				// arrived - start a new trip
				enemyList[i].xPos = (float)enemyList[i].homeX;
				enemyList[i].goingBack = false;
				enemyList[i].walkStartMs = now;

				// random direction for next trip
				if (rand() % 2 == 0)
					enemyList[i].direction = -1;
				else
					enemyList[i].direction = 1;
			}
			else if (distanceToHome > 0)
			{
				enemyList[i].direction = 1;    // home is to the right
				enemyList[i].xPos += step;
			}
			else
			{
				enemyList[i].direction = -1;   // home is to the left
				enemyList[i].xPos -= step;
			}

			enemyList[i].x = (int)(enemyList[i].xPos + 0.5f);
		}

		// undo this frame's move if it walked the enemy TOO FAR into the
		// hero's box - same "allow a little overlap, then act like a wall"
		// rule as overlapsEnemy() in level.hpp (the hero's own half of
		// this), controlled by ENEMY_WALL_OVERLAP_PX in enemy_properties.h.
		// ATTACK doesn't move x at all, so this is a no-op there.
		bool touchingHero = rectsOverlap(enemyList[i].x, enemyList[i].y, ENEMY_TYPE_WIDTH[type], ENEMY_TYPE_HEIGHT[type],
			heroX, heroY, heroWidth, heroHeight);

		if (touchingHero)
		{
			int overlap = overlapWidth(enemyList[i].x, ENEMY_TYPE_WIDTH[type], heroX, heroWidth);

			if (overlap > ENEMY_WALL_OVERLAP_PX[type])
			{
				enemyList[i].x = prevX;
				enemyList[i].xPos = prevXPos;
			}
		}

		// animation (walk or attack, depending on type) - real-time delay
		// so playback speed doesn't depend on how fast the draw loop runs.
		// See enemy_animation.h's advanceEnemyAnimation() for why ATTACK
		// paces itself off its own separate timer/frame-count.
		advanceEnemyAnimation(enemyList[i], now);
	}
}

// =====================================================================
// LEVEL 3 WAVE SPAWNING
//
// Moved here from Level3.hpp so every bit of code that puts an enemy on
// the floor lives in one place. Level3.hpp still owns the WAVE TIMER/PHASE
// (level3Phase, level3ClockTicks, the boss-intro trigger) - only the
// "spawn N enemies onto the arena floor" logic lives here, parameterized on
// the arena width / floor Y Level3.hpp passes in, so this file stays free
// of any L3_* macro and doesn't need to #include Level3.hpp.
//
// Spawn X is restricted to the MIDDLE-TO-RIGHT half of the arena, not the
// full width. The hero starts pinned near the LEFT edge of the Level 3
// floor (see initLevel3() in Level3.hpp) - spawning across the full arena
// width let enemies land right on top of / directly in front of him the
// instant the wave started. Clamping every spawn to
// [arenaWidth/2, arenaWidth - enemyWidth] keeps the wave on the far side of
// the arena instead, so the hero always gets a beat to see them coming
// before anything reaches him.
// =====================================================================
inline int level3SpawnBandX(int type, int arenaWidth)
{
	int width = ENEMY_TYPE_WIDTH[type];
	int minX = arenaWidth / 2;
	int maxX = arenaWidth - width;
	if (maxX < minX) maxX = minX;   // arena too narrow to split - fall back to "as far right as possible"
	return minX + rand() % (maxX - minX + 1);
}

// One random-type enemy, standing on the floor, somewhere in the
// middle-to-right band (see level3SpawnBandX() above).
inline void spawnLevel3Enemy(int arenaWidth, int floorY)
{
	int type = rand() % ENEMY_TYPE_COUNT;
	int x = level3SpawnBandX(type, arenaWidth);
	spawnEnemyAt(x, floorY, type, 0, arenaWidth - ENEMY_TYPE_WIDTH[type]);
}

// Same as spawnLevel3Enemy() but the caller picks the TYPE instead of it
// being randomized - used by spawnLevel3EnemyWave() below to guarantee the
// opening wave includes every entry in the roster at least once.
inline void spawnLevel3EnemyOfType(int type, int arenaWidth, int floorY)
{
	int x = level3SpawnBandX(type, arenaWidth);
	spawnEnemyAt(x, floorY, type, 0, arenaWidth - ENEMY_TYPE_WIDTH[type]);
}

// The Level 3 opening wave: totalCount enemies, the first ENEMY_TYPE_COUNT
// of them one of EACH type in the roster (shuffled so the guaranteed set
// doesn't always land type-0-first), the rest fully random types - same mix
// Level3.hpp's level3StartWave() used to build inline. Assumes the caller
// has already cleared the floor (clearEnemies()) - this only ever ADDS
// enemies, it never clears anything itself.
inline void spawnLevel3EnemyWave(int arenaWidth, int floorY, int totalCount)
{
	int shuffledTypes[ENEMY_TYPE_COUNT];
	for (int i = 0; i < ENEMY_TYPE_COUNT; i++) shuffledTypes[i] = i;
	for (int i = ENEMY_TYPE_COUNT - 1; i > 0; i--) {
		int j = rand() % (i + 1);
		int tmp = shuffledTypes[i]; shuffledTypes[i] = shuffledTypes[j]; shuffledTypes[j] = tmp;
	}

	int guaranteed = ENEMY_TYPE_COUNT < totalCount ? ENEMY_TYPE_COUNT : totalCount;
	for (int i = 0; i < guaranteed; i++) spawnLevel3EnemyOfType(shuffledTypes[i], arenaWidth, floorY);
	for (int i = guaranteed; i < totalCount; i++) spawnLevel3Enemy(arenaWidth, floorY);
}

// drawEnemies() now lives in enemy_animation.h (included above) - it's
// purely about showing the current frame, not about spawning/updating.