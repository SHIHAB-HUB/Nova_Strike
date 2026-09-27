#ifndef KNIFE_HPP
#define KNIFE_HPP

#include "Utility.hpp"

// Owner: [Gameplay / Physics teammate]
//
// One weapon = its animation frames + its stats. Loading follows the same
// R_#/L_# on-disk convention and the same loadImage() helper Hero.hpp uses,
// so this is just Hero's asset-loading style pulled out per-weapon.

// DAMAGE PASS: 100 -> 175 (x1.75). The knife is the only weapon available
// from the very first second of the game AND the slowest swing in the
// roster (KNIFE_ATTACK_TICKS = 36 below, vs the pistol's 20), so at 100 it
// was strictly worse than everything else once the guns unlocked. At 175 it
// two-shots the Enemy 1/2 trash tiers and stays a real answer when a gun is
// on cooldown, without ever competing with the ranged weapons against the
// 700-1000 HP heavies - it still has to be in melee range to land at all.
const int KNIFE_DMG = 175;       // damage dealt per hit
const int KNIFE_ATK_FRAMES = 6;  // R_1..R_6 / L_1..L_6 under attack\knife

// How long a slash takes to play out, in ticks - same numbers Hero.hpp used
// to hardcode as the shared ATTACK_TICKS/ATTACK_HIT_TICK before each weapon
// got its own pace (see PISTOL_ATTACK_TICKS in Pistol.hpp for the faster
// pistol version). Unchanged from before, so the knife still feels exactly
// like it used to.
const int KNIFE_ATTACK_TICKS = 36;    // how long the attack animation takes
const int KNIFE_ATTACK_HIT_TICK = 18; // which tick of the attack actually hits

struct Knife {
	int runImgR[4];
	int runImgL[4];

	int jumpImgR[4];
	int jumpImgL[4];

	int atkImgR[KNIFE_ATK_FRAMES];
	int atkImgL[KNIFE_ATK_FRAMES];

	void loadAssets() {
		loadImage(runImgR, 4, "images\\hero\\running\\Knife", "R");
		loadImage(runImgL, 4, "images\\hero\\running\\Knife", "L");

		loadImage(jumpImgR, 4, "images\\hero\\jumping\\Knife", "R");
		loadImage(jumpImgL, 4, "images\\hero\\jumping\\Knife", "L");

		loadImage(atkImgR, KNIFE_ATK_FRAMES, "images\\hero\\attack\\knife", "R");
		loadImage(atkImgL, KNIFE_ATK_FRAMES, "images\\hero\\attack\\knife", "L");
	}
};

#endif