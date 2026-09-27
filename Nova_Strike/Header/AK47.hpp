// ===== Header/AK47.hpp =====
#ifndef AK47_HPP
#define AK47_HPP

#include <ctime>
#include "Utility.hpp"

// Owner: [Gameplay / Physics teammate]
//
// Same shape as Pistol.hpp - running/jumping/attack frames, its own bullet
// sprite + damage, and a fire-cap/cooldown - just automatic instead of
// semi-auto: a much shorter attack cycle (AK47_ATTACK_TICKS) so holding the
// trigger down just keeps re-triggering startAttack() the instant each tiny
// cycle ends, and a bigger 36-round cap before the cooldown kicks in (see
// AK47_BASE_MAX_ROUNDS/AK47_COOLDOWN_MS) instead of the pistol's 6 - bumped
// up to 100 once Level 2 starts (see AK47_LEVEL2_MAX_ROUNDS).
//
// The bullet itself flies exactly like the pistol's - same Bullet.hpp pool,
// same BULLET_SPEED (see Pistol.hpp) - just launched from here with this
// gun's own sprite/damage (see loadAssets()/Playercontroller.hpp).
//
// Attack art is asymmetric on disk right now - 6 right-facing frames
// (R_1..R_6) but only 5 left-facing ones (L_1..L_5, no L_6 yet). Rather than
// special-case that everywhere atkImg() gets called, loadAssets() below just
// repeats the last real left frame into the missing slot once, at load
// time - so atkImgL[] ends up fully valid at every index atkImgR[] is, same
// as every other weapon, and nothing downstream has to know the art's short
// a frame on one side.

const int AK47_DMG = 60;                 // damage dealt per round - doubled from the
// original 60 per the Level 1 rebalance (still
// fires much faster than the pistol - see
// AK47_ATTACK_TICKS below - so this hits a lot
// harder per round now, not just a DPS tweak)
const int AK47_ATK_FRAMES = 6;            // R_1..R_6 under attack\AK47 - paces atkIndx
// for BOTH directions, so turning around mid-burst
// doesn't change the fire cadence
const int AK47_ATK_FRAMES_L_ON_DISK = 5;  // only L_1..L_5 actually exist right now

// One round's animation cycle, in ticks - MUCH shorter than the pistol's
// 20-tick cycle (see PISTOL_ATTACK_TICKS in Pistol.hpp), which is what
// makes holding the trigger read as automatic fire instead of one shot at
// a time: startAttack() re-fires the instant the previous tiny cycle ends.
const int AK47_ATTACK_TICKS = 8;

// Vestigial, same as PISTOL_ATTACK_HIT_TICK - the round actually leaves the
// barrel immediately in Playercontroller.hpp (see the pistol's "before the
// firing animation even plays its first frame" comment there), not on a
// mid-animation tick, so this only exists to keep WeaponControl's per-weapon
// attackHitTick() dispatch uniform across every weapon.
const int AK47_ATTACK_HIT_TICK = 4;

// Fire the magazine back to back and the gun needs to cool down for 2
// seconds before it'll fire again - same shape as Pistol's 6-shot limiter,
// just a bigger magazine. AK47_BASE_MAX_ROUNDS is what the hero carries once
// he picks the AK47 up in Level 1 (36 rounds); AK47_LEVEL2_MAX_ROUNDS is the
// bigger magazine the gun gets refilled to the moment he warps into Level 2
// (see AK47::refill() below and initLevel2() in Level2.hpp) - both feed the
// same runtime maxRounds field rather than a single fixed constant, since
// the cap now changes mid-game instead of being fixed for the whole run.
const int AK47_BASE_MAX_ROUNDS = 24;
const int AK47_LEVEL2_MAX_ROUNDS = 100;
const int AK47_COOLDOWN_MS = 2000; // 2 second cooldown, real time (not ticks)

struct AK47 {
	int runImgR[4];
	int runImgL[4];

	int jumpImgR[4];
	int jumpImgL[4];

	int atkImgR[AK47_ATK_FRAMES];
	int atkImgL[AK47_ATK_FRAMES];

	unsigned int bulletImgR = (unsigned int)-1;
	unsigned int bulletImgL = (unsigned int)-1;

	// rounds fired since the counter last reset (either by hitting the cap
	// below, or never yet) - same "bullet counter" idea as
	// Pistol::bulletsFired, just a bigger magazine.
	int  roundsFired = 0;
	bool inCooldown = false;
	long cooldownStartMs = 0;

	// how many rounds the current magazine holds before cooldown kicks in -
	// starts at the Level 1 pickup size and gets bumped up by refill() the
	// moment Level 2 starts (see initLevel2() in Level2.hpp).
	int  maxRounds = AK47_BASE_MAX_ROUNDS;

	void loadAssets() {
		loadImage(runImgR, 4, "images\\hero\\running\\AK47", "R");
		loadImage(runImgL, 4, "images\\hero\\running\\AK47", "L");

		loadImage(jumpImgR, 4, "images\\hero\\jumping\\AK47", "R");
		loadImage(jumpImgL, 4, "images\\hero\\jumping\\AK47", "L");

		loadImage(atkImgR, AK47_ATK_FRAMES, "images\\hero\\attack\\AK47", "R");
		loadImage(atkImgL, AK47_ATK_FRAMES_L_ON_DISK, "images\\hero\\attack\\AK47", "L");

		// no L_6 on disk yet - repeat the last real left frame into the
		// remaining slot(s) so atkImgL[] is valid at every index atkImgR[]
		// is, without WeaponControl::atkImg() needing to know one side is
		// short a frame. Drop this loop once L_6 exists.
		for (int i = AK47_ATK_FRAMES_L_ON_DISK; i < AK47_ATK_FRAMES; i++)
			atkImgL[i] = atkImgL[AK47_ATK_FRAMES_L_ON_DISK - 1];

		bulletImgR = iLoadImage((char*)"images\\hero\\attack\\AK47\\R_Bullet.png");
		bulletImgL = iLoadImage((char*)"images\\hero\\attack\\AK47\\L_Bullet.png");
	}

	// call this the instant a round actually leaves the barrel (see
	// Playercontroller.hpp). Counts it, and the moment the count reaches
	// maxRounds, resets the counter back to zero and starts the cooldown -
	// identical shape to Pistol::registerShotFired().
	void registerShotFired() {
		roundsFired++;
		if (roundsFired >= maxRounds) {
			roundsFired = 0;
			inCooldown = true;
			cooldownStartMs = (long)(clock() * 1000L / CLOCKS_PER_SEC);
		}
	}

	// Tops the magazine back up and clears any cooldown in progress, and
	// sets a new cap - used when entering Level 2 to bump the AK47 from its
	// Level 1 pickup size (36) up to the bigger Level 2 magazine (100). Call
	// with AK47_BASE_MAX_ROUNDS instead to reset it back down (e.g. a fresh
	// game/death back at the start of Level 1 - see resetGameState()).
	void refill(int newMaxRounds) {
		roundsFired = 0;
		inCooldown = false;
		maxRounds = newMaxRounds;
	}

	// call every tick - clears inCooldown once AK47_COOLDOWN_MS of real
	// time has passed since the cooldown started.
	void updateCooldown() {
		if (!inCooldown) return;

		long now = (long)(clock() * 1000L / CLOCKS_PER_SEC);
		if (now - cooldownStartMs >= AK47_COOLDOWN_MS)
			inCooldown = false;
	}

	bool canFire() { return !inCooldown; }
};

#endif