#ifndef GRENADE_HPP
#define GRENADE_HPP

#include "iGraphics.h"
#include "Utility.hpp"

// Owner: [Gameplay / Physics teammate]
//
// Unlike every other weapon, the grenade isn't tied to the equipped-weapon
// slot / space-bar attack at all - it's thrown with its own dedicated key
// (TAB, see Playercontroller.hpp) no matter what's currently equipped, and
// plays out in three distinct stages instead of one attack animation:
//
//   1) THROW (windup)  - the hero plays Throwing_R_1..4 / Throwing_L_1
//                         (Grenade::isThrowing/throwFrame below - see
//                         Hero::currentImage() in Hero.hpp, which checks
//                         this BEFORE the normal isAttacking pose so it
//                         overrides whatever weapon is equipped).
//   2) FLYING           - once the windup finishes, a real physics object
//                         (FlyingGrenade below) launches from the hero's
//                         position: constant horizontal velocity + gravity
//                         pulling it down, same "vy -= GRAVITY every tick"
//                         shape as the hero's own jump (see Hero.hpp) - it
//                         keeps falling/drifting until it lands on a solid
//                         tile, however far that takes (so a grenade tossed
//                         off a ledge keeps arcing down past where it would
//                         have landed on flat ground). Drawn as a single
//                         GrenadeIcon.png the whole time it's airborne.
//   3) BLAST            - the instant it lands, it deals GRENADE_DMG to
//                         every enemy within GRENADE_BLAST_RADIUS px (see
//                         damageEnemiesInRadius() in enemy.h) and plays the
//                         Brusting_R/L_1..7 explosion in place.
//
// The physics step (stage 2->3, since it needs levelGrid collision) lives in
// level.hpp's updateFlyingGrenades() - same reason updateBullets() lives
// there instead of in Bullet.hpp. This file only owns the art, the windup
// timer, the ammo count, and the flying-grenade pool's data/spawn/draw.

const int GRENADE_DMG = 250;            // damage dealt to anything caught in the blast radius
const int GRENADE_BLAST_RADIUS = 50;    // px - see damageEnemiesInRadius() in enemy.h
const int GRENADE_START_COUNT = 2;      // how many grenades the hero starts a life with

const int GRENADE_ATK_FRAMES = 4;       // Throwing_R_1..4 - only Throwing_L_1 exists on disk
                                         // (see loadAssets() below, same repeat-last-frame
                                         // trick AK47.hpp uses for its own asymmetric art)
const int GRENADE_BURST_FRAMES = 7;     // Brusting_R_1..7 / Brusting_L_1..7

const int GRENADE_ATTACK_TICKS = 36;      // unused (WeaponControl's generic startAttack()/
const int GRENADE_ATTACK_HIT_TICK = 18;   // updateAttack() path never runs for the grenade -
                                           // it's driven entirely by TAB/updateThrow() below
                                           // instead) - kept only so WeaponControl's per-weapon
                                           // attackTicks()/attackHitTick() dispatch (which falls
                                           // through to these as grenade's case) compiles.

// how long each windup/burst frame stays on screen, in ticks
const int GRENADE_THROW_FRAME_TICKS = 5; // 4 frames * 5 ticks = 20-tick windup, same pace as the pistol's firing animation
const int GRENADE_BURST_FRAME_TICKS = 4; // 7 frames * 4 ticks = 28-tick explosion

// On-screen size of the flying grenade icon (also its collision box - see
// updateFlyingGrenades() in level.hpp) and of the burst art drawn over it
// once it lands. GrenadeIcon.png is a tall/narrow sprite (1055x1490 on
// disk); BURST_DRAW_SIZE is deliberately 2*GRENADE_BLAST_RADIUS so the
// explosion's drawn size visually matches how far its damage actually
// reaches.
const int GRENADE_ICON_WIDTH = 24;
const int GRENADE_ICON_HEIGHT = 34;
const int GRENADE_BURST_DRAW_SIZE = GRENADE_BLAST_RADIUS * 2;

// --- flight physics (px/tick) ---
// Same discrete "velocity, then gravity chips away at it every tick" shape
// as GRAVITY/JUMP_SPEED in Hero.hpp - duplicated here (not shared) because
// Grenade.hpp is included from WeaponControl.hpp BEFORE Hero.hpp's own
// struct exists (Hero.hpp -> WeaponControl.hpp -> Grenade.hpp), so this
// file can't #include Hero.hpp itself without going circular.
//
// GRENADE_SPEED_X/GRENADE_LAUNCH_VY are tuned together so a grenade thrown
// over flat ground (launches and lands at the same height - see
// Playercontroller.hpp's launchY) travels ~50px horizontally before it
// lands, per spec. Thrown off a ledge (no ground under it at that point) it
// simply keeps falling/drifting past that until it finds one, however far
// down that is.
const int   GRENADE_GRAVITY = 1;
const float GRENADE_LAUNCH_VY = 6.0f;
const float GRENADE_SPEED_X = 4.5f;

// forward declaration - defined further down, after FlyingGrenade exists.
// Grenade::updateThrow() below needs to call it; keeping the actual pool/
// definition below the struct it's a pool OF just reads top-to-bottom.
inline void spawnFlyingGrenade(float launchX, float launchY, int facing);

struct Grenade {
	// no runImg/jumpImg arrays - same reasoning as before, there's no run/
	// jump art for this weapon. WeaponControl falls back to the pistol's
	// stance while grenade is the EQUIPPED slot and the hero is moving/
	// jumping (see WeaponControl::runImg()/jumpImg()) - unrelated to the
	// TAB throw itself, which works no matter what's equipped.

	int atkImgR[GRENADE_ATK_FRAMES]; // Throwing_R_1..4 - doubles as both the
	int atkImgL[GRENADE_ATK_FRAMES]; // equipped-slot idle pose AND the TAB windup animation

	int burstImgR[GRENADE_BURST_FRAMES]; // Brusting_R_1..7
	int burstImgL[GRENADE_BURST_FRAMES]; // Brusting_L_1..7

	unsigned int iconImg = (unsigned int)-1; // GrenadeIcon.png - the flying sprite AND the
	                                          // "grenades remaining" HUD icon (see Health.hpp)

	// --- ammo ---
	int grenadesRemaining = GRENADE_START_COUNT;

	// --- TAB windup state ---
	bool isThrowing = false;
	int  throwFrame = 0;
	int  throwTicker = 0;
	int  throwFacing = 1; // captured at the moment the throw starts, so turning
	                       // around mid-windup doesn't change which way it launches

	void loadAssets() {
		loadImage(atkImgR, GRENADE_ATK_FRAMES, "images\\hero\\attack\\grenade", "Throwing_R");

		// only Throwing_L_1 exists on disk - load that one real frame, then
		// repeat it into every other slot so atkImgL[] is valid at every
		// index atkImgR[] is, same trick AK47::loadAssets() uses for its
		// own missing L_6.
		loadImage(atkImgL, 1, "images\\hero\\attack\\grenade", "Throwing_L");
		for (int i = 1; i < GRENADE_ATK_FRAMES; i++)
			atkImgL[i] = atkImgL[0];

		loadImage(burstImgR, GRENADE_BURST_FRAMES, "images\\hero\\attack\\grenade", "Brusting_R");
		loadImage(burstImgL, GRENADE_BURST_FRAMES, "images\\hero\\attack\\grenade", "Brusting_L");

		iconImg = iLoadImage((char*)"images\\hero\\attack\\grenade\\GrenadeIcon.png");
	}

	// resets ammo + windup state back to a fresh life - see resetGameState()
	// in level.hpp. Doesn't touch the flying-grenade pool itself; that's a
	// separate reset (resetFlyingGrenades() below) since it isn't part of
	// any one Grenade instance.
	void reset() {
		grenadesRemaining = GRENADE_START_COUNT;
		isThrowing = false;
		throwFrame = 0;
		throwTicker = 0;
		throwFacing = 1;
	}

	// which windup frame to show right now - Hero::currentImage() calls
	// this directly (ahead of the normal weapon.atkImg() check) whenever
	// isThrowing is true.
	int throwImg(int facing, int index) {
		return (facing >= 0) ? atkImgR[index] : atkImgL[index];
	}

	// call from Playercontroller.hpp the instant TAB is pressed (edge-
	// detected there, not here, so holding the key doesn't spam this every
	// tick). Does nothing (returns false) if a throw is already in progress
	// or the hero is out of grenades - otherwise commits one grenade from
	// the count right away and starts the windup.
	bool startThrow(int facing) {
		if (isThrowing) return false;
		if (grenadesRemaining <= 0) return false;

		isThrowing = true;
		throwFrame = 0;
		throwTicker = 0;
		throwFacing = (facing >= 0) ? 1 : -1;
		grenadesRemaining--;
		return true;
	}

	// call every tick regardless of whether a throw is actually in progress
	// (a no-op otherwise). Advances the windup animation and, the instant
	// it finishes, spawns the real physics grenade (spawnFlyingGrenade()
	// below) from (launchX, launchY) - see Playercontroller.hpp for what
	// those actually are. Takes plain ints rather than a Hero& on purpose:
	// this header is included from inside Hero.hpp's own include chain
	// (via WeaponControl.hpp), before struct Hero exists yet, so it can't
	// take a Hero& parameter without going circular.
	void updateThrow(int launchX, int launchY) {
		if (!isThrowing) return;

		throwTicker++;
		if (throwTicker >= GRENADE_THROW_FRAME_TICKS) {
			throwTicker = 0;
			throwFrame++;

			if (throwFrame >= GRENADE_ATK_FRAMES) {
				isThrowing = false;
				throwFrame = 0;
				spawnFlyingGrenade((float)launchX, (float)launchY, throwFacing);
			}
		}
	}
};

// --- the actual thrown, physics-driven grenade(s) ---
// Same fixed-size-pool-plus-active-flag shape as Bullet.hpp's bulletList -
// no dynamic allocation, plenty of headroom (4) over the 2-grenade starting
// supply for when a level later hands out more.
const int MAX_FLYING_GRENADES = 4;

struct FlyingGrenade {
	float x, y;
	float vx, vy;
	int   facing;     // which side's Brusting_* art to play once it lands
	bool  active;
	bool  bursting;   // false = still flying (physics runs, draws the plain
	                  // GrenadeIcon.png); true = landed, playing Brusting_*
	                  // in place (see updateFlyingGrenades() in level.hpp)
	int   burstFrame;
	int   burstTicker;
};

static FlyingGrenade flyingGrenades[MAX_FLYING_GRENADES];
// zero-initialized like bulletList/enemyList - every slot starts inactive.

// Launches a new grenade from (launchX, launchY) in the given direction -
// called by Grenade::updateThrow() the instant the windup animation
// finishes. Silently drops the throw if every slot is already in flight
// (4 is already more than the 2-grenade supply could ever fill at once).
inline void spawnFlyingGrenade(float launchX, float launchY, int facing) {
	for (int i = 0; i < MAX_FLYING_GRENADES; i++) {
		if (!flyingGrenades[i].active) {
			flyingGrenades[i].x = launchX;
			flyingGrenades[i].y = launchY;
			flyingGrenades[i].vx = (facing >= 0) ? GRENADE_SPEED_X : -GRENADE_SPEED_X;
			flyingGrenades[i].vy = GRENADE_LAUNCH_VY;
			flyingGrenades[i].facing = facing;
			flyingGrenades[i].active = true;
			flyingGrenades[i].bursting = false;
			flyingGrenades[i].burstFrame = 0;
			flyingGrenades[i].burstTicker = 0;
			return;
		}
	}
}

// drops every grenade currently in flight/bursting - called from
// resetGameState() in level.hpp, same spirit as clearing bulletList there.
inline void resetFlyingGrenades() {
	for (int i = 0; i < MAX_FLYING_GRENADES; i++)
		flyingGrenades[i].active = false;
}

// Draws every grenade currently in flight or bursting, camera-relative like
// drawBullets()/drawEnemies(). Takes the owning Grenade& purely for its
// already-loaded icon/burst textures (there's only ever one hero/one
// Grenade instance in this game - hero.weapon.grenade - same as
// drawWeaponHUD(Hero&) takes the whole Hero for its textures).
inline void drawFlyingGrenades(float cameraX, float cameraY, Grenade& g) {
	for (int i = 0; i < MAX_FLYING_GRENADES; i++) {
		FlyingGrenade& fg = flyingGrenades[i];
		if (!fg.active) continue;

		int screenX = (int)(fg.x - cameraX);
		int screenY = (int)(fg.y - cameraY);

		if (!fg.bursting) {
			if (g.iconImg != (unsigned int)-1)
				iShowImage(screenX, screenY, GRENADE_ICON_WIDTH, GRENADE_ICON_HEIGHT, g.iconImg);
		}
		else {
			int img = (fg.facing >= 0) ? g.burstImgR[fg.burstFrame] : g.burstImgL[fg.burstFrame];
			// center the (bigger) burst art over the grenade's own small
			// collision box so the explosion visually surrounds where it landed
			int drawX = screenX - (GRENADE_BURST_DRAW_SIZE - GRENADE_ICON_WIDTH) / 2;
			int drawY = screenY - (GRENADE_BURST_DRAW_SIZE - GRENADE_ICON_HEIGHT) / 2;
			if (img != -1)
				iShowImage(drawX, drawY, GRENADE_BURST_DRAW_SIZE, GRENADE_BURST_DRAW_SIZE, img);
		}
	}
}

#endif
