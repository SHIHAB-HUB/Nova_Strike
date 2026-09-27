// =====================================================================
// SideQuest.hpp - LEVEL 2 SIDE QUEST: "THE FROZEN NODE"
// Entered from the third Level 2 door (tile 43, the one that warns
// "DO NOT ENTER").
//
// A single-screen low-gravity mini-map on an alien moon (background.jpg):
// six floating platforms built from the Level 1 block art, patrolling
// flying drones, and a frozen switch on the highest platform at the far
// top-right.
//
// Survival is a race between three clocks:
//   * OXYGEN  - drains from 100% the moment you arrive. At 0% you die.
//   * WATER   - the Level 2 flood keeps rising underneath you while you are
//               in here; it is the same global 3-minute clock, not a second
//               timer, so hiding in the quest does not stop the level.
//   * DRONES  - keep attacking until the switch is thrown.
//
// The switch itself is encased in ice and cannot be touched. The ice is
// treated as a 200 HP entity that ONLY the flamethrower can burn through -
// see WEAPON_FLAMETHROWER in Header/WeaponControl.hpp. The flamethrower
// itself comes from solving Puzzle 4 (Weather Node, Level 2 / Door 2 - see
// grantFlamethrowerAndKey() in PuzzleCommon.hpp), not from this quest, so
// it's effectively a prerequisite for melting the ice here. Once it breaks,
// [E] on the switch refills oxygen, halts the drain, grounds the drones and
// heals the hero to full.
//
// NOTE for whoever owns the enemies: sqFlyers[] below is a deliberately
// simple placeholder patrol. The six spawn anchors are sqFlyerHome[] - swap
// the update/draw bodies for the real flying enemy type and nothing else in
// this file has to change.
// =====================================================================
#ifndef SIDEQUEST_HPP
#define SIDEQUEST_HPP

#include <math.h>
#include <stdio.h>
#include "Variables.h"
#include "level.hpp"
#include "PuzzleCommon.hpp"
#include "SoundManager.hpp"

int isKeyPressed(unsigned char key);
int isSpecialKeyPressed(unsigned char key);

// ---------------------------------------------------------------- layout --
#define SQ_PLATFORMS 6
#define SQ_TILE 40
#define SQ_INTERACT_RADIUS 70

struct SqPlatform { int x, y, tiles; };

// Five ordinary platforms plus the sixth (index 5, top-right) that carries
// the switch. Laid out as a climb: bottom-left up to top-right.
__declspec(selectany) SqPlatform sqPlatforms[SQ_PLATFORMS] = {
	{ 60, 110, 5 },
	{ 330, 205, 4 },
	{ 120, 330, 4 },
	{ 560, 300, 4 },
	{ 830, 405, 4 },
	{ 980, 530, 5 }   // switch platform
};

inline int sqPlatWidth(int i) { return sqPlatforms[i].tiles * SQ_TILE; }

// ------------------------------------------------------------ quest state --
#define SQ_OXYGEN_DRAIN_PER_TICK 0.030f   // 100% -> 0% in ~100 seconds
#define SQ_ICE_MAX_HP 200
#define SQ_WATER_MAX_HEIGHT 430.0f
#define SQ_WATER_DAMAGE 10
#define SQ_WATER_DAMAGE_INTERVAL 33
#define SQ_WARN_FIRST_TICK (60 * TICKS_PER_SECOND)   // 1 minute in
#define SQ_WARN_SECOND_TICK (70 * TICKS_PER_SECOND)  // 10 seconds later
#define SQ_WARN_DURATION (3 * TICKS_PER_SECOND)

__declspec(selectany) unsigned int imgSqBackground = (unsigned int)-1;
__declspec(selectany) float sqOxygen = 100.0f;
__declspec(selectany) bool sqOxygenHalted = false;
__declspec(selectany) int sqIceHP = SQ_ICE_MAX_HP;
__declspec(selectany) bool sqSwitchThrown = false;
__declspec(selectany) bool sqDronesActive = true;
__declspec(selectany) int sqTicks = 0;
__declspec(selectany) int sqWarnFlashTicks = 0;
__declspec(selectany) int sqWarnsShown = 0;
__declspec(selectany) int sqMeltedToastTicks = 0;
__declspec(selectany) int sqDoneToastTicks = 0;
// The "switch online / oxygen restored" banner is QUEUED, not started, when
// the switch is thrown: the Space Stone reward toast has to play out first.
// This counts the wait, then arms sqDoneToastTicks (see updateSideQuest).
__declspec(selectany) int sqDoneToastDelay = 0;

#define SQ_STONE_TOAST_TICKS 150    // how long the Space Stone toast holds
#define SQ_DONE_TOAST_TICKS  135    // how long the oxygen-restored banner holds
#define SQ_TOAST_GAP_TICKS   12     // small beat between the two messages
__declspec(selectany) int sqReturnTicks = 0;      // countdown back to Level 2
__declspec(selectany) int sqWaterDamageTick = 0;

// Hero's Level 2 position, parked while he is in the quest.
__declspec(selectany) int sqSavedHeroX = 0;
__declspec(selectany) int sqSavedHeroY = 0;
__declspec(selectany) int sqSavedGravityDir = 1;

// ------------------------------------------------------------- the switch --
// Sits on top of the sixth platform, at its right end.
inline void sqSwitchRect(int& x, int& y, int& w, int& h) {
	w = 70; h = 56;
	x = sqPlatforms[5].x + sqPlatWidth(5) - w - 20;
	y = sqPlatforms[5].y + SQ_TILE;
}

inline bool sqIceIntact() { return sqIceHP > 0; }

// ------------------------------------------------------------- the drones --
struct SqFlyer {
	float x, y;
	float homeX, homeY;
	float phase;
	int attackCooldown;
	bool alive;
};

__declspec(selectany) SqFlyer sqFlyers[SQ_PLATFORMS];

#define SQ_FLYER_W 44
#define SQ_FLYER_H 34
#define SQ_FLYER_DMG 6              // rebalanced alongside ENEMY_ATTACK_DMG
#define SQ_FLYER_ATTACK_INTERVAL 45
#define SQ_FLYER_RANGE 46

inline void sqResetFlyers() {
	for (int i = 0; i < SQ_PLATFORMS; i++) {
		SqFlyer& f = sqFlyers[i];
		f.homeX = (float)(sqPlatforms[i].x + sqPlatWidth(i) / 2);
		f.homeY = (float)(sqPlatforms[i].y + 120);
		f.x = f.homeX;
		f.y = f.homeY;
		f.phase = (float)i * 1.1f;
		f.attackCooldown = 0;
		f.alive = true;
	}
}

// ------------------------------------------------------- enemy_7 walkers --
// Ground mini-bosses that patrol the platforms. Built in the same shape as
// SqFlyer above (own struct, own reset/update/draw, all local to this file)
// rather than going through enemy.h's spawnEnemiesFromLevelGrid(), because
// that spawner is driven by the main-level TILE GRID and the side quest has
// no grid - it uses the sqPlatforms[] rectangle list instead.
//
// Layout, per design: 2 walkers on ordinary platforms, 3 on the switch
// platform (index 5) so the switch is genuinely defended.
#define SQ_WALKERS 5
#define SQ_WALKER_TYPE 6            // index into the enemy_7 sprite arrays
#define SQ_WALKER_W 92              // scaled down from the 140px source art
#define SQ_WALKER_H 92              // so 3 fit on one platform and read clearly
#define SQ_WALKER_HP 120
#define SQ_WALKER_DMG 34            // matches ENEMY_ATTACK_DMG[6]
#define SQ_WALKER_PATROL_SPEED 1.7f // px/tick - "kinda faster" than the mains
#define SQ_WALKER_CHARGE_SPEED 3.4f // doubles once it has aggro
#define SQ_WALKER_WALK_FRAMES 4
#define SQ_WALKER_ATTACK_FRAMES 7
#define SQ_WALKER_WALK_FRAME_TICKS 4   // ~7.5 fps cycle - smooth, not strobing
#define SQ_WALKER_ATTACK_FRAME_TICKS 3 // faster cadence so hits read crisply
#define SQ_WALKER_ATTACK_HIT_FRAME 3
#define SQ_WALKER_AGGRO_RANGE 260
#define SQ_WALKER_ATTACK_RANGE 74
#define SQ_WALKER_ATTACK_COOLDOWN 28

struct SqWalker {
	float x, y;
	int platform;        // which sqPlatforms[] index it patrols
	int dir;             // -1 left, +1 right
	int hp;
	bool alive;
	bool aggro;          // latched true once the hero lands on its platform
	bool attacking;
	int frame;           // current animation frame
	int frameTick;       // ticks held on this frame
	int attackCooldown;
	int hitFlash;
};

__declspec(selectany) SqWalker sqWalkers[SQ_WALKERS];

// walk_7 / attack_7, left+right. Loaded once, same guard pattern as every
// other asset in this project.
__declspec(selectany) unsigned int imgSqWalkWalk[2][SQ_WALKER_WALK_FRAMES] = {
	{ (unsigned int)-1, (unsigned int)-1, (unsigned int)-1, (unsigned int)-1 },
	{ (unsigned int)-1, (unsigned int)-1, (unsigned int)-1, (unsigned int)-1 }
};
__declspec(selectany) unsigned int imgSqWalkAtk[2][SQ_WALKER_ATTACK_FRAMES] = {
	{ (unsigned int)-1, (unsigned int)-1, (unsigned int)-1, (unsigned int)-1, (unsigned int)-1, (unsigned int)-1, (unsigned int)-1 },
	{ (unsigned int)-1, (unsigned int)-1, (unsigned int)-1, (unsigned int)-1, (unsigned int)-1, (unsigned int)-1, (unsigned int)-1 }
};
__declspec(selectany) bool sqWalkerArtLoaded = false;

inline void sqLoadWalkerArt() {
	if (sqWalkerArtLoaded) return;
	sqWalkerArtLoaded = true;

	char path[160];
	for (int f = 0; f < SQ_WALKER_WALK_FRAMES; f++) {
		sprintf_s(path, sizeof(path), "Enemy_Images/enemy_7/walk_7/enemy_7_walk_left_%d.png", f + 1);
		imgSqWalkWalk[0][f] = safeLoadImage(path);
		sprintf_s(path, sizeof(path), "Enemy_Images/enemy_7/walk_7/enemy_7_walk_right_%d.png", f + 1);
		imgSqWalkWalk[1][f] = safeLoadImage(path);
	}
	for (int f = 0; f < SQ_WALKER_ATTACK_FRAMES; f++) {
		sprintf_s(path, sizeof(path), "Enemy_Images/enemy_7/attack_7/enemy_7_attack_left_%d.png", f + 1);
		imgSqWalkAtk[0][f] = safeLoadImage(path);
		sprintf_s(path, sizeof(path), "Enemy_Images/enemy_7/attack_7/enemy_7_attack_right_%d.png", f + 1);
		imgSqWalkAtk[1][f] = safeLoadImage(path);
	}
}

inline void sqResetWalkers() {
	sqLoadWalkerArt();

	// Two on ordinary platforms (1 and 3), three stacked across the switch
	// platform (5) at different offsets so they don't spawn on top of
	// each other.
	const int homePlat[SQ_WALKERS] = { 1, 3, 5, 5, 5 };
	const float offsetFrac[SQ_WALKERS] = { 0.35f, 0.40f, 0.12f, 0.45f, 0.78f };

	for (int i = 0; i < SQ_WALKERS; i++) {
		SqWalker& w = sqWalkers[i];
		w.platform = homePlat[i];
		const SqPlatform& p = sqPlatforms[w.platform];

		w.x = (float)p.x + sqPlatWidth(w.platform) * offsetFrac[i] - SQ_WALKER_W * 0.5f;
		w.y = (float)p.y + SQ_TILE;      // stand ON the platform's top face
		w.dir = (i % 2 == 0) ? 1 : -1;
		w.hp = SQ_WALKER_HP;
		w.alive = true;
		w.aggro = false;
		w.attacking = false;
		w.frame = 0;
		w.frameTick = 0;
		w.attackCooldown = 0;
		w.hitFlash = 0;
	}
}

// True while the hero is standing on this walker's platform. This is the
// aggro trigger: "the moment Nova lands on their platform".
inline bool sqHeroOnPlatform(int platIdx) {
	const SqPlatform& p = sqPlatforms[platIdx];
	int top = p.y + SQ_TILE;
	// Feet within a few px of the platform's top face, and horizontally
	// within its span.
	if (hero.posY > top + 6 || hero.posY < top - 10) return false;
	return (hero.posX + hero.WIDTH) > p.x && hero.posX < (p.x + sqPlatWidth(platIdx));
}

// Solid-body check used by sqHeroMove() so the hero cannot walk through a
// living walker - he has to kill it.
inline bool sqOverlapsWalker(int x, int y, int w, int h) {
	for (int i = 0; i < SQ_WALKERS; i++) {
		const SqWalker& k = sqWalkers[i];
		if (!k.alive) continue;
		// A little horizontal forgiveness so bodies touch rather than
		// snapping apart at arm's length.
		if (rectsOverlap(x, y, w, h, (int)k.x + 18, (int)k.y, SQ_WALKER_W - 36, SQ_WALKER_H))
			return true;
	}
	return false;
}

// Damage entry point for the hero's weapons (see sqApplyPlayerHitToWalkers
// wired into the existing bullet/melee loops).
inline bool sqDamageWalkerAt(int x, int y, int w, int h, int dmg) {
	bool hitAny = false;
	for (int i = 0; i < SQ_WALKERS; i++) {
		SqWalker& k = sqWalkers[i];
		if (!k.alive) continue;
		if (!rectsOverlap(x, y, w, h, (int)k.x, (int)k.y, SQ_WALKER_W, SQ_WALKER_H)) continue;

		k.hp -= dmg;
		k.hitFlash = 6;
		k.aggro = true;              // shooting one wakes it even from range
		hitAny = true;

		if (k.hp <= 0) {
			k.alive = false;
			score += 50;             // matches ENEMY_SCORE_VALUE[6]
		}
	}
	return hitAny;
}

// FIX: pistol/AK47 bullets in the side quest. This reuses the exact same
// bulletList pool spawnBullet()/drawBullets() already work with (see
// Header/Bullet.hpp, included transitively via level.hpp above) - it just
// can't reuse level.hpp's updateBullets(), because that checks hits against
// the MAIN LEVEL's enemyList/levelGrid, and the side quest has its own
// separate sqFlyers[]/sqWalkers[] instead. Same per-tick physics
// (bulletList[i].x += BULLET_SPEED * dir), just checked against the side
// quest's own targets, and bounded by the raw screen edges since this page
// has no camera scroll (see drawSideQuestPage() - hero is drawn straight at
// hero.posX/posY, no cameraX/Y subtraction, so bullets shouldn't use one
// either).
inline void updateSqBullets() {
	for (int i = 0; i < MAX_BULLETS; i++) {
		if (!bulletList[i].active) continue;

		bulletList[i].x += BULLET_SPEED * bulletList[i].dir;

		int bx = (int)bulletList[i].x;
		int by = (int)bulletList[i].y;

		bool hit = false;
		for (int f = 0; f < SQ_PLATFORMS; f++) {
			if (!sqFlyers[f].alive) continue;
			if (rectsOverlap(bx, by, BULLET_WIDTH, BULLET_HEIGHT,
				(int)sqFlyers[f].x, (int)sqFlyers[f].y, SQ_FLYER_W, SQ_FLYER_H)) {
				sqFlyers[f].alive = false;
				hit = true;
			}
		}
		if (sqDamageWalkerAt(bx, by, BULLET_WIDTH, BULLET_HEIGHT, bulletList[i].dmg))
			hit = true;

		if (hit) {
			bulletList[i].active = false;
			continue;
		}

		if (bulletList[i].x < -BULLET_WIDTH || bulletList[i].x > SCREEN_WIDTH)
			bulletList[i].active = false;
	}
}

inline void sqUpdateWalkers() {
	for (int i = 0; i < SQ_WALKERS; i++) {
		SqWalker& k = sqWalkers[i];
		if (!k.alive) continue;

		if (k.hitFlash > 0) k.hitFlash--;
		if (k.attackCooldown > 0) k.attackCooldown--;

		const SqPlatform& p = sqPlatforms[k.platform];
		float leftBound = (float)p.x;
		float rightBound = (float)(p.x + sqPlatWidth(k.platform)) - SQ_WALKER_W;

		// --- aggro latch ---
		float heroCenter = (float)hero.posX + hero.WIDTH * 0.5f;
		float myCenter = k.x + SQ_WALKER_W * 0.5f;
		float dx = heroCenter - myCenter;
		float adx = dx < 0 ? -dx : dx;

		if (sqHeroOnPlatform(k.platform) && adx < SQ_WALKER_AGGRO_RANGE)
			k.aggro = true;

		// --- attacking state: play the swing out, land the hit on the
		// designated frame, then drop back to walking ---
		if (k.attacking) {
			k.frameTick++;
			if (k.frameTick >= SQ_WALKER_ATTACK_FRAME_TICKS) {
				k.frameTick = 0;
				k.frame++;

				if (k.frame == SQ_WALKER_ATTACK_HIT_FRAME) {
					// Hit lands mid-swing, and only if the hero is still
					// in range - lets the player dodge out of a committed
					// attack instead of eating it on contact alone.
					if (rectsOverlap(hero.posX, hero.posY, hero.WIDTH, hero.HEIGHT,
						(int)k.x - 10, (int)k.y, SQ_WALKER_W + 20, SQ_WALKER_H)) {
						hero.takeDamage(SQ_WALKER_DMG);
					}
				}

				if (k.frame >= SQ_WALKER_ATTACK_FRAMES) {
					k.attacking = false;
					k.frame = 0;
					k.attackCooldown = SQ_WALKER_ATTACK_COOLDOWN;
				}
			}
			continue;   // no movement mid-swing
		}

		// --- start an attack when close enough ---
		if (k.aggro && adx < SQ_WALKER_ATTACK_RANGE && k.attackCooldown == 0) {
			k.attacking = true;
			k.frame = 0;
			k.frameTick = 0;
			k.dir = (dx >= 0) ? 1 : -1;
			continue;
		}

		// --- movement: charge the hero when aggroed, else patrol ---
		float speed = k.aggro ? SQ_WALKER_CHARGE_SPEED : SQ_WALKER_PATROL_SPEED;

		if (k.aggro) {
			k.dir = (dx >= 0) ? 1 : -1;
		}

		k.x += k.dir * speed;

		// Stay on the platform either way - they never walk off the edge.
		if (k.x < leftBound) { k.x = leftBound; k.dir = 1; }
		if (k.x > rightBound) { k.x = rightBound; k.dir = -1; }

		// --- walk animation ---
		k.frameTick++;
		if (k.frameTick >= SQ_WALKER_WALK_FRAME_TICKS) {
			k.frameTick = 0;
			k.frame = (k.frame + 1) % SQ_WALKER_WALK_FRAMES;
		}
	}
}

inline void sqDrawWalker(const SqWalker& k) {
	if (!k.alive) return;

	int facing = (k.dir >= 0) ? 1 : 0;   // 0 = left art, 1 = right art
	unsigned int img = k.attacking
		? imgSqWalkAtk[facing][k.frame < SQ_WALKER_ATTACK_FRAMES ? k.frame : SQ_WALKER_ATTACK_FRAMES - 1]
		: imgSqWalkWalk[facing][k.frame < SQ_WALKER_WALK_FRAMES ? k.frame : 0];

	if (img != (unsigned int)-1)
		iShowImage((int)k.x, (int)k.y, SQ_WALKER_W, SQ_WALKER_H, img);
	else {
		iSetColor(180, 60, 60);
		iFilledRectangle(k.x, k.y, SQ_WALKER_W, SQ_WALKER_H);
	}

	// White flash on hit, same feedback language the main enemies use.
	if (k.hitFlash > 0)
		fxFilledRectA(k.x, k.y, (float)SQ_WALKER_W, (float)SQ_WALKER_H, 255, 255, 255, 0.45f);

	// Health pip above the head so the player can see progress on a 120 HP
	// body - without it they read as unkillable.
	float frac = (float)k.hp / (float)SQ_WALKER_HP;
	if (frac < 1.0f) {
		iSetColor(30, 30, 35);
		iFilledRectangle(k.x + 16, k.y + SQ_WALKER_H + 6, SQ_WALKER_W - 32, 6);
		fxFilledRectA(k.x + 16, k.y + SQ_WALKER_H + 6, (SQ_WALKER_W - 32) * frac, 6.0f,
			frac < 0.35f ? 230 : 90, frac < 0.35f ? 60 : 220, 80, 0.9f);
	}
}

// ------------------------------------------------------------- collisions --
inline bool sqOverlapsPlatform(int x, int y, int w, int h) {
	for (int i = 0; i < SQ_PLATFORMS; i++) {
		if (rectsOverlap(x, y, w, h, sqPlatforms[i].x, sqPlatforms[i].y, sqPlatWidth(i), SQ_TILE))
			return true;
	}
	return false;
}

inline bool sqHeroSupported() {
	int narrowW = (int)(hero.WIDTH * 0.6f);
	int narrowX = hero.posX + (hero.WIDTH - narrowW) / 2;
	return sqOverlapsPlatform(narrowX, hero.posY - 1, narrowW, 1);
}

// Same shape as the Level 2 gravity routine, minus the gravity flip - the
// quest map is always the right way up.
inline void sqHeroGravity() {
	hero.posY += hero.velocityY;

	if (sqOverlapsPlatform(hero.posX, hero.posY, hero.WIDTH, hero.HEIGHT)) {
		int step = (hero.velocityY > 0) ? -1 : 1;
		int guard = 0;
		while (sqOverlapsPlatform(hero.posX, hero.posY, hero.WIDTH, hero.HEIGHT) && guard++ < 200)
			hero.posY += step;
		bool landed = (hero.velocityY <= 0);
		hero.velocityY = 0;
		if (landed) hero.isGrounded = true;
	}

	hero.velocityY -= GRAVITY;

	if (hero.velocityY <= 0 && sqHeroSupported()) {
		int guard = 0;
		while (sqOverlapsPlatform(hero.posX, hero.posY, hero.WIDTH, hero.HEIGHT) && guard++ < 200)
			hero.posY++;
		hero.velocityY = 0;
		hero.isGrounded = true;
	}

	// Floor of the map - he cannot fall out of the world, he just ends up in
	// the water, which is its own problem.
	if (hero.posY < 0) {
		hero.posY = 0;
		hero.velocityY = 0;
		hero.isGrounded = true;
	}
}

inline void sqHeroMove(int dir) {
	int prevX = hero.posX;
	hero.moveHorizontal(dir, SCREEN_WIDTH);
	// Platforms block as before - and now so do living enemy_7 walkers, so
	// the hero genuinely has to kill them instead of strolling past.
	if (sqOverlapsPlatform(hero.posX, hero.posY, hero.WIDTH, hero.HEIGHT) ||
		sqOverlapsWalker(hero.posX, hero.posY, hero.WIDTH, hero.HEIGHT))
		hero.posX = prevX;
}

// ------------------------------------------------------------ enter/leave --
inline void initSideQuestAssets() {
	if (imgSqBackground == (unsigned int)-1)
		imgSqBackground = safeLoadImage("Images\\Level_2\\background.jpg");
}

inline void startSideQuest() {
	initSideQuestAssets();

	// Park the Level 2 position so leaving puts him back exactly where he
	// pressed E, facing the door.
	sqSavedHeroX = hero.posX;
	sqSavedHeroY = hero.posY;
	sqSavedGravityDir = gravityDir;
	gravityDir = 1;

	hero.posX = sqPlatforms[0].x + 20;
	hero.posY = sqPlatforms[0].y + SQ_TILE;
	hero.velocityY = 0;
	hero.isGrounded = true;
	hero.facing = 1;

	sqOxygen = 100.0f;
	sqOxygenHalted = false;
	sqIceHP = SQ_ICE_MAX_HP;
	sqSwitchThrown = false;
	sqDronesActive = true;
	sqTicks = 0;
	sqWarnFlashTicks = 0;
	sqWarnsShown = 0;
	sqMeltedToastTicks = 0;
	sqDoneToastTicks = 0;
	sqDoneToastDelay = 0;
	sqReturnTicks = 0;
	sqWaterDamageTick = 0;
	sqResetFlyers();
	sqResetWalkers();

	// The flamethrower is NOT handed out here anymore - it now comes from
	// solving Puzzle 4 (Weather Node, Level 2 / Door 2 - see
	// grantFlamethrowerAndKey() in PuzzleCommon.hpp). The ice wall below
	// still only yields to it (see the file banner above), so Puzzle 4 is
	// effectively a soft prerequisite for finishing this quest: walking in
	// without hasFlamethrowerPickup just means key '4' does nothing here
	// (see the side quest's own key handler further down) until it's been
	// earned properly.
	currentPage = PAGE_SIDEQUEST;
}

inline void exitSideQuest() {
	hero.posX = sqSavedHeroX;
	hero.posY = sqSavedHeroY;
	gravityDir = sqSavedGravityDir;
	hero.velocityY = 0;
	hero.isGrounded = false;
	hero.weapon.flamethrower.reset();
	stopFlameSfx();
	currentPage = PAGE_PLAYING;
}

// ----------------------------------------------------------- fixed update --
inline void updateSideQuest() {
	if (currentPage != PAGE_SIDEQUEST) return;

	sqTicks++;
	sqUpdateWalkers();

	// --- death checks first, so nothing else runs on a dead hero ---
	// Per the respawn matrix, dying in the side quest does NOT show the
	// Game Over screen and does NOT end the run - the hero just wakes back
	// up on the entry platform. Only the quest's own state is rewound;
	// score, keys and weapon unlocks are all left alone.
	if (sqOxygen <= 0.0f || hero.isDead()) {
		hero.posX = sqPlatforms[0].x + 20;
		hero.posY = sqPlatforms[0].y + SQ_TILE;
		hero.velocityY = 0;
		hero.isGrounded = true;
		hero.facing = 1;
		hero.currentHP = heroMaxHP;
		hero.isAttacking = false;
		hero.atkTimer = 0;
		hero.hasHit = false;

		sqOxygen = 100.0f;
		sqWaterDamageTick = 0;
		sqResetWalkers();      // mini-bosses come back with him
		return;
	}

	// --- oxygen ---
	if (!sqOxygenHalted) {
		sqOxygen -= SQ_OXYGEN_DRAIN_PER_TICK;
		if (sqOxygen < 0.0f) sqOxygen = 0.0f;
	}

	// --- warning banner: once at 1:00, once more 10s later ---
	if (sqWarnsShown == 0 && sqTicks >= SQ_WARN_FIRST_TICK && !sqSwitchThrown) {
		sqWarnFlashTicks = SQ_WARN_DURATION;
		sqWarnsShown = 1;
	}
	else if (sqWarnsShown == 1 && sqTicks >= SQ_WARN_SECOND_TICK && !sqSwitchThrown) {
		sqWarnFlashTicks = SQ_WARN_DURATION;
		sqWarnsShown = 2;
	}
	if (sqWarnFlashTicks > 0) sqWarnFlashTicks--;
	if (sqMeltedToastTicks > 0) sqMeltedToastTicks--;
	if (sqDoneToastTicks > 0) sqDoneToastTicks--;

	// Hand-off between the two completion messages: when the Space Stone
	// toast's slot expires, the oxygen-restored banner takes the screen.
	if (sqDoneToastDelay > 0) {
		sqDoneToastDelay--;
		if (sqDoneToastDelay == 0) sqDoneToastTicks = SQ_DONE_TOAST_TICKS;
	}

	// --- movement ---
	bool leftHeld = isKeyPressed('a') || isKeyPressed('A') || isSpecialKeyPressed(GLUT_KEY_LEFT);
	bool rightHeld = isKeyPressed('d') || isKeyPressed('D') || isSpecialKeyPressed(GLUT_KEY_RIGHT);
	if (leftHeld) sqHeroMove(-1);
	if (rightHeld) sqHeroMove(1);
	if (!leftHeld && !rightHeld) hero.stopMoving();
	if (isKeyPressed('w') || isKeyPressed('W') || isSpecialKeyPressed(GLUT_KEY_UP)) hero.startJump();

	if (hero.isGrounded && !sqHeroSupported()) hero.isGrounded = false;
	if (!hero.isGrounded) sqHeroGravity();

	// --- weapon switching (same locks as the main game) ---
	// FlameThrower lives on slot 4 now (moved from 5) - same slot as the
	// main game's Playercontroller.hpp.
	if (!hero.isAttacking) {
		if (isKeyPressed('1')) hero.weapon.equip(WEAPON_KNIFE);
		if (isKeyPressed('3') && hasAK47Pickup) hero.weapon.equip(WEAPON_AK47);
		if (isKeyPressed('4') && hasFlamethrowerPickup) hero.weapon.equip(WEAPON_FLAMETHROWER);
	}

	// --- flamethrower: the only thing that hurts the ice ---
	bool spaceHeld = isKeyPressed(' ') != 0;
	if (hero.weapon.equipped == WEAPON_FLAMETHROWER) {
		FlameThrower& flame = hero.weapon.flamethrower;
		bool wasActive = flame.isActive();
		bool dealsDamageNow = flame.update(spaceHeld);

		hero.isAttacking = flame.isActive();
		hero.atkIndx = flame.frameIndex;

		if (!wasActive && flame.isActive()) playFlameSfx();
		if (wasActive && !flame.isActive()) stopFlameSfx();

		if (dealsDamageNow) {
			int x1, y1, x2, y2;
			hero.flameHitbox(x1, y1, x2, y2);

			// Ice casing - treated exactly like an enemy with 200 HP.
			if (sqIceIntact()) {
				int ix, iy, iw, ih;
				sqSwitchRect(ix, iy, iw, ih);
				if (rectsOverlap(x1, y1, x2 - x1, y2 - y1, ix - 8, iy - 8, iw + 16, ih + 16)) {
					sqIceHP -= FLAMETHROWER_DMG;
					if (sqIceHP <= 0) {
						sqIceHP = 0;
						sqMeltedToastTicks = 90; // "Melted!"
					}
				}
			}

			// Flame also clears drones that get in the way.
			for (int i = 0; i < SQ_PLATFORMS; i++) {
				if (!sqFlyers[i].alive) continue;
				if (rectsOverlap(x1, y1, x2 - x1, y2 - y1,
					(int)sqFlyers[i].x, (int)sqFlyers[i].y, SQ_FLYER_W, SQ_FLYER_H))
					sqFlyers[i].alive = false;
			}

			// ...and burns the enemy_7 walkers. They have real HP rather
			// than dying on contact like the drones, so the flamethrower
			// has to be held on them.
			sqDamageWalkerAt(x1, y1, x2 - x1, y2 - y1, FLAMETHROWER_DMG);
		}
	}
	else {
		hero.weapon.updateCooldowns();

		if (spaceHeld) {
			bool justStarted = hero.startAttack();

			// FIX: pistol/AK47 now fire a real flying bullet here too, the
			// same spawn math the main game uses in Playercontroller.hpp -
			// instead of the old behavior below, which ran EVERY non-flame
			// weapon (knife AND pistol AND AK47) through the melee hitbox
			// on a timer tick and never called spawnBullet() at all. That's
			// why no bullet ever appeared in the side quest: ranged shots
			// were dealing damage as an invisible instant melee swing.
			if (justStarted && (hero.weapon.equipped == WEAPON_PISTOL || hero.weapon.equipped == WEAPON_AK47)) {
				int muzzleY = hero.posY + hero.HEIGHT / 2 - BULLET_HEIGHT / 2 + BULLET_MUZZLE_Y_OFFSET;
				int muzzleX = (hero.facing >= 0) ? (hero.posX + hero.WIDTH) : (hero.posX - BULLET_WIDTH);
				unsigned int img = hero.weapon.bulletImg(hero.facing);
				spawnBullet(muzzleX, muzzleY, hero.facing, hero.weapon.damage(), img);

				hero.weapon.registerShotFired();
				if (hero.weapon.equipped == WEAPON_PISTOL) playPistolSfx();
				else                                       playAk47Sfx();
			}
		}

		if (hero.isAttacking) {
			bool hitsNow = hero.updateAttack();
			// Only the knife still lands through the instant melee hitbox -
			// pistol/AK47 damage now comes from the bullet hitting something
			// (see updateSqBullets() below).
			if (hitsNow && hero.weapon.equipped == WEAPON_KNIFE) {
				int x1, y1, x2, y2;
				hero.attackHitbox(x1, y1, x2, y2);
				for (int i = 0; i < SQ_PLATFORMS; i++) {
					if (!sqFlyers[i].alive) continue;
					if (rectsOverlap(x1, y1, x2 - x1, y2 - y1,
						(int)sqFlyers[i].x, (int)sqFlyers[i].y, SQ_FLYER_W, SQ_FLYER_H))
						sqFlyers[i].alive = false;
				}
				// Knife still lands on the walkers through the same hitbox
				// the drones use.
				sqDamageWalkerAt(x1, y1, x2 - x1, y2 - y1, hero.weapon.damage());
			}
		}

		updateSqBullets();
	}

	// --- switch interaction ---
	int sx, sy, sw, sh;
	sqSwitchRect(sx, sy, sw, sh);
	bool nearSwitch = rectsOverlap(hero.posX - SQ_INTERACT_RADIUS, hero.posY - SQ_INTERACT_RADIUS,
		hero.WIDTH + SQ_INTERACT_RADIUS * 2, hero.HEIGHT + SQ_INTERACT_RADIUS * 2, sx, sy, sw, sh);

	if (nearSwitch && !sqIceIntact() && !sqSwitchThrown &&
		(isKeyPressed('e') || isKeyPressed('E'))) {
		sqSwitchThrown = true;
		sqOxygen = 100.0f;
		sqOxygenHalted = true;      // life support restored, drain stops
		sqDronesActive = false;     // air defences go quiet
		hero.currentHP = heroMaxHP; // full heal
		sideQuestComplete = true;

		// ---- MESSAGE ORDER ----------------------------------------------
		// The reward comes first, the status report second. Throwing the
		// switch used to fire BOTH banners on the same tick, so the Space
		// Stone pickup - the actual payoff of the whole quest - was drawn
		// underneath the life-support message and read as clutter.
		//
		// Now:  [Space Stone secured]  ->  short beat  ->  [oxygen restored]
		// and only after BOTH have finished does the auto-return to Level 2
		// fire, so neither message is ever cut off by the screen changing.
		//
		// Only the first completion awards the stone; on a replay there is no
		// reward to announce, so the oxygen banner starts immediately.
		if (!hasSpaceStone) {
			hasSpaceStone = true;

			// The stone also raises the hero's max HP to HERO_STONE_HP (500)
			// and fills the bar; Health.hpp swaps to healthBar2 by itself
			// once heroMaxHP is above the base value. First completion only
			// - a replay never gets here, so it can't stack.
			applySpaceStoneHPBoost(hero);

			// Securing the stone is what unlocks the Plasma Rifle (key
			// '5' checks hasSpaceStone directly - see Playercontroller.hpp).
			// It used to unlock completely silently, so a player could
			// finish the quest and never learn they'd gained a weapon.
			// It now announces itself twice, the same way every other
			// weapon does: once on the big Space Stone banner below (see
			// drawSpaceStoneToast() in GameFlow.hpp) and once as the
			// standard reward toast, which is still on screen when the
			// auto-return drops the player back into Level 2 - the exact
			// same showRewardToast() every puzzle weapon uses (see
			// grantAK47AndKey()/grantFlamethrowerAndKey() in
			// PuzzleCommon.hpp).
			showRewardToast("PLASMA RIFLE UNLOCKED  [5]");

			spaceStoneToastTicks = SQ_STONE_TOAST_TICKS;   // message 1, now
			sqDoneToastTicks = 0;
			sqDoneToastDelay = SQ_STONE_TOAST_TICKS + SQ_TOAST_GAP_TICKS;   // message 2, queued
			sqReturnTicks = SQ_STONE_TOAST_TICKS + SQ_TOAST_GAP_TICKS + SQ_DONE_TOAST_TICKS;
		}
		else {
			sqDoneToastTicks = SQ_DONE_TOAST_TICKS;
			sqDoneToastDelay = 0;
			sqReturnTicks = SQ_DONE_TOAST_TICKS;
		}
	}

	if (sqReturnTicks > 0) {
		sqReturnTicks--;
		if (sqReturnTicks == 0) { exitSideQuest(); return; }
	}

	// --- drones ---
	for (int i = 0; i < SQ_PLATFORMS; i++) {
		SqFlyer& f = sqFlyers[i];
		if (!f.alive) continue;

		// Lazy patrol arc around its home point.
		f.phase += 0.035f;
		f.x = f.homeX + 110.0f * (float)sin(f.phase);
		f.y = f.homeY + 26.0f * (float)sin(f.phase * 1.7f);

		if (f.attackCooldown > 0) f.attackCooldown--;

		if (sqDronesActive && f.attackCooldown == 0) {
			if (rectsOverlap((int)f.x - SQ_FLYER_RANGE, (int)f.y - SQ_FLYER_RANGE,
				SQ_FLYER_W + SQ_FLYER_RANGE * 2, SQ_FLYER_H + SQ_FLYER_RANGE * 2,
				hero.posX, hero.posY, hero.WIDTH, hero.HEIGHT)) {
				hero.takeDamage(SQ_FLYER_DMG);
				f.attackCooldown = SQ_FLYER_ATTACK_INTERVAL;
			}
		}
	}

	// --- inherited water level ---
	float waterTop = floodProgress() * SQ_WATER_MAX_HEIGHT;
	if (hero.posY < waterTop) {
		sqWaterDamageTick++;
		if (sqWaterDamageTick >= SQ_WATER_DAMAGE_INTERVAL) {
			sqWaterDamageTick = 0;
			hero.takeDamage(SQ_WATER_DAMAGE);
		}
	}
	else {
		sqWaterDamageTick = 0;
	}
}

// ---------------------------------------------------------------- drawing --
inline void sqDrawPlatform(int i) {
	const SqPlatform& p = sqPlatforms[i];
	for (int t = 0; t < p.tiles; t++) {
		unsigned int img = imgPlatformMid;
		if (t == 0) img = imgPlatformLeft;
		else if (t == p.tiles - 1) img = imgPlatformRight;

		if (img != (unsigned int)-1)
			iShowImage(p.x + t * SQ_TILE, p.y, SQ_TILE, SQ_TILE, img);
		else {
			iSetColor(120, 120, 135);
			iFilledRectangle(p.x + t * SQ_TILE, p.y, SQ_TILE, SQ_TILE);
		}
	}
}

// Built entirely from iGraphics primitives, as specified: metal console,
// circular core, and either a cyan ice casing or a live golden glow.
inline void sqDrawSwitch() {
	int x, y, w, h;
	sqSwitchRect(x, y, w, h);
	float p = fxPulse();

	// Base console
	iSetColor(52, 56, 66);
	iFilledRectangle(x, y, w, h);
	iSetColor(90, 96, 110);
	iRectangle(x, y, w, h);
	iSetColor(34, 37, 45);
	iFilledRectangle(x + 6, y + 6, w - 12, 10);

	// Mounting struts
	iSetColor(70, 74, 86);
	iLine(x + 10, y, x + 4, y - 14);
	iLine(x + w - 10, y, x + w - 4, y - 14);

	int cx = x + w / 2, cy = y + h / 2 + 6;

	if (sqSwitchThrown) {
		// Live: neon green/gold core with a breathing halo.
		iSetColor(255, 215, 90);
		iFilledCircle(cx, cy, 15 + 2 * p, 26);
		iSetColor(120, 255, 170);
		iCircle(cx, cy, 20 + 4 * p, 26);
		fxFilledRectA((float)x - 6, (float)y - 6, (float)w + 12, (float)h + 12,
			140, 255, 170, 0.20f + 0.25f * p);
	}
	else if (sqIceIntact()) {
		// Dormant core behind a semi-transparent ice block.
		iSetColor(70, 140, 170);
		iFilledCircle(cx, cy, 14, 26);
		fxFilledRectA((float)x - 8, (float)y - 4, (float)w + 16, (float)h + 24,
			120, 220, 255, 0.42f);
		fxFilledRectA((float)x - 8, (float)y - 4, (float)w + 16, 10.0f, 220, 250, 255, 0.55f);
		iSetColor(190, 235, 255);
		double fx[5] = { x - 8.0, x + w / 2.0, x + w + 8.0, x + w + 2.0, x - 2.0 };
		double fy[5] = { y - 4.0, y + h + 26.0, y - 4.0, y - 4.0, y - 4.0 };
		iPolygon(fx, fy, 3);

		// Ice integrity bar
		float frac = sqIceHP / (float)SQ_ICE_MAX_HP;
		iSetColor(20, 30, 40);
		iFilledRectangle(x - 10, y + h + 34, w + 20, 8);
		fxFilledRectA((float)(x - 10), (float)(y + h + 34), (w + 20) * frac, 8.0f, 150, 230, 255, 0.9f);
	}
	else {
		// Melted, not yet thrown - waiting for [E].
		iSetColor(200, 180, 90);
		iFilledCircle(cx, cy, 14, 26);
		iSetColor(255, 230, 140);
		iCircle(cx, cy, 19 + 3 * p, 26);
	}
}

inline void sqDrawFlyer(const SqFlyer& f) {
	if (!f.alive) return;
	float p = fxPulse();
	iSetColor(sqDronesActive ? 190 : 110, 70, 90);
	iFilledRectangle(f.x, f.y, SQ_FLYER_W, SQ_FLYER_H);
	iSetColor(40, 20, 30);
	iRectangle(f.x, f.y, SQ_FLYER_W, SQ_FLYER_H);
	iSetColor(sqDronesActive ? 255 : 90, sqDronesActive ? 90 : 160, 120);
	iFilledCircle(f.x + SQ_FLYER_W / 2, f.y + SQ_FLYER_H / 2, 7, 16);
	// Rotor blur
	iSetColor(210, 210, 230);
	iLine(f.x - 8, f.y + SQ_FLYER_H + 4, f.x + SQ_FLYER_W + 8, f.y + SQ_FLYER_H + 4);
	fxFilledRectA(f.x, f.y, (float)SQ_FLYER_W, (float)SQ_FLYER_H,
		sqDronesActive ? 255 : 80, 60, 90, 0.10f + 0.16f * p);
}

inline void drawSideQuestPage() {
	initSideQuestAssets();

	if (imgSqBackground != (unsigned int)-1)
		iShowImage(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, imgSqBackground);
	else {
		iSetColor(18, 14, 30);
		iFilledRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	}

	// Water inherited from the Level 2 flood clock.
	float waterTop = floodProgress() * SQ_WATER_MAX_HEIGHT;
	if (waterTop > 1.0f) {
		fxFilledRectA(0, 0, (float)SCREEN_WIDTH, waterTop, 40, 120, 200, 0.45f);
		fxFilledRectA(0, waterTop - 8.0f, (float)SCREEN_WIDTH, 8.0f,
			150, 230, 255, 0.35f + 0.30f * fxPulse());
	}

	for (int i = 0; i < SQ_PLATFORMS; i++) sqDrawPlatform(i);
	sqDrawSwitch();
	for (int i = 0; i < SQ_PLATFORMS; i++) sqDrawFlyer(sqFlyers[i]);
	for (int i = 0; i < SQ_WALKERS; i++) sqDrawWalker(sqWalkers[i]);

	int heroImg = hero.currentImage();
	if (heroImg != -1) iShowImage(hero.posX, hero.posY, hero.WIDTH, hero.HEIGHT, heroImg);
	else {
		iSetColor(0, 255, 255);
		iFilledRectangle(hero.posX, hero.posY, hero.WIDTH, hero.HEIGHT);
	}

	// FIX: bullets were spawning (once updateSqBullets() above actually
	// exists) but never drawn - drawSideQuestPage() never called
	// drawBullets(). No camera offset here (0, 0), same as the hero draw
	// just above - this page doesn't scroll.
	drawBullets(0.0f, 0.0f);

	drawHealthBar(hero);
	drawWeaponHUD(hero);

	// --- oxygen gauge ---
	char line[80];
	float oxFrac = sqOxygen / 100.0f;
	iSetColor(20, 26, 34);
	iFilledRectangle(SCREEN_WIDTH / 2 - 160, SCREEN_HEIGHT - 52, 320, 20);
	fxFilledRectA((float)(SCREEN_WIDTH / 2 - 160), (float)(SCREEN_HEIGHT - 52), 320.0f * oxFrac, 20.0f,
		oxFrac < 0.25f ? 255 : 90, oxFrac < 0.25f ? 70 : 220, 255, 0.85f);
	iSetColor(150, 200, 230);
	iRectangle(SCREEN_WIDTH / 2 - 160, SCREEN_HEIGHT - 52, 320, 20);
	sprintf_s(line, sizeof(line), "OXYGEN  %d%%", (int)sqOxygen);
	iSetColor(255, 255, 255);
	iText(SCREEN_WIDTH / 2 - 55, SCREEN_HEIGHT - 47, line, GLUT_BITMAP_HELVETICA_18);

	// --- flood clock (shared with Level 2) ---
	sprintf_s(line, sizeof(line), "FLOOD  %d:%02d", floodSecondsLeft() / 60, floodSecondsLeft() % 60);
	iSetColor(255, 200, 90);
	iText(SCREEN_WIDTH - 170, SCREEN_HEIGHT - 47, line, GLUT_BITMAP_HELVETICA_18);

	// --- proximity / state toasts ---
	int sx, sy, sw, sh;
	sqSwitchRect(sx, sy, sw, sh);
	bool nearSwitch = rectsOverlap(hero.posX - SQ_INTERACT_RADIUS, hero.posY - SQ_INTERACT_RADIUS,
		hero.WIDTH + SQ_INTERACT_RADIUS * 2, hero.HEIGHT + SQ_INTERACT_RADIUS * 2, sx, sy, sw, sh);

	if (nearSwitch && sqIceIntact()) {
		// Floating toast that rides just above the hero's head.
		iSetColor(150, 230, 255);
		iText(hero.posX - 10, hero.posY + hero.HEIGHT + 18, (char*)"It's Frozen!", GLUT_BITMAP_HELVETICA_18);
		iSetColor(200, 230, 255);
		iText(sx - 60, sy + sh + 60, (char*)"BURN IT WITH THE FLAMETHROWER  [4]", GLUT_BITMAP_HELVETICA_18);
	}
	else if (nearSwitch && !sqIceIntact() && !sqSwitchThrown) {
		iSetColor(255, 230, 140);
		iText(hero.posX - 20, hero.posY + hero.HEIGHT + 18, (char*)"[E] THROW SWITCH", GLUT_BITMAP_HELVETICA_18);
	}

	if (sqMeltedToastTicks > 0) {
		iSetColor(140, 255, 200);
		iText(sx - 20, sy + sh + 30, (char*)"Melted!", GLUT_BITMAP_TIMES_ROMAN_24);
	}

	if (sqDoneToastTicks > 0) {
		fxFilledRectA(0, (float)(SCREEN_HEIGHT / 2 - 50), (float)SCREEN_WIDTH, 100.0f, 20, 90, 60, 0.55f);
		iSetColor(160, 255, 200);
		iText(SCREEN_WIDTH / 2 - 250, SCREEN_HEIGHT / 2,
			(char*)"SWITCH ONLINE - OXYGEN RESTORED, DRONES GROUNDED", GLUT_BITMAP_TIMES_ROMAN_24);
	}

	// --- 1-minute warning banner, pulsing ---
	if (sqWarnFlashTicks > 0) {
		float p = 0.5f + 0.5f * (float)sin(sqTicks * 0.35);
		fxFilledRectA(0, (float)(SCREEN_HEIGHT / 2 + 90), (float)SCREEN_WIDTH, 90.0f,
			200, 30, 30, 0.25f + 0.35f * p);
		iSetColor(255, 200 - (int)(120 * p), 200 - (int)(120 * p));
		iText(SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 + 125, (char*)"FIX THE SWITCH", GLUT_BITMAP_TIMES_ROMAN_24);
	}

	iSetColor(190, 200, 220);
	iText(20, 20, (char*)"[A/D] MOVE   [W] JUMP   [SPACE] FIRE   [E] SWITCH   [ESC] LEAVE QUEST",
		GLUT_BITMAP_HELVETICA_18);
}

#endif