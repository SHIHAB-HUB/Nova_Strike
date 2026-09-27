#pragma once


#define ENEMY_TYPE_COUNT   8
#define MAX_FRAMES         13


enum EnemyMoveType { ENEMY_MOVE_WALK = 0, ENEMY_MOVE_FLY = 1 };


// Types 5/6/7 (Enemy 6/7/8) are all ground walkers - their art only has
// walk_N/attack_N folders, no fly_N folder like Enemy 4 has.
static const EnemyMoveType ENEMY_MOVE_TYPE[ENEMY_TYPE_COUNT] =
{
	ENEMY_MOVE_WALK, ENEMY_MOVE_WALK, ENEMY_MOVE_WALK, ENEMY_MOVE_FLY, ENEMY_MOVE_WALK,
	ENEMY_MOVE_WALK, ENEMY_MOVE_WALK, ENEMY_MOVE_WALK
};



// ms an enemy walks away before turning back 
static const long ENEMY_WALK_MS[ENEMY_TYPE_COUNT] = { 3000, 3000, 1000, 1000, 3000, 2000, 3000, 1500 };


static const float ENEMY_SPEED_PX_PER_SEC[ENEMY_TYPE_COUNT] = { 60.0f, 50.0f, 50.0f, 90.0f, 40.0f, 45.0f, 35.0f, 70.0f };

// ms to hold each animation frame before advancing
static const int ENEMY_ANIM_DELAY_MS[ENEMY_TYPE_COUNT] = { 220, 160, 180, 200, 90, 180, 200, 140 };

// ms to hold each ATTACK frame 
static const int ENEMY_ATTACK_ANIM_DELAY_MS[ENEMY_TYPE_COUNT] = { 100, 150, 200, 300, 250, 200, 220, 120 };

// walk/fly frame count - matches how many walk_N frames exist on disk for
// each type (enemy_6: 6, enemy_7: 4, enemy_8: 4 - see Enemy_Images/enemy_N/walk_N/)
static const int ENEMY_FRAME_COUNT[ENEMY_TYPE_COUNT] = { 2, 4, 3, 3, 2, 6, 4, 4 };


// Enemy 7 is the new heavy tank (biggest sprite, slowest, most HP), Enemy 8
// is the new fast/fragile skirmisher, Enemy 6 sits in between.
static const int ENEMY_MAX_HEALTH[ENEMY_TYPE_COUNT] = { 100, 200, 500, 1000, 1000, 300, 700, 80 };


// Enemy 6/7/8 sizes taken directly from their walk sprite's pixel dimensions
// (Enemy_Images/enemy_N/walk_N/*_right_1.png): 112x106, 140x140, 66x53.
static const int ENEMY_TYPE_WIDTH[ENEMY_TYPE_COUNT] = { 34 * 1.5, 65, 125, 63 * 2, 70, 112 * 1.1, 140 * 1.05, 66 * 1.7 };
static const int ENEMY_TYPE_HEIGHT[ENEMY_TYPE_COUNT] = { 27 * 1.5, 56, 101, 73 * 2, 90, 106 * 1.1, 140 * 1.05, 53 * 1.7 };

// How far (in pixels) enemy notices hero and starts chase
static const int ENEMY_ATTACK_RANGE_PX[ENEMY_TYPE_COUNT] = { 120, 150, 180, 130, 220, 160, 190, 100 };


// Attack frame counts match how many attack_N frames exist on disk
// (enemy_6: 5, enemy_7: 7, enemy_8: 4 - see Enemy_Images/enemy_N/attack_N/)
static const int ENEMY_ATTACK_FRAME_COUNT[ENEMY_TYPE_COUNT] = { 4, 4, 5, 2, 13, 5, 7, 4 };

// --- enemy attack damage ---------------------------------------------------
// SECOND PASS (was 5/12/18/25/32/40/45/10). Playtesting said the mid and
// upper tiers still spiked too hard, so the whole curve is pulled down by
// roughly a quarter while Hero.hpp's max HP goes UP from 180 to 230. The two
// changes compound: every tier now takes noticeably longer to kill you, and
// the heavy enemies stop being one-mistake-and-you're-dead.
//
// Hits-to-kill from full health against the new HP (230):
//   Enemy 1 ( 4) -> 57 hits   trash mob, safe to trade with
//   Enemy 2 ( 9) -> 25 hits   (was 15)
//   Enemy 3 (14) -> 16 hits   (was 10)
//   Enemy 4 (19) -> 12 hits   (was  7)  flying, harder to dodge
//   Enemy 5 (24) ->  9 hits   (was  6)
//   Enemy 6 (30) ->  7 hits   (was  5)
//   Enemy 7 (34) ->  6 hits   (was  4)  side-quest mini-boss - still the
//                             biggest threat, just no longer a 4-hit wipe
//   Enemy 8 ( 8) -> 28 hits   fastest/most fragile; low damage offsets how
//                             quickly it closes distance
//
// The ORDERING of the tiers is untouched on purpose - enemy 7 is still the
// hardest hitter and enemy 1 the softest, so the difficulty design still
// reads exactly as intended. Only the absolute numbers moved, which is what
// keeps the rebalance proportional instead of flattening the roster.
static const int ENEMY_ATTACK_DMG[ENEMY_TYPE_COUNT] = { 4, 9, 14, 19, 24, 30, 34, 8 };

// Attack starting frame
static const int ENEMY_ATTACK_HIT_FRAME[ENEMY_TYPE_COUNT] = { 2, 2, 2, 1, 7, 2, 3, 1 };


static const int ENEMY_ATTACK_TRIGGER_GAP_PX[ENEMY_TYPE_COUNT] = { 0, 0, 0, 0, 0, 0, 0, 0 };

// How many pixels the hero and this enemy type are allowed to step into
// each other (sideways) before they act like a solid wall and stop.
//   Small number (e.g. 10)  -> blocks almost as soon as they touch.
//   Big number (e.g. 40)    -> lets them overlap more before blocking,
//                               so it feels a bit "softer".
// Used by overlapsEnemy() in level.hpp (hero walking into an enemy) and by
// updateEnemies() in enemy.h (an enemy walking into the hero).
static const int ENEMY_WALL_OVERLAP_PX[ENEMY_TYPE_COUNT] = { 15, 15, 15, 15, 15, 15, 15, 15 };


// Pixels a hit enemy gets shoved away - attackinthebox
const int ENEMY_KNOCKBACK_PX = 50;

// enemy disappears after being hit for
const long ENEMY_HIT_FLASH_MS = 90;


static const int ENEMY_ATTACK_WIDTH[ENEMY_TYPE_COUNT] = { ENEMY_TYPE_WIDTH[0], ENEMY_TYPE_WIDTH[1], ENEMY_TYPE_WIDTH[2], ENEMY_TYPE_WIDTH[3], ENEMY_TYPE_WIDTH[4], ENEMY_TYPE_WIDTH[5], ENEMY_TYPE_WIDTH[6], ENEMY_TYPE_WIDTH[7] };
static const int ENEMY_ATTACK_HEIGHT[ENEMY_TYPE_COUNT] = { ENEMY_TYPE_HEIGHT[0], ENEMY_TYPE_WIDTH[1], ENEMY_TYPE_HEIGHT[2], ENEMY_TYPE_HEIGHT[3], ENEMY_TYPE_HEIGHT[4], ENEMY_TYPE_HEIGHT[5], ENEMY_TYPE_HEIGHT[6], ENEMY_TYPE_HEIGHT[7] };


// --- Scoring (Header/enemy.h's healHeroOnKill() reads this on every kill) ---
// Points awarded to the player for killing one enemy of this type, by index:
//   0: Enemy 1, 1: Enemy 2, 2: Enemy 3, 3: Enemy 4, 4: Enemy 5,
//   5: Enemy 6, 6: Enemy 7, 7: Enemy 8
// The 3 gate-guarding types (Enemy 3/4/5 - see spawnSpecialEnemies() in
// Header/enemy.h, stationed right under the "1 7 1"/"1 6 1" door markers)
// score 5/15/40 in ascending order of how dangerous they are to fight
// (Enemy 3 = lone ground melee, Enemy 4 = flying and harder to hit,
// Enemy 5 = the slow heavy tank with the highest attack damage of the
// three). Enemy 8 - the smallest, fastest, most fragile type (see the
// ENEMY_TYPE_WIDTH/HEIGHT and "fast/fragile skirmisher" comment above) -
// is the hardest to actually land a hit on, so it scores the most (100)
// and also fully restores the hero's HP on kill (see healHeroOnKill()).
// The remaining types (1/2/6/7) aren't singled out by name anywhere else
// in the game, so they're just scaled to roughly match ENEMY_MAX_HEALTH -
// tweak any of these freely, they're independent of every other array here.
static const int ENEMY_SCORE_VALUE[ENEMY_TYPE_COUNT] = { 5, 10, 5, 15, 40, 20, 50, 100 };

// Only Enemy 8 (index 7) fully heals the hero on kill - see
// healHeroOnKill() in Header/enemy.h. Enemy 6/7 (index 5/6) still get their
// existing partial (5%/10%) heal from healHeroOnKill()'s own logic below.
static const bool ENEMY_FULL_HEAL_ON_KILL[ENEMY_TYPE_COUNT] = { false, false, false, false, false, false, false, true };


//walk pic

static const char* LEFT_PIC_PATH[ENEMY_TYPE_COUNT][MAX_FRAMES] =
{
	{ "Enemy_Images/enemy_1/walk_1/walk_left_1.png", "Enemy_Images/enemy_1/walk_1/walk_left_2.png", "", "", "", "", "", "", "", "", "", "", "" },
	{ "Enemy_Images/enemy_2/walk_2/enemy_2_walk_left_1.png", "Enemy_Images/enemy_2/walk_2/enemy_2_walk_left_2.png", "Enemy_Images/enemy_2/walk_2/enemy_2_walk_left_3.png", "Enemy_Images/enemy_2/walk_2/enemy_2_walk_left_4.png", "", "", "", "", "", "", "", "", "" },
	{ "Enemy_Images/enemy_3/walk_3/enemy_3_walk_left_1.png", "Enemy_Images/enemy_3/walk_3/enemy_3_walk_left_2.png", "Enemy_Images/enemy_3/walk_3/enemy_3_walk_left_3.png", "", "", "", "", "", "", "", "", "", "" },
	{ "Enemy_Images/enemy_4/fly_4/enemy_4_fly_left_1.png", "Enemy_Images/enemy_4/fly_4/enemy_4_fly_left_2.png", "Enemy_Images/enemy_4/fly_4/enemy_4_fly_left_3.png", "", "", "", "", "", "", "", "", "", "" },
	{ "Enemy_Images/enemy_5/walk_5/enemy_5_walk_left_1.png", "Enemy_Images/enemy_5/walk_5/enemy_5_walk_left_2.png", "", "", "", "", "", "", "", "", "", "", "" },
	{ "Enemy_Images/enemy_6/walk_6/enemy_6_walk_left_1.png", "Enemy_Images/enemy_6/walk_6/enemy_6_walk_left_2.png", "Enemy_Images/enemy_6/walk_6/enemy_6_walk_left_3.png", "Enemy_Images/enemy_6/walk_6/enemy_6_walk_left_4.png", "Enemy_Images/enemy_6/walk_6/enemy_6_walk_left_5.png", "Enemy_Images/enemy_6/walk_6/enemy_6_walk_left_6.png", "", "", "", "", "", "", "" },
	{ "Enemy_Images/enemy_7/walk_7/enemy_7_walk_left_1.png", "Enemy_Images/enemy_7/walk_7/enemy_7_walk_left_2.png", "Enemy_Images/enemy_7/walk_7/enemy_7_walk_left_3.png", "Enemy_Images/enemy_7/walk_7/enemy_7_walk_left_4.png", "", "", "", "", "", "", "", "", "" },
	{ "Enemy_Images/enemy_8/walk_8/enemy_8_walk_left_1.png", "Enemy_Images/enemy_8/walk_8/enemy_8_walk_left_2.png", "Enemy_Images/enemy_8/walk_8/enemy_8_walk_left_3.png", "Enemy_Images/enemy_8/walk_8/enemy_8_walk_left_4.png", "", "", "", "", "", "", "", "", "" }
};
static const char* RIGHT_PIC_PATH[ENEMY_TYPE_COUNT][MAX_FRAMES] =
{
	{ "Enemy_Images/enemy_1/walk_1/walk_right_1.png", "Enemy_Images/enemy_1/walk_1/walk_right_2.png", "", "", "", "", "", "", "", "", "", "", "" },
	{ "Enemy_Images/enemy_2/walk_2/enemy_2_walk_right_1.png", "Enemy_Images/enemy_2/walk_2/enemy_2_walk_right_2.png", "Enemy_Images/enemy_2/walk_2/enemy_2_walk_right_3.png", "Enemy_Images/enemy_2/walk_2/enemy_2_walk_right_4.png", "", "", "", "", "", "", "", "", "" },
	{ "Enemy_Images/enemy_3/walk_3/enemy_3_walk_right_1.png", "Enemy_Images/enemy_3/walk_3/enemy_3_walk_right_2.png", "Enemy_Images/enemy_3/walk_3/enemy_3_walk_right_3.png", "", "", "", "", "", "", "", "", "", "" },
	{ "Enemy_Images/enemy_4/fly_4/enemy_4_fly_right_1.png", "Enemy_Images/enemy_4/fly_4/enemy_4_fly_right_2.png", "Enemy_Images/enemy_4/fly_4/enemy_4_fly_right_3.png", "", "", "", "", "", "", "", "", "", "" },
	{ "Enemy_Images/enemy_5/walk_5/enemy_5_walk_right_1.png", "Enemy_Images/enemy_5/walk_5/enemy_5_walk_right_2.png", "", "", "", "", "", "", "", "", "", "", "" },
	{ "Enemy_Images/enemy_6/walk_6/enemy_6_walk_right_1.png", "Enemy_Images/enemy_6/walk_6/enemy_6_walk_right_2.png", "Enemy_Images/enemy_6/walk_6/enemy_6_walk_right_3.png", "Enemy_Images/enemy_6/walk_6/enemy_6_walk_right_4.png", "Enemy_Images/enemy_6/walk_6/enemy_6_walk_right_5.png", "Enemy_Images/enemy_6/walk_6/enemy_6_walk_right_6.png", "", "", "", "", "", "", "" },
	{ "Enemy_Images/enemy_7/walk_7/enemy_7_walk_right_1.png", "Enemy_Images/enemy_7/walk_7/enemy_7_walk_right_2.png", "Enemy_Images/enemy_7/walk_7/enemy_7_walk_right_3.png", "Enemy_Images/enemy_7/walk_7/enemy_7_walk_right_4.png", "", "", "", "", "", "", "", "", "" },
	{ "Enemy_Images/enemy_8/walk_8/enemy_8_walk_right_1.png", "Enemy_Images/enemy_8/walk_8/enemy_8_walk_right_2.png", "Enemy_Images/enemy_8/walk_8/enemy_8_walk_right_3.png", "Enemy_Images/enemy_8/walk_8/enemy_8_walk_right_4.png", "", "", "", "", "", "", "", "", "" }
};

//attack pic

static const char* ATTACK_LEFT_PIC_PATH[ENEMY_TYPE_COUNT][MAX_FRAMES] =
{
	{ "Enemy_Images/enemy_1/attack_1/enemy_1_attack_left_1.png", "Enemy_Images/enemy_1/attack_1/enemy_1_attack_left_2.png", "Enemy_Images/enemy_1/attack_1/enemy_1_attack_left_3.png", "Enemy_Images/enemy_1/attack_1/enemy_1_attack_left_4.png", "", "", "", "", "", "", "", "", "" },
	{ "Enemy_Images/enemy_2/attack_2/enemy_2_attack_left_1.png", "Enemy_Images/enemy_2/attack_2/enemy_2_attack_left_2.png", "Enemy_Images/enemy_2/attack_2/enemy_2_attack_left_3.png", "Enemy_Images/enemy_2/attack_2/enemy_2_attack_left_4.png", "", "", "", "", "", "", "", "", "" },
	{ "Enemy_Images/enemy_3/attack_3/enemy_3_attack_left_1.png", "Enemy_Images/enemy_3/attack_3/enemy_3_attack_left_2.png", "Enemy_Images/enemy_3/attack_3/enemy_3_attack_left_3.png", "Enemy_Images/enemy_3/attack_3/enemy_3_attack_left_4.png", "Enemy_Images/enemy_3/attack_3/enemy_3_attack_left_5.png", "", "", "", "", "", "", "", "" },
	{ "Enemy_Images/enemy_4/attack_4/enemy_4_attack_left_1.png", "Enemy_Images/enemy_4/attack_4/enemy_4_attack_left_2.png", "", "", "", "", "", "", "", "", "", "", "" },
	{ "Enemy_Images/enemy_5/attack_5/enemy_5_left_attack_1.png", "Enemy_Images/enemy_5/attack_5/enemy_5_left_attack_2.png", "Enemy_Images/enemy_5/attack_5/enemy_5_left_attack_3.png", "Enemy_Images/enemy_5/attack_5/enemy_5_left_attack_4.png", "Enemy_Images/enemy_5/attack_5/enemy_5_left_attack_5.png", "Enemy_Images/enemy_5/attack_5/enemy_5_left_attack_6.png", "Enemy_Images/enemy_5/attack_5/enemy_5_left_attack_7.png", "Enemy_Images/enemy_5/attack_5/enemy_5_left_attack_8.png", "Enemy_Images/enemy_5/attack_5/enemy_5_left_attack_9.png", "Enemy_Images/enemy_5/attack_5/enemy_5_left_attack_10.png", "Enemy_Images/enemy_5/attack_5/enemy_5_left_attack_11.png", "Enemy_Images/enemy_5/attack_5/enemy_5_left_attack_12.png", "Enemy_Images/enemy_5/attack_5/enemy_5_left_attack_13.png" },
	{ "Enemy_Images/enemy_6/attack_6/enemy_6_attack_left_1.png", "Enemy_Images/enemy_6/attack_6/enemy_6_attack_left_2.png", "Enemy_Images/enemy_6/attack_6/enemy_6_attack_left_3.png", "Enemy_Images/enemy_6/attack_6/enemy_6_attack_left_4.png", "Enemy_Images/enemy_6/attack_6/enemy_6_attack_left_5.png", "", "", "", "", "", "", "", "" },
	{ "Enemy_Images/enemy_7/attack_7/enemy_7_attack_left_1.png", "Enemy_Images/enemy_7/attack_7/enemy_7_attack_left_2.png", "Enemy_Images/enemy_7/attack_7/enemy_7_attack_left_3.png", "Enemy_Images/enemy_7/attack_7/enemy_7_attack_left_4.png", "Enemy_Images/enemy_7/attack_7/enemy_7_attack_left_5.png", "Enemy_Images/enemy_7/attack_7/enemy_7_attack_left_6.png", "Enemy_Images/enemy_7/attack_7/enemy_7_attack_left_7.png", "", "", "", "", "", "" },
	{ "Enemy_Images/enemy_8/attack_8/enemy_8_attack_left_1.png", "Enemy_Images/enemy_8/attack_8/enemy_8_attack_left_2.png", "Enemy_Images/enemy_8/attack_8/enemy_8_attack_left_3.png", "Enemy_Images/enemy_8/attack_8/enemy_8_attack_left_4.png", "", "", "", "", "", "", "", "", "" }
};
static const char* ATTACK_RIGHT_PIC_PATH[ENEMY_TYPE_COUNT][MAX_FRAMES] =
{
	{ "Enemy_Images/enemy_1/attack_1/enemy_1_attack_right_1.png", "Enemy_Images/enemy_1/attack_1/enemy_1_attack_right_2.png", "Enemy_Images/enemy_1/attack_1/enemy_1_attack_right_3.png", "Enemy_Images/enemy_1/attack_1/enemy_1_attack_right_4.png", "", "", "", "", "", "", "", "", "" },
	{ "Enemy_Images/enemy_2/attack_2/enemy_2_attack_right_1.png", "Enemy_Images/enemy_2/attack_2/enemy_2_attack_right_2.png", "Enemy_Images/enemy_2/attack_2/enemy_2_attack_right_3.png", "Enemy_Images/enemy_2/attack_2/enemy_2_attack_right_4.png", "", "", "", "", "", "", "", "", "" },
	{ "Enemy_Images/enemy_3/attack_3/enemy_3_attack_right_1.png", "Enemy_Images/enemy_3/attack_3/enemy_3_attack_right_2.png", "Enemy_Images/enemy_3/attack_3/enemy_3_attack_right_3.png", "Enemy_Images/enemy_3/attack_3/enemy_3_attack_right_4.png", "Enemy_Images/enemy_3/attack_3/enemy_3_attack_right_5.png", "", "", "", "", "", "", "", "" },
	{ "Enemy_Images/enemy_4/attack_4/enemy_4_attack_right_1.png", "Enemy_Images/enemy_4/attack_4/enemy_4_attack_right_2.png", "", "", "", "", "", "", "", "", "", "", "" },
	{ "Enemy_Images/enemy_5/attack_5/enemy_5_right_attack_1.png", "Enemy_Images/enemy_5/attack_5/enemy_5_right_attack_2.png", "Enemy_Images/enemy_5/attack_5/enemy_5_right_attack_3.png", "Enemy_Images/enemy_5/attack_5/enemy_5_right_attack_4.png", "Enemy_Images/enemy_5/attack_5/enemy_5_right_attack_5.png", "Enemy_Images/enemy_5/attack_5/enemy_5_right_attack_6.png", "Enemy_Images/enemy_5/attack_5/enemy_5_right_attack_7.png", "Enemy_Images/enemy_5/attack_5/enemy_5_right_attack_8.png", "Enemy_Images/enemy_5/attack_5/enemy_5_right_attack_9.png", "Enemy_Images/enemy_5/attack_5/enemy_5_right_attack_10.png", "Enemy_Images/enemy_5/attack_5/enemy_5_right_attack_11.png", "Enemy_Images/enemy_5/attack_5/enemy_5_right_attack_12.png", "Enemy_Images/enemy_5/attack_5/enemy_5_right_attack_13.png" },
	{ "Enemy_Images/enemy_6/attack_6/enemy_6_attack_right_1.png", "Enemy_Images/enemy_6/attack_6/enemy_6_attack_right_2.png", "Enemy_Images/enemy_6/attack_6/enemy_6_attack_right_3.png", "Enemy_Images/enemy_6/attack_6/enemy_6_attack_right_4.png", "Enemy_Images/enemy_6/attack_6/enemy_6_attack_right_5.png", "", "", "", "", "", "", "", "" },
	{ "Enemy_Images/enemy_7/attack_7/enemy_7_attack_right_1.png", "Enemy_Images/enemy_7/attack_7/enemy_7_attack_right_2.png", "Enemy_Images/enemy_7/attack_7/enemy_7_attack_right_3.png", "Enemy_Images/enemy_7/attack_7/enemy_7_attack_right_4.png", "Enemy_Images/enemy_7/attack_7/enemy_7_attack_right_5.png", "Enemy_Images/enemy_7/attack_7/enemy_7_attack_right_6.png", "Enemy_Images/enemy_7/attack_7/enemy_7_attack_right_7.png", "", "", "", "", "", "" },
	{ "Enemy_Images/enemy_8/attack_8/enemy_8_attack_right_1.png", "Enemy_Images/enemy_8/attack_8/enemy_8_attack_right_2.png", "Enemy_Images/enemy_8/attack_8/enemy_8_attack_right_3.png", "Enemy_Images/enemy_8/attack_8/enemy_8_attack_right_4.png", "", "", "", "", "", "", "", "", "" }
};