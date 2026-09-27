// ===== Header/PlasmaRifle.hpp =====
#ifndef PLASMARIFLE_HPP
#define PLASMARIFLE_HPP

#include "Utility.hpp"

// Owner: [Gameplay / Physics teammate]
//
// The 5th weapon in the roster (Knife/Pistol/AK47/Grenade/PlasmaRifle) -
// see WEAPON_PLASMARIFLE in WeaponControl.hpp, bound to key '5'. Unlike
// every other weapon it has no puzzle of its own to unlock it - key '5'
// checks hasSpaceStone directly (see Playercontroller.hpp), so it becomes
// available the moment the Space Stone is secured at the end of the Level 2
// side quest (SideQuest.hpp). The FlameThrower keeps its own key-'4' slot
// (unlocked by Puzzle 4 - Weather Node - instead, see PuzzleCommon.hpp);
// the two weapons don't compete for a slot.
//
// Same shape as Pistol.hpp/AK47.hpp - running/jumping/attack frames off the
// "images\hero\<pose>\Plasma Rifel" folders (yes, "Rifel" - that's the exact
// spelling baked into the asset folder names on disk, kept as-is here rather
// than "fixed", same as Pistol.hpp keeping the "Pristol Icon.png" typo) -
// plus its own bullet sprite. What makes it different from the pistol/AK47's
// flat per-shot damage is the range-based falloff below (see
// plasmaDamageAtRange()) - a bolt that lands point-blank hits for the full
// PLASMA_RIFLE_DMG, and the further away the target was, the less it deals,
// down to a floor of PLASMA_RIFLE_MIN_DMG.

// DAMAGE PASS: 200 -> 450 point-blank, 100 -> 260 at maximum range.
//
// This is the LAST weapon in the game - it is handed over at the end of the
// Level 2 side quest and it is what the Titan fight is meant to be won with,
// so it has to out-damage everything earned before it by a clear margin. It
// also has no magazine and no cooldown of its own (see the note on
// loadAssets() below): firing is paced purely by PLASMA_RIFLE_ATTACK_TICKS
// (24 ticks, ~0.72s per bolt), which is what keeps 450 from being absurd -
// it works out to roughly 620 damage per second at point-blank, so the
// 5000 HP first life plus the 10000 HP revived life is still a fight of
// real length rather than a two-second execution.
//
// The falloff spread was widened along with it (100->260 floor rather than
// staying at 100) so that the "get close for more damage" incentive is still
// worth acting on - the gap between a point-blank bolt and a cross-arena one
// is now 190 rather than 100 - while a cautious player kiting Thanos from
// range still does meaningfully more than the AK47 would.
const int PLASMA_RIFLE_DMG = 450;        // max damage - point-blank hit
const int PLASMA_RIFLE_MIN_DMG = 260;    // damage floor - never drops below this, no matter the range
const int PLASMA_RIFLE_ATK_FRAMES = 4;   // R_1..R_4 / L_1..L_4 under attack\Plasma Rifel

// Pacing sits between the pistol's snappy 20-tick shot (PISTOL_ATTACK_TICKS
// in Pistol.hpp) and the knife's 36-tick slash - a heavier single bolt than
// the pistol, without being as slow as a melee swing.
const int PLASMA_RIFLE_ATTACK_TICKS = 24;    // how long the firing animation takes
const int PLASMA_RIFLE_ATTACK_HIT_TICK = 12; // which tick the bolt actually leaves the barrel

// Only 3 jumping frames exist on disk for the LEFT side (L_1..L_3, no L_4
// yet) vs 4 for the right (R_1..R_4) - same asymmetric-art situation
// AK47.hpp already hit with its attack frames. Hero::currentImage() only
// ever reads jumpImg(facing, 2) - a single fixed mid-air pose (see
// Hero.hpp) - so index 2 is all that actually has to be valid; loadAssets()
// below still pads the unused index 3 by repeating the last real frame,
// purely so jumpImgL[] stays the same 4-slot shape as every other weapon's.
const int PLASMA_RIFLE_JUMP_FRAMES_L_ON_DISK = 3;

// How far (in px) the damage falloff plays out over. A hit landing right on
// top of the target (0px hero-to-target gap) deals the full
// PLASMA_RIFLE_DMG (200); the damage drops off linearly as that gap grows,
// reaching the floor PLASMA_RIFLE_MIN_DMG (100) once the gap hits this many
// pixels, and holds flat at the floor beyond that.
const int PLASMA_RIFLE_FALLOFF_PX = 500;

// Scales a plasma hit's damage by how far the target was from the hero the
// instant the bolt actually landed (see updateBullets() in level.hpp, the
// only caller - it recomputes this every hit instead of using the flat dmg
// baked into the bullet at spawn time like every other weapon's shots do).
// Point-blank: full PLASMA_RIFLE_DMG. Every pixel of extra distance chips a
// proportional amount off, down to the PLASMA_RIFLE_MIN_DMG floor at
// PLASMA_RIFLE_FALLOFF_PX or beyond. Every other weapon in the game deals a
// flat, fixed amount per hit - the plasma rifle is the only one where "how
// close were you" changes the number.
inline int plasmaDamageAtRange(int distancePx) {
	if (distancePx <= 0) return PLASMA_RIFLE_DMG;

	int falloff = (distancePx * (PLASMA_RIFLE_DMG - PLASMA_RIFLE_MIN_DMG)) / PLASMA_RIFLE_FALLOFF_PX;
	int dmg = PLASMA_RIFLE_DMG - falloff;

	if (dmg < PLASMA_RIFLE_MIN_DMG) dmg = PLASMA_RIFLE_MIN_DMG;
	if (dmg > PLASMA_RIFLE_DMG)     dmg = PLASMA_RIFLE_DMG;
	return dmg;
}

struct PlasmaRifle {
	int runImgR[4];
	int runImgL[4];

	int jumpImgR[4];
	int jumpImgL[4];

	int atkImgR[PLASMA_RIFLE_ATK_FRAMES];
	int atkImgL[PLASMA_RIFLE_ATK_FRAMES];

	// One shared bolt sprite for both directions - unlike the pistol/AK47
	// (separate R_Bullet.png/L_Bullet.png), the art on disk is a single
	// PlasmaBullet.png with no left/right variant.
	unsigned int bulletImg = (unsigned int)-1;

	// No magazine/cooldown modeled yet (unlike Pistol/AK47) - firing is
	// gated purely by the attack animation itself (can't start a new shot
	// until the current one finishes, same as the knife - see
	// Hero::startAttack()/WeaponControl::canFire()).
	void loadAssets() {
		loadImage(runImgR, 4, "images\\hero\\running\\Plasma Rifel", "R");
		loadImage(runImgL, 4, "images\\hero\\running\\Plasma Rifel", "L");

		loadImage(jumpImgR, 4, "images\\hero\\jumping\\Plasma Rifel", "R");
		loadImage(jumpImgL, PLASMA_RIFLE_JUMP_FRAMES_L_ON_DISK, "images\\hero\\jumping\\Plasma Rifel", "L");
		// no L_4 on disk yet - pad the unused slot the same way AK47.hpp
		// pads its own short (attack) side - see AK47::loadAssets().
		for (int i = PLASMA_RIFLE_JUMP_FRAMES_L_ON_DISK; i < 4; i++)
			jumpImgL[i] = jumpImgL[PLASMA_RIFLE_JUMP_FRAMES_L_ON_DISK - 1];

		loadImage(atkImgR, PLASMA_RIFLE_ATK_FRAMES, "images\\hero\\attack\\Plasma Rifel", "R");
		loadImage(atkImgL, PLASMA_RIFLE_ATK_FRAMES, "images\\hero\\attack\\Plasma Rifel", "L");

		bulletImg = iLoadImage((char*)"images\\hero\\attack\\Plasma Rifel\\PlasmaBullet.png");
	}
};

#endif