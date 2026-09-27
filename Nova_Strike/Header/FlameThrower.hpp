#ifndef FLAMETHROWER_HPP
#define FLAMETHROWER_HPP

#include <ctime>
#include "Utility.hpp"

// Owner: [Gameplay / Physics teammate]
//
// Unlike every other weapon, the flamethrower isn't a single fire-and-forget
// animation cycle (knife slash / pistol shot) - it's a SUSTAINED stream that
// keeps going for as long as space is held, so it can't be driven by Hero's
// generic startAttack()/updateAttack() (that state machine assumes an attack
// always finishes on its own after a fixed number of ticks). Instead this
// struct owns its own little state machine, ticked directly from
// Playercontroller.hpp while WEAPON_FLAMETHROWER is equipped - see update()
// below.
//
// Animation shape (0-indexed frame numbers into atkImgR/L[], which hold
// R_1..R_6 / L_1..L_6):
//   FLAME_STARTUP - plays R_1 -> R_2 -> R_3 (index 0,1,2) once, the "wind-up"
//   FLAME_LOOP    - flickers back and forth between R_4 and R_5 (index 3,4)
//                   for as long as space stays held (and the time limit
//                   hasn't been hit) - this is the sustained stream
//   FLAME_ENDING  - shows R_6 (index 5) briefly, then drops back to idle -
//                   plays whether the stream stopped because space was
//                   released or because the 10s limit was hit
//
// Damage: enemies standing in the flame's hitbox (see Hero::flameHitbox() in
// Hero.hpp, using FLAME_RANGE_X/Y below) take FLAMETHROWER_DMG - but only
// once every FLAMETHROWER_DAMAGE_INTERVAL_TICKS while actually in the LOOP
// phase, not every single tick (100 dmg every ~16ms would be absurd DPS -
// this cadence lands it in the same rough DPS ballpark as the pistol/AK47).

const int FLAMETHROWER_DMG = 100;      // damage dealt to anything caught in the flame area
const int FLAMETHROWER_ATK_FRAMES = 6; // R_1..R_6 / L_1..L_6 under attack\FlameThrower

// The flamethrower never actually runs through Hero::startAttack()/
// updateAttack() - it's driven entirely by FlameThrower::update() instead
// (see the sustained-stream state machine below), so these two never
// actually get read at runtime. They only exist so WeaponControl::
// attackTicks()/attackHitTick() (WeaponControl.hpp) - which fall through to
// these as their default/last case for every weapon type, flamethrower
// included - have something to return and compile cleanly.
const int FLAMETHROWER_ATTACK_TICKS = 1;
const int FLAMETHROWER_ATTACK_HIT_TICK = 1;

// how far in front of the hero the flame actually reaches/damages - kept
// separate from the knife's ATTACK_RANGE_X/Y (Hero.hpp) since a flame cone
// reasonably reaches further than a melee swing.
const int FLAME_RANGE_X = 110;
const int FLAME_RANGE_Y = 45;

// --- animation pacing (in ticks - one tick = one fixedUpdate() call) ---
const int FLAMETHROWER_STARTUP_FRAME_TICKS = 4; // ticks spent on each of R_1/R_2/R_3
const int FLAMETHROWER_LOOP_FRAME_TICKS = 5;     // ticks spent on R_4 before flicking to R_5 (and back)
const int FLAMETHROWER_END_FRAME_TICKS = 8;      // ticks R_6 stays on screen before fully stopping

// how often (in ticks) a LOOP-phase tick actually deals damage - decoupled
// from the visual flicker rate above so animation speed and DPS can be
// tuned independently.
const int FLAMETHROWER_DAMAGE_INTERVAL_TICKS = 15; // ~240ms between hits at a ~16ms tick rate

// --- sustained-fire time limit / cooldown (real time, like Pistol's) ---
const long FLAMETHROWER_MAX_FIRE_MS = 10000; // 10s of continuous fire before it's forced to stop
const long FLAMETHROWER_COOLDOWN_MS = 4000;  // 4s cooldown once that limit is hit

enum FlamePhase {
	FLAME_IDLE,     // not firing, ready (assuming not on cooldown)
	FLAME_STARTUP,  // R_1->R_2->R_3, once
	FLAME_LOOP,     // R_4<->R_5, sustained, dealing damage
	FLAME_ENDING    // R_6, then back to FLAME_IDLE
};

struct FlameThrower {
	int runImgR[4];
	int runImgL[4];

	int jumpImgR[4];
	int jumpImgL[4];

	int atkImgR[FLAMETHROWER_ATK_FRAMES];
	int atkImgL[FLAMETHROWER_ATK_FRAMES];

	FlamePhase phase = FLAME_IDLE;
	int frameIndex = 0;  // which atkImg[] index (0-5) is showing right now
	int frameTicker = 0; // ticks spent on the current frame/phase step
	int dmgTicker = 0;   // ticks since the last damage application (LOOP phase only)

	// real time (ms, clock()-based - same style as Pistol::cooldownStartMs)
	// this CONTINUOUS hold started. Reset to 0 whenever the stream fully
	// stops (whether by release or by hitting the limit) - so per the spec,
	// releasing early gives the next press a full fresh 10 seconds rather
	// than picking up mid-window.
	long fireStartMs = 0;

	bool inCooldown = false;
	long cooldownStartMs = 0;

	void loadAssets() {
		loadImage(runImgR, 4, "images\\hero\\running\\FlameThrower", "R");
		loadImage(runImgL, 4, "images\\hero\\running\\FlameThrower", "L");

		loadImage(jumpImgR, 4, "images\\hero\\jumping\\FlameThrower", "R");
		loadImage(jumpImgL, 4, "images\\hero\\jumping\\FlameThrower", "L");

		loadImage(atkImgR, FLAMETHROWER_ATK_FRAMES, "images\\hero\\attack\\FlameThrower", "R");
		loadImage(atkImgL, FLAMETHROWER_ATK_FRAMES, "images\\hero\\attack\\FlameThrower", "L");
	}

	// resets every piece of runtime state back to a fresh, ready weapon -
	// used when the game restarts the hero from the beginning (see
	// resetGameState() in level.hpp) so a death mid-stream/mid-cooldown
	// doesn't carry over into the next life.
	void reset() {
		phase = FLAME_IDLE;
		frameIndex = 0;
		frameTicker = 0;
		dmgTicker = 0;
		fireStartMs = 0;
		inCooldown = false;
		cooldownStartMs = 0;
	}

	bool canFire() { return !inCooldown; }

	// true any tick the stream is doing SOMETHING (startup, looping, or
	// playing its ending puff) - Playercontroller.hpp mirrors this into
	// hero.isAttacking so currentImage()/weapon-swap-lock behave the same
	// way they already do for every other weapon's attack.
	bool isActive() { return phase != FLAME_IDLE; }

	// call every tick while WEAPON_FLAMETHROWER is equipped - spaceHeld is
	// whether the space bar is physically down THIS tick. Returns true on
	// the ticks the flame should actually apply FLAMETHROWER_DMG.
	bool update(bool spaceHeld) {
		long now = (long)(clock() * 1000L / CLOCKS_PER_SEC);
		bool dealsDamageNow = false;

		if (phase == FLAME_IDLE) {
			if (spaceHeld && canFire()) {
				phase = FLAME_STARTUP;
				frameIndex = 0;
				frameTicker = 0;
				dmgTicker = 0;
				fireStartMs = now;
			}
			return false;
		}

		if (phase == FLAME_STARTUP) {
			frameTicker++;
			if (frameTicker >= FLAMETHROWER_STARTUP_FRAME_TICKS) {
				frameTicker = 0;
				frameIndex++;
				if (frameIndex >= 3) { // just reached R_4 - hand off to the sustained loop
					phase = FLAME_LOOP;
					frameIndex = 3;
					dmgTicker = 0;
				}
			}
		}
		else if (phase == FLAME_LOOP) {
			frameTicker++;
			if (frameTicker >= FLAMETHROWER_LOOP_FRAME_TICKS) {
				frameTicker = 0;
				frameIndex = (frameIndex == 3) ? 4 : 3; // flicker R_4 <-> R_5
			}

			dmgTicker++;
			if (dmgTicker >= FLAMETHROWER_DAMAGE_INTERVAL_TICKS) {
				dmgTicker = 0;
				dealsDamageNow = true;
			}
		}
		else if (phase == FLAME_ENDING) {
			frameTicker++;
			if (frameTicker >= FLAMETHROWER_END_FRAME_TICKS) {
				phase = FLAME_IDLE;
				frameIndex = 0;
				frameTicker = 0;
				fireStartMs = 0; // fully reset - the next press gets a fresh 10s window
			}
		}

		// still actively streaming (startup or loop) - check whether this
		// is the tick it has to stop, either because the player let go of
		// space or because the 10s cap was just hit.
		if (phase == FLAME_STARTUP || phase == FLAME_LOOP) {
			bool exceededLimit = (now - fireStartMs) >= FLAMETHROWER_MAX_FIRE_MS;

			if (exceededLimit) {
				// hit the time limit - forced stop AND a 4s cooldown before
				// it can be used again at all.
				phase = FLAME_ENDING;
				frameIndex = 5; // R_6 / L_6
				frameTicker = 0;
				inCooldown = true;
				cooldownStartMs = now;
			}
			else if (!spaceHeld) {
				// released early, under the limit - still ends on R_6, but
				// no cooldown; fireStartMs gets reset once FLAME_ENDING
				// finishes above, so the next press starts a clean 10s.
				phase = FLAME_ENDING;
				frameIndex = 5;
				frameTicker = 0;
			}
		}

		return dealsDamageNow;
	}

	// call every tick regardless of what's equipped, same as Pistol/AK47's
	// updateCooldown() - lets a cooldown started earlier keep expiring even
	// if the player swaps away from the flamethrower mid-cooldown.
	void updateCooldown() {
		if (!inCooldown) return;

		long now = (long)(clock() * 1000L / CLOCKS_PER_SEC);
		if (now - cooldownStartMs >= FLAMETHROWER_COOLDOWN_MS)
			inCooldown = false;
	}
};

#endif
