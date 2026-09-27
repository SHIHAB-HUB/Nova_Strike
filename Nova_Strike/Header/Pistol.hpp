#ifndef PISTOL_HPP
#define PISTOL_HPP

#include <ctime>
#include "Utility.hpp"

// Owner: [Gameplay / Physics teammate]
//
// Same shape as Knife.hpp - running/jumping/attack frames + a damage
// constant - just pointed at the Pistol art folders instead. Also owns
// everything specific to firing a shot: the bullet's flight speed, the
// firing animation's pacing, and the 6-shots-then-cooldown limiter.

// DAMAGE PASS: 200 -> 320 (x1.6). The pistol is the first gun the player
// earns and it is hard-limited to 6 shots before a 2 second cooldown
// (PISTOL_BULLETS_BEFORE_COOLDOWN / PISTOL_COOLDOWN_MS below), so its real
// damage-per-second is capped by that magazine no matter what this number
// is - which is exactly why it can afford to hit hard per shot. At 320 a
// full magazine is 1920 damage: enough to clear any of the mid tiers, still
// short of the 5000 HP Thanos opens with.
const int PISTOL_DMG = 320;       // damage dealt per shot (bullet AND melee-range hitbox)
const int PISTOL_ATK_FRAMES = 4;  // R_1..R_4 / L_1..L_4 under attack\Pistol

// How long firing plays out, in ticks - noticeably faster than the knife's
// slash (see KNIFE_ATTACK_TICKS in Knife.hpp) so the pistol reads as a
// snappy quick-draw instead of the same pace as a melee swing.
const int PISTOL_ATTACK_TICKS = 20;    // how long the firing animation takes
const int PISTOL_ATTACK_HIT_TICK = 10; // which tick the shot actually leaves the barrel

// Bullet flight speed, px/tick - lives here (not Bullet.hpp) since it's a
// property of the gun firing the bullet, not of bullets in general.
// Bullet.hpp includes this header to read it. Slowed down from 20 -> 12
// so shots read as a visible traveling bullet instead of an near-instant
// hitscan-looking snap across the screen.
const int BULLET_SPEED = 12;

// Fire 6 shots back to back and the gun needs to cool down for 2 seconds
// before it'll fire again - see Pistol::registerShotFired()/canFire() below.
const int PISTOL_BULLETS_BEFORE_COOLDOWN = 6;
const int PISTOL_COOLDOWN_MS = 2000; // 2 second cooldown, real time (not ticks)

struct Pistol {
	int runImgR[4];
	int runImgL[4];

	int jumpImgR[4];
	int jumpImgL[4];

	int atkImgR[PISTOL_ATK_FRAMES];
	int atkImgL[PISTOL_ATK_FRAMES];

	unsigned int bulletImgR = (unsigned int)-1;
	unsigned int bulletImgL = (unsigned int)-1;

	// shots fired since the counter last reset (either by hitting the cap
	// below, or never yet) - the "bullet counter" that counts up after
	// every bullet launched.
	int  bulletsFired = 0;
	bool inCooldown = false;
	long cooldownStartMs = 0;

	void loadAssets() {
		loadImage(runImgR, 4, "images\\hero\\running\\Pistol", "R");
		loadImage(runImgL, 4, "images\\hero\\running\\Pistol", "L");

		loadImage(jumpImgR, 4, "images\\hero\\jumping\\Pistol", "R");
		loadImage(jumpImgL, 4, "images\\hero\\jumping\\Pistol", "L");

		loadImage(atkImgR, PISTOL_ATK_FRAMES, "images\\hero\\attack\\Pistol", "R");
		loadImage(atkImgL, PISTOL_ATK_FRAMES, "images\\hero\\attack\\Pistol", "L");

		bulletImgR = iLoadImage((char*)"images\\hero\\attack\\Pistol\\R_Bullet.png");
		bulletImgL = iLoadImage((char*)"images\\hero\\attack\\Pistol\\L_Bullet.png");
	}

	// call this the instant a bullet actually leaves the barrel (see
	// Playercontroller.hpp). Counts it, and the moment the count reaches
	// PISTOL_BULLETS_BEFORE_COOLDOWN (6), resets the counter back to zero
	// and starts the cooldown.
	void registerShotFired() {
		bulletsFired++;
		if (bulletsFired >= PISTOL_BULLETS_BEFORE_COOLDOWN) {
			bulletsFired = 0;
			inCooldown = true;
			cooldownStartMs = (long)(clock() * 1000L / CLOCKS_PER_SEC);
		}
	}

	// call every tick - clears inCooldown once PISTOL_COOLDOWN_MS of real
	// time has passed since the cooldown started.
	void updateCooldown() {
		if (!inCooldown) return;

		long now = (long)(clock() * 1000L / CLOCKS_PER_SEC);
		if (now - cooldownStartMs >= PISTOL_COOLDOWN_MS)
			inCooldown = false;
	}

	bool canFire() { return !inCooldown; }
};

#endif