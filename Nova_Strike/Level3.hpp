// =====================================================
// Level3.hpp
// Owner: Mahdin (Map Design)
//
// LEVEL 3 - "Planet Titan : Endgame"
// The boss arena. Deliberately the simplest map in the game, because the
// fight is the content: one long, unbroken floor, no platforming, no
// environmental timer, nothing to fall off.
//
// Two things make it different from Levels 1/2:
//   1. The background is STATIC - it does not parallax or scroll. It is a
//      painted skybox of Titan, drawn once at (0,0) filling the screen.
//      Level 1/2 scroll their backgrounds; this one deliberately doesn't,
//      so the arena reads as a fixed stage rather than a corridor.
//   2. The floor is a repeating 5-tile sequence (Ground_1..Ground_5, then
//      back to Ground_1) laid edge-to-edge with no gaps, exactly like the
//      11/12/13 chains in Level 2 but without end-caps, since the floor
//      runs the entire width of the arena.
// =====================================================
// !! INCLUDE ORDER MATTERS !!
// Must be #included AFTER "level.hpp" - it uses TILE_SIZE, cameraX/cameraY
// and drawHealthBar(), all of which live in that chain. It does not include
// level.hpp itself to avoid a circular include.
#ifndef LEVEL3_HPP
#define LEVEL3_HPP
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include "Variables.h"
#include "Header\Hero.hpp"
#include "Header\thanos.hpp"  // the boss himself - already pulled in via level.hpp too, included directly here as well since this file leans on it directly (spawnThanos()/updateThanos()/drawThanos())
#include "SoundManager.hpp"   // playLevel3BGM() / stopBGM()
#include "GameFlow.hpp"       // showLevelTitle()

// Same forward-declaration convention Playercontroller.hpp/SideQuest.hpp/
// SpaceStone.hpp already use for direct key polling - the real definition
// lives in iGraphics.h (included well before this file via iMain.cpp), this
// just guarantees the prototype is visible regardless of include order.
int isKeyPressed(unsigned char key);

// The arena is wider than one screen so the fight has room to kite, but
// nowhere near the 64-column sprawl of Levels 1/2 - a boss arena wants the
// player and the boss on screen together most of the time.
// ONE SCREEN WIDE on purpose. The arena used to be 48 columns with a
// following camera, which is what made the floor appear to "slide left"
// as the hero crossed the midpoint - that was the camera scrolling, not
// the platform moving. A boss arena wants a fixed stage: hero and Thanos
// both on screen at all times, nothing sliding underfoot.
#define L3_ARENA_COLS (SCREEN_WIDTH / TILE_SIZE)
#define L3_ARENA_WIDTH (L3_ARENA_COLS * TILE_SIZE)
#define L3_FLOOR_ROWS 3                       // floor thickness in tiles
#define L3_FLOOR_TOP_Y (L3_FLOOR_ROWS * TILE_SIZE)
#define L3_GROUND_VARIANTS 5

// Score-on-kill and every other Thanos constant now live in
// Header/thanos.hpp (THANOS_SCORE_VALUE etc.) - see that file.

__declspec(selectany) unsigned int imgL3Background = (unsigned int)-1;
__declspec(selectany) unsigned int imgL3Ground[L3_GROUND_VARIANTS] = {
	(unsigned int)-1, (unsigned int)-1, (unsigned int)-1, (unsigned int)-1, (unsigned int)-1
};
__declspec(selectany) bool level3AssetsLoaded = false;

// Same file-probe guard used everywhere else in this project: iLoadImage()
// hands back a "valid" handle even when the file is missing, so a probe is
// the only way the "!= -1" fallback checks below mean anything.
inline unsigned int l3SafeLoad(const char* path) {
	FILE* probe = NULL;
	fopen_s(&probe, path, "rb");
	if (!probe) return (unsigned int)-1;
	fclose(probe);
	return iLoadImage((char*)path);
}

// One-time-load guard: repeated glGenTextures/glTexImage2D on every restart
// is what destabilizes the Intel integrated driver, so assets load once for
// the whole process lifetime.
inline void initLevel3Assets() {
	if (level3AssetsLoaded) return;
	level3AssetsLoaded = true;

	imgL3Background = l3SafeLoad("Images\\Level_3\\background.jpg");
	imgL3Ground[0] = l3SafeLoad("Images\\Level_3\\Ground_1.png");
	imgL3Ground[1] = l3SafeLoad("Images\\Level_3\\Ground_2.png");
	imgL3Ground[2] = l3SafeLoad("Images\\Level_3\\Ground_3.png");
	imgL3Ground[3] = l3SafeLoad("Images\\Level_3\\Ground_4.png");
	imgL3Ground[4] = l3SafeLoad("Images\\Level_3\\Ground_5.png");

	// BUG FIX: thanos.loadAssets() (Header/thanos.hpp) was written but never
	// actually called anywhere - his running/dying/reviving textures were
	// all sitting as uninitialized garbage ints instead of real texture
	// handles, which is why he wasn't rendering at all. This is the one-time
	// load guard for exactly that reason, so it's the right place for it.
	thanos.loadAssets();
}

// ---------------------------------------------------------------- physics --
// The arena floor is a single solid slab, so collision is one comparison
// rather than a grid sweep - far cheaper than Level 1/2's per-tile loop and
// exactly as correct for a flat stage.
inline bool isSolidAtL3(int x, int y, int w, int h) {
	if (y < L3_FLOOR_TOP_Y && (y + h) > 0) {
		if ((x + w) > 0 && x < L3_ARENA_WIDTH) return true;
	}
	return false;
}

inline bool heroIsSupportedL3() {
	// Same 60%-centered-band trick as Level 2: checking the hero's FULL
	// width lets him hang in the air off a corner, which is the "floating
	// beside blocks" bug that was fixed there.
	int narrowW = (int)(hero.WIDTH * 0.6f);
	int narrowX = hero.posX + (hero.WIDTH - narrowW) / 2;
	return isSolidAtL3(narrowX, hero.posY - 1, narrowW, 1);
}

inline void heroUpdateGravityL3() {
	hero.posY += hero.velocityY;

	// Head-bump: nothing overhead in an open arena, but kept so the physics
	// contract matches Levels 1/2 if a ceiling hazard is ever added.
	if (hero.velocityY > 0 && isSolidAtL3(hero.posX, hero.posY + hero.HEIGHT - 1, hero.WIDTH, 1)) {
		while (isSolidAtL3(hero.posX, hero.posY, hero.WIDTH, hero.HEIGHT)) hero.posY--;
		hero.velocityY = 0;
	}

	hero.velocityY -= GRAVITY;

	int narrowW = (int)(hero.WIDTH * 0.6f);
	int narrowX = hero.posX + (hero.WIDTH - narrowW) / 2;
	if (hero.velocityY <= 0 && isSolidAtL3(narrowX, hero.posY - 1, narrowW, 1)) {
		while (isSolidAtL3(hero.posX, hero.posY, hero.WIDTH, hero.HEIGHT)) hero.posY++;
		hero.velocityY = 0;
		hero.isGrounded = true;
	}
}

inline void heroMoveHorizontalL3(int dir) {
	int prevX = hero.posX;
	hero.moveHorizontal(dir, L3_ARENA_WIDTH);
	// Walls at both ends of the arena so the fight can't be walked out of.
	if (hero.posX < 0) hero.posX = 0;
	if (hero.posX > L3_ARENA_WIDTH - hero.WIDTH) hero.posX = L3_ARENA_WIDTH - hero.WIDTH;
	if (isSolidAtL3(hero.posX, hero.posY, hero.WIDTH, hero.HEIGHT)) hero.posX = prevX;

	// Thanos is a SOLID BODY, not a ghost - the hero used to be able to
	// walk clean through him, which made the fight read as if he weren't
	// really there. Same revert-the-move idiom as the solid-tile check on
	// the line above (and as overlapsEnemy() in level.hpp's own
	// heroMoveHorizontal()): if the step would put the hero inside his
	// hitbox, the step simply doesn't happen.
	//
	// Being blocked by him is completely harmless on its own - his contact
	// damage has been removed (see Header/thanos.hpp). Bumping into him
	// stops the hero and nothing else; only the sword swing hurts.
	if (thanosBlocksHero(hero.posX, hero.posY, hero.WIDTH, hero.HEIGHT)) hero.posX = prevX;
}

// ----------------------------------------------------------------- camera --
// Horizontal only. cameraY is pinned to 0 because the arena is one screen
// tall - letting it drift would slide the static background off its anchor.
// Level 3 has NO camera movement at all. Both axes are pinned to zero for
// the whole fight, so world coordinates and screen coordinates are the same
// thing here. The hero walks left edge to right edge and nothing scrolls.
inline void updateCameraL3() {
	cameraX = 0.0f;
	cameraY = 0.0f;
}

// =====================================================================
// PART 1 - ENEMY WAVE + BOSS COUNTDOWN
//
// The arena opens on a 2-minute wave of the regular enemy roster (the same
// Enemy struct/spawnEnemyAt()/updateEnemies()/drawEnemies() Levels 1/2 use -
// see Header/enemy.h, pulled in transitively through level.hpp before this
// file is ever included). Enemies are scattered across the whole arena
// floor; every time the hero kills one, a fresh one takes its place
// somewhere else, so the arena never empties out during the wave.
//
// A countdown clock drives it, same shape as Level2.hpp's flood clock
// (levelClockTicks / TICKS_PER_SECOND) - TICKS_PER_SECOND is that file's
// #define and is already in scope here because level.hpp #includes
// Level2.hpp at its own bottom, and level.hpp is required (by this file's
// own top-of-file comment) to be #included before Level3.hpp everywhere it
// matters (see iMain.cpp's include order).
//
// PART 2 HOOK: the instant the clock runs out, level3Phase flips to
// L3_PHASE_BOSS_PENDING and wave-spawning stops for good (see
// updateLevel3Wave() below) - that phase change is where Thanos's entrance
// (spawnThanos(), the power-stone animation, and the meteor shower that
// wipes whatever's left standing) gets wired in next.
// =====================================================================
// CHANGED: 120 -> 60. One minute of the regular roster before Thanos walks
// in, instead of two. The wave is an opening act, not the level - two full
// minutes of top-up spawning was long enough that players were out of
// pistol/AK ammo cycles before the boss had even appeared. Nothing else
// about the wave changed: same roster, same L3_MAX_WAVE_ENEMIES cap, same
// one-for-one replacement on every kill.
//
// The countdown is also no longer DISPLAYED (see drawLevel3HUD() below) -
// the clock still runs exactly as before, the player just isn't told about
// it, so Thanos's arrival lands as an event rather than an appointment.
#define L3_WAVE_SECONDS 60                                     // 1 minute of regular enemies before the boss
#define L3_WAVE_TOTAL_TICKS (L3_WAVE_SECONDS * TICKS_PER_SECOND)
#define L3_MAX_WAVE_ENEMIES 16                                 // how many stay alive on the floor at once during the wave - one full pass of the 8-strong roster, twice over

enum Level3Phase { L3_PHASE_WAVE = 0, L3_PHASE_BOSS_PENDING = 1, L3_PHASE_BOSS = 2 };

__declspec(selectany) Level3Phase level3Phase = L3_PHASE_WAVE;
__declspec(selectany) int level3ClockTicks = 0;          // counts up to L3_WAVE_TOTAL_TICKS
__declspec(selectany) int level3PrevActiveEnemies = 0;   // active-enemy count last frame, so a drop = a kill to replace

// --- PART 2: boss intro / meteor shower ------------------------------
// true once the entrance sequence (spawnThanos() + the forced power-stone
// summon) has actually fired - guards it to run exactly once per level
// entry, since L3_PHASE_BOSS_PENDING can last several frames while the
// summon clip plays out.
__declspec(selectany) bool level3BossIntroStarted = false;

// A handful of purely COSMETIC falling shapes, separate from
// Header/thanos.hpp's own single-slot thanosMeteor - these carry no hit
// test of their own. They rain down across the whole arena at the same
// moment the real meteor (thanosMeteor) is released, so the intro reads as
// a SHOWER hitting the whole floor rather than one rock landing in one
// spot. The actual enemy wipe happens instantly in code the same tick (see
// the L3_PHASE_BOSS_PENDING block in drawLevel3()) - these never need to
// "hit" anything, they're just decoration on top of that.
#define L3_SHOWER_METEOR_COUNT 6
#define L3_SHOWER_FALL_MS 450
#define L3_SHOWER_STAGGER_MS 220   // random extra delay so they don't all land in unison

struct Level3ShowerMeteor {
	bool active = false;
	int x = 0;
	long startMs = 0;
};
static Level3ShowerMeteor level3ShowerMeteors[L3_SHOWER_METEOR_COUNT];

// Fires every decorative meteor at once, each with its own random x and a
// small random start delay (see L3_SHOWER_STAGGER_MS).
inline void level3TriggerShower() {
	long now = thanosClockMs();
	for (int i = 0; i < L3_SHOWER_METEOR_COUNT; i++) {
		level3ShowerMeteors[i].active = true;
		level3ShowerMeteors[i].x = rand() % L3_ARENA_WIDTH;
		level3ShowerMeteors[i].startMs = now + (rand() % L3_SHOWER_STAGGER_MS);
	}
}

// Advance + draw every decorative meteor still falling. Safe to call every
// frame regardless of whether any are active (same "no-op when idle" shape
// as updateThanosMeteor()/drawThanosMeteor() in thanos.hpp).
inline void drawLevel3Shower() {
	long now = thanosClockMs();
	for (int i = 0; i < L3_SHOWER_METEOR_COUNT; i++) {
		if (!level3ShowerMeteors[i].active) continue;

		long elapsed = now - level3ShowerMeteors[i].startMs;
		if (elapsed < 0) continue;   // still waiting out its own stagger delay
		if (elapsed >= L3_SHOWER_FALL_MS) { level3ShowerMeteors[i].active = false; continue; }

		float t = (float)elapsed / (float)L3_SHOWER_FALL_MS;
		float y = (float)SCREEN_HEIGHT - t * ((float)SCREEN_HEIGHT - (float)L3_FLOOR_TOP_Y);

		// Same placeholder color drawThanosMeteor() falls back to when the
		// real meteor's art is missing - keeps the whole shower visually
		// consistent even without dedicated shower art.
		iSetColor(170, 80, 255);
		iFilledRectangle(level3ShowerMeteors[i].x - 8, (int)y, 16, 40);
	}
}

inline int level3WaveSecondsLeft() {
	int left = (L3_WAVE_TOTAL_TICKS - level3ClockTicks) / TICKS_PER_SECOND;
	return left < 0 ? 0 : left;
}

inline int level3CountActiveEnemies() {
	int n = 0;
	for (int i = 0; i < MAX_ENEMIES; i++) if (enemyList[i].active) n++;
	return n;
}

// Actual enemy spawning (spawnLevel3Enemy() / spawnLevel3EnemyOfType() /
// spawnLevel3EnemyWave() - the "guaranteed one of each type + fill the rest
// randomly, in the middle-to-right band" logic) now lives in Header/enemy.h,
// next to spawnEnemyAt() and the rest of the enemy roster code it depends
// on. This file only owns WHEN spawning happens (the wave clock/phase
// below), not HOW an individual enemy gets placed.

// Called once from initLevel3(): wipes anything left over from Level 1/2
// (clearEnemies() - Level 3 never did this before, so a fresh run used to
// carry Level 2's leftover enemy roster straight into the boss arena) and
// scatters the opening wave (L3_MAX_WAVE_ENEMIES, guaranteed one of every
// type - see spawnLevel3EnemyWave() in Header/enemy.h) across the floor.
inline void level3StartWave() {
	clearEnemies();
	level3Phase = L3_PHASE_WAVE;
	level3ClockTicks = 0;
	level3BossIntroStarted = false;
	for (int i = 0; i < L3_SHOWER_METEOR_COUNT; i++) level3ShowerMeteors[i].active = false;

	spawnLevel3EnemyWave(L3_ARENA_WIDTH, L3_FLOOR_TOP_Y, L3_MAX_WAVE_ENEMIES);

	level3PrevActiveEnemies = level3CountActiveEnemies();
}

// One call per FIXED TICK (30ms) while Level 3 is on screen - called from
// fixedUpdate() in iMain.cpp, same cadence as Level2.hpp's
// updateLevel2Clock(), and for the same reason: TICKS_PER_SECOND (33) is
// defined as "one tick per 30ms fixedUpdate call", so anything counted in
// those ticks has to actually BE ticked from fixedUpdate() to mean what its
// name says.
//
// BUG FIX: this used to be called from drawLevel3() instead - i.e. once per
// RENDERED FRAME, not once per fixed 30ms step. iDraw() runs at whatever
// rate the display/driver pushes frames (commonly 60fps+, uncapped on some
// systems), which is faster than the 33-ticks/sec the clock was designed
// around - so the "3 minutes" wave clock was actually finishing in well
// under 3 real minutes, and enemy replacement was checked/spawned far more
// often than intended too. Moving the call to fixedUpdate() (gated the same
// way Level 2's clock is - see iMain.cpp) makes one tick mean one 30ms step
// again, regardless of how fast the screen is actually redrawing. It also
// means the clock now correctly freezes on ESC/pause, which it didn't
// before either (drawLevel3() keeps rendering - "frozen" or not - while
// fixedUpdate() is what gamePaused actually gates).
inline void updateLevel3Wave() {
	if (level3Phase != L3_PHASE_WAVE) return;

	// DEV/PLAYER SHORTCUT: pressing P skips straight to the end of the wave
	// countdown - same effect as the clock running out on its own, just
	// forced early. Only clamping level3ClockTicks up to the cap (never
	// past it, never below where it already is) means holding the key down
	// is harmless: once the phase leaves L3_PHASE_WAVE the early-return
	// above stops this branch from running again anyway.
	if (isKeyPressed('p') || isKeyPressed('P')) {
		level3ClockTicks = L3_WAVE_TOTAL_TICKS;
	}

	if (level3ClockTicks < L3_WAVE_TOTAL_TICKS) level3ClockTicks++;

	int nowActive = level3CountActiveEnemies();

	// Somebody died since last frame (possibly more than one, e.g. a
	// grenade or the flamethrower catching two at once) - replace them
	// one-for-one, up to the cap, so a kill always opens a slot for a new
	// enemy elsewhere on the floor instead of just thinning the wave out.
	if (nowActive < level3PrevActiveEnemies) {
		int toSpawn = level3PrevActiveEnemies - nowActive;
		for (int i = 0; i < toSpawn && nowActive < L3_MAX_WAVE_ENEMIES; i++) {
			spawnLevel3Enemy(L3_ARENA_WIDTH, L3_FLOOR_TOP_Y);
			nowActive++;
		}
	}
	level3PrevActiveEnemies = nowActive;

	// Timer's up - stop spawning new enemies for good (the early-return at
	// the top of this function means nothing below this point ever runs
	// again once level3Phase leaves L3_PHASE_WAVE) and bring Thanos in.
	if (level3ClockTicks >= L3_WAVE_TOTAL_TICKS && !level3BossIntroStarted) {
		level3Phase = L3_PHASE_BOSS_PENDING;
		level3BossIntroStarted = true;

		spawnThanos(L3_ARENA_WIDTH - THANOS_WIDTH - TILE_SIZE * 2, L3_FLOOR_TOP_Y);

		// Force straight into the power-stone summon instead of his normal
		// ALIVE/chase state - same fields updateThanos() itself sets when a
		// power stone triggers normally (see the POWER_STONE branch in
		// Header/thanos.hpp's updateThanos()), just set directly here since
		// this has to start on the very first frame he exists, before
		// updateThanos() has ever run for him.
		thanos.state = THANOS_POWER_STONE;
		thanos.actionSpeedMult = 1.0f;
		thanos.powerStoneFrame = 0;
		thanos.powerStoneStartMs = thanosClockMs();
		thanos.powerStoneTargetX = thanos.x + THANOS_WIDTH / 2;   // doesn't matter - the shower ignores this, it kills every active enemy regardless of position
	}
}

// Level 3's slice of the HUD.
//
// REMOVED: the "THANOS ARRIVES IN  m:ss" headline and the draining
// drawEnvBar() underneath it that used to live here. The wave clock itself
// is untouched and still runs on exactly the same fixed tick
// (updateLevel3Wave() above) - it is only the READOUT that is gone, by
// design: the arena is supposed to feel like a fight that gets interrupted,
// not like a timer the player is waiting out. GameFlow.hpp's drawCenterHUD()
// already draws no environmental bar for Level 3 ("clean combat HUD"), so
// with the countdown gone Level 3's HUD is now genuinely just score +
// health + weapon + (once he is up) Thanos's own HP bar.
//
// What took its place is the adrenaline readout below - the one piece of
// Level 3 state the player DOES need told to them, because it changes how
// the hero handles mid-fight.
inline void drawLevel3HUD() {
	// --- ADRENALINE SURGE -------------------------------------------------
	// Mirrors the condition Playercontroller.hpp uses to actually apply the
	// boost (hero.speedScale), rather than re-deriving it from HP here, so
	// the label can never claim the surge is up on a tick where it isn't.
	// Sits in the space the old countdown used to occupy, just under the
	// score plate (whose bottom edge is SCREEN_HEIGHT - 84).
	if (hero.speedScale > 1.0f && !gameOverActive) {
		// Gentle pulse so it reads as a live effect rather than a static
		// label someone forgot to clear.
		float p = 0.5f + 0.5f * (float)fabs(sin(pulseTimer * 3.0f));

		iSetColor(255, 90 + (int)(70 * p), 60);
		drawBigTextCentered(SCREEN_HEIGHT - 104.0f, "ADRENALINE SURGE   +30% MOBILITY", 0.105f, 1.6f);
	}
}

// ------------------------------------------------------------------ setup --
inline void initLevel3() {
	initLevel3Assets();

	currentLevel = 3;
	hero.posX = TILE_SIZE * 2;
	hero.posY = L3_FLOOR_TOP_Y;
	hero.velocityY = 0;
	hero.isGrounded = true;
	hero.facing = 1;

	cameraX = 0.0f;
	cameraY = 0.0f;

	// Thanos does NOT stand up yet - the arena opens on the 2-minute enemy
	// wave (level3StartWave() below) instead. He used to be spawned right
	// here unconditionally (spawnThanos()); that call - and the power stone
	// entrance/meteor shower that goes with it - now happens once
	// updateLevel3Wave() flips level3Phase to L3_PHASE_BOSS_PENDING (PART 2).
	// Forcing active = false here also guarantees a leftover Thanos from a
	// previous attempt at the fight (e.g. the hero died mid-fight and
	// re-entered) can never still be standing/visible during the wave.
	thanos.active = false;

	level3StartWave();
}

// --------------------------------------------------------------- rendering --
// STATIC background: drawn at a fixed screen position with no cameraX/Y
// term at all. That single omission is what makes it a painted backdrop
// instead of a scrolling layer.
inline void drawLevel3Background() {
	if (imgL3Background != (unsigned int)-1) {
		iShowImage(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, imgL3Background);
	}
	else {
		iSetColor(48, 30, 18);
		iFilledRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	}
}

// Floor: Ground_1..5 repeating, laid edge-to-edge. Only the columns
// actually on screen are drawn (same culling Level 2 uses), and the
// variant index is derived from the WORLD column - not the screen column -
// so the pattern stays locked to the world and doesn't crawl as the
// camera moves.
inline void drawLevel3Floor() {
	int startCol = (int)(cameraX / TILE_SIZE) - 1;
	if (startCol < 0) startCol = 0;
	int endCol = startCol + (SCREEN_WIDTH / TILE_SIZE) + 3;
	if (endCol > L3_ARENA_COLS) endCol = L3_ARENA_COLS;

	for (int row = 0; row < L3_FLOOR_ROWS; row++) {
		for (int c = startCol; c < endCol; c++) {
			float screenX = (float)(c * TILE_SIZE) - cameraX;
			float screenY = (float)(row * TILE_SIZE);

			unsigned int img = imgL3Ground[c % L3_GROUND_VARIANTS];

			if (img != (unsigned int)-1) {
				// +1px on width/height closes the hairline seam that
				// appears between adjacent tiles when the camera sits on a
				// fractional pixel - this is the "no visible gaps" fix.
				iShowImage((int)screenX, (int)screenY, TILE_SIZE + 1, TILE_SIZE + 1, img);
			}
			else {
				iSetColor(120, 72, 34);
				iFilledRectangle(screenX, screenY, TILE_SIZE, TILE_SIZE);
			}
		}
	}
}

inline void drawLevel3() {
	// Measure real frame time once, up front - updateBullets(), updateThanos()
	// and updateThanosBeams() below all scale their per-tick steps by it so
	// Level 3 plays at the same speed regardless of render frame rate.
	l3BeginFrame();

	drawLevel3Background();
	updateCameraL3();
	drawLevel3Floor();

	// NOTE: updateLevel3Wave() (the wave clock + enemy top-up - see the
	// "PART 1 - ENEMY WAVE + BOSS COUNTDOWN" block above) is deliberately
	// NOT called here anymore - it's ticked from fixedUpdate() in
	// iMain.cpp now, once per fixed 30ms step, so the clock runs at real-
	// time speed regardless of the render frame rate (see the bug-fix
	// comment on updateLevel3Wave() itself).

	// Same regular-enemy update/draw pair Level 1/2 make every frame (see
	// level.hpp's drawGameLevel() and Level2.hpp's drawLevel2()) - Level 3
	// never called these before, so the wave enemies spawned above would
	// otherwise just sit there frozen and invisible, exactly like the
	// bullet bug noted below.
	updateEnemies(hero);
	drawEnemies();

	// FIX: the pistol/AK47 both spawn their bullets from Playercontroller.hpp
	// regardless of page (it gates on PAGE_PLAYING || PAGE_LEVEL3, same as
	// movement), but nothing on the PAGE_LEVEL3 draw path ever advanced or
	// drew them - drawLevel3() never called updateBullets()/drawBullets(),
	// so every round fired in the Titan arena spawned into bulletList and
	// just sat there, frozen and invisible, instead of flying across the
	// screen. Same two calls level.hpp's drawGameLevel() makes for Levels
	// 1/2 - this is what actually moves bulletList[i].x every tick and
	// draws the sprite at its current position.
	updateBullets();
	drawBullets(cameraX, cameraY);

	// The boss - chase/contact-damage/animation tick, then draw (see
	// Header/thanos.hpp). Drawn before the hero so the hero renders on top
	// when they overlap.
	updateThanos(hero, L3_ARENA_WIDTH);

	// PART 2: catch the INTRO power-stone summon finishing. updateThanos()
	// (Header/thanos.hpp, unchanged) just called spawnThanosMeteor() and
	// flipped thanos.state back to THANOS_ALIVE the instant the summon clip
	// ended - thanosMeteor.phase only ever becomes THANOS_METEOR_FALLING
	// inside spawnThanosMeteor(), so this combination can only mean "a power
	// stone summon just released its meteor." Scoping it to
	// L3_PHASE_BOSS_PENDING (and advancing the phase in the very last line)
	// makes sure this fires for THIS one intro summon only - every later
	// power stone attack during the real fight happens with level3Phase
	// already at L3_PHASE_BOSS, so this block simply won't match again.
	if (level3Phase == L3_PHASE_BOSS_PENDING &&
		thanos.state == THANOS_ALIVE &&
		thanosMeteor.phase == THANOS_METEOR_FALLING) {

		// Wipe every regular enemy still standing. No score/heal here
		// (killEnemy()/healHeroOnKill() are for hero kills) - these enemies
		// died to the meteor shower, not to the hero.
		for (int i = 0; i < MAX_ENEMIES; i++) enemyList[i].active = false;

		level3TriggerShower();
		level3Phase = L3_PHASE_BOSS;
	}

	drawThanos(cameraX, cameraY);
	drawThanosHPBar();

	// Second-life beam bolts - a separate tick/draw pair, not folded into
	// updateThanos()/drawThanos() above, because a bolt already in flight
	// has to keep flying (and stay hittable) independent of whatever state
	// Thanos himself is in a tick later (see the comment on
	// updateThanosBeams() in Header/thanos.hpp).
	updateThanosBeams(hero, L3_ARENA_WIDTH);
	drawThanosBeams(cameraX, cameraY);

	// Power stone meteor - same reasoning as the beam bolts above: it has to keep
	// falling (and stay able to hit) independent of whatever Thanos is doing a
	// tick later, so it is its own tick/draw pair (see updateThanosMeteor() in
	// Header/thanos.hpp).
	updateThanosMeteor(hero);
	drawThanosMeteor(cameraX, cameraY);

	// The rest of the shower - purely decorative, see level3TriggerShower()/
	// drawLevel3Shower() above. Drawn on top of the real meteor so the
	// intro reads as multiple rocks hitting the floor at once.
	drawLevel3Shower();

	float heroScreenX = hero.posX - cameraX;
	float heroScreenY = hero.posY - cameraY;
	int heroImg = hero.currentImage();
	if (heroImg != -1) {
		iShowImage((int)heroScreenX, (int)heroScreenY, hero.WIDTH, hero.HEIGHT, heroImg);
	}
	else {
		iSetColor(0, 255, 255);
		iFilledRectangle(heroScreenX, heroScreenY, hero.WIDTH, hero.HEIGHT);
	}

	drawHealthBar(hero);
	// FIX: the weapon HUD (icon + round counter, top-right) never showed up
	// in the boss arena either - drawLevel3() never called drawWeaponHUD().
	drawWeaponHUD(hero);

	// Wave countdown / boss-pending readout - see drawLevel3HUD() above.
	drawLevel3HUD();
}

// The one entry point into the Titan arena. Everything that has to change
// on the way in happens here so no caller can half-transition: page, level
// id, arena physics/spawn, music, and the title card.
inline void enterLevel3() {
	initLevel3();                 // sets currentLevel = 3, spawns, resets camera
	currentPage = PAGE_LEVEL3;

	stopBGM();
	playLevel3BGM();

	showLevelTitle(3);
}

#endif