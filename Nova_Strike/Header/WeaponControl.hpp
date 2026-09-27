// ===== Header/WeaponControl.hpp =====
#ifndef WEAPONCONTROL_HPP
#define WEAPONCONTROL_HPP

#include "Knife.hpp"
#include "Pistol.hpp"
#include "AK47.hpp"
#include "Grenade.hpp"
#include "FlameThrower.hpp"
#include "PlasmaRifle.hpp"

// Owner: [Gameplay / Physics teammate]
//
// Add a new weapon by giving it its own header the same shape as
// Knife.hpp/Pistol.hpp, adding a case here, and adding it as a member
// below - Hero.hpp never needs to change.
enum WeaponType {
	WEAPON_KNIFE = 0,
	WEAPON_PISTOL = 1,
	WEAPON_AK47 = 2,
	WEAPON_GRENADE = 3,

	// Key '4' - unlocked by solving Puzzle 4 (Weather Node, Level 2 / Door 2 -
	// see grantFlamethrowerAndKey() in PuzzleCommon.hpp). Also the only
	// weapon that can melt the ice wall in the Level 2 side quest (see
	// SideQuest.hpp).
	WEAPON_FLAMETHROWER = 4,

	// The 5th weapon added to the roster - key '5'. Unlocked once the Space
	// Stone is secured (hasSpaceStone - see Playercontroller.hpp) rather
	// than through a puzzle of its own. See PlasmaRifle.hpp.
	WEAPON_PLASMARIFLE = 5
};

// Owns every weapon the hero can carry and tracks which one is currently
// equipped. "Grabbing" a weapon is just equip() flipping that state.
// Hero.hpp asks this for whichever image/damage the active weapon needs
// instead of loading or picking weapon art itself.

struct WeaponControl {
	Knife knife;
	Pistol pistol;
	AK47 ak47;
	Grenade grenade;
	FlameThrower flamethrower;
	PlasmaRifle plasmarifle;

	// FIX: this said "knife out by default" but actually initialized to
	// WEAPON_AK47, so every fresh game (and every death/respawn, since
	// resetGameState() never touched this either) started the hero holding
	// the AK47 despite it not being unlocked yet. Now it actually is the
	// knife - the AK47 is unlocked for real by solving the frequency puzzle
	// (see hasAK47Pickup in Variables.h, granted by completePuzzle() in
	// Puzzlecommon.hpp).
	WeaponType equipped = WEAPON_KNIFE; // knife out by default

	void loadAssets() {
		knife.loadAssets();
		pistol.loadAssets();
		ak47.loadAssets();
		grenade.loadAssets();
		flamethrower.loadAssets();
		plasmarifle.loadAssets();
	}

	void equip(WeaponType w) { equipped = w; }

	// how many attack frames the active weapon has (knife slashes over 6
	// frames, the pistol fires over 4, ...) - Hero paces atkIndx off this
	// instead of a hard-coded frame count.
	int atkFrameCount() {
		if (equipped == WEAPON_KNIFE)        return KNIFE_ATK_FRAMES;
		if (equipped == WEAPON_PISTOL)       return PISTOL_ATK_FRAMES;
		if (equipped == WEAPON_AK47)         return AK47_ATK_FRAMES;
		if (equipped == WEAPON_GRENADE)      return GRENADE_ATK_FRAMES;
		if (equipped == WEAPON_PLASMARIFLE)  return PLASMA_RIFLE_ATK_FRAMES;
		return FLAMETHROWER_ATK_FRAMES;
	}

	int damage() {
		if (equipped == WEAPON_KNIFE)        return KNIFE_DMG;
		if (equipped == WEAPON_PISTOL)       return PISTOL_DMG;
		if (equipped == WEAPON_AK47)         return AK47_DMG;
		if (equipped == WEAPON_GRENADE)      return GRENADE_DMG;
		// The flat max - a plasma bolt's ACTUAL landed damage is scaled down
		// by range at hit time (see plasmaDamageAtRange() in PlasmaRifle.hpp,
		// called from updateBullets() in level.hpp), but this is still what
		// gets baked into the bullet at spawn time as its starting value.
		if (equipped == WEAPON_PLASMARIFLE)  return PLASMA_RIFLE_DMG;
		return FLAMETHROWER_DMG;
	}

	// true for the knife (melee - needs solid footing, and locks movement/
	// jumping while it plays), false for every ranged weapon (pistol, AK47,
	// grenade, flamethrower - can be used while running or mid-air, and
	// never freezes movement). See Hero::moveHorizontal()/startJump()/
	// startAttack() in Hero.hpp.
	bool isMelee() {
		return equipped == WEAPON_KNIFE;
	}

	// how many ticks the active weapon's attack animation takes, and which
	// tick of it actually lands the hit/fires the shot - per-weapon so the
	// pistol/AK47 can play faster than the knife's slash (see
	// PISTOL_ATTACK_TICKS in Pistol.hpp, AK47_ATTACK_TICKS in AK47.hpp).
	// Hero::updateAttack() paces itself off these instead of a single
	// shared constant.
	int attackTicks() {
		if (equipped == WEAPON_KNIFE)        return KNIFE_ATTACK_TICKS;
		if (equipped == WEAPON_PISTOL)       return PISTOL_ATTACK_TICKS;
		if (equipped == WEAPON_AK47)         return AK47_ATTACK_TICKS;
		if (equipped == WEAPON_GRENADE)      return GRENADE_ATTACK_TICKS;
		if (equipped == WEAPON_PLASMARIFLE)  return PLASMA_RIFLE_ATTACK_TICKS;
		return FLAMETHROWER_ATTACK_TICKS;
	}

	int attackHitTick() {
		if (equipped == WEAPON_KNIFE)        return KNIFE_ATTACK_HIT_TICK;
		if (equipped == WEAPON_PISTOL)       return PISTOL_ATTACK_HIT_TICK;
		if (equipped == WEAPON_AK47)         return AK47_ATTACK_HIT_TICK;
		if (equipped == WEAPON_GRENADE)      return GRENADE_ATTACK_HIT_TICK;
		if (equipped == WEAPON_PLASMARIFLE)  return PLASMA_RIFLE_ATTACK_HIT_TICK;
		return FLAMETHROWER_ATTACK_HIT_TICK;
	}

	// false only while the pistol/AK47 is on its post-magazine cooldown -
	// every other weapon has no ammo limit modeled yet, so it's always true
	// for them. Hero::startAttack() checks this before letting a new attack
	// start.
	bool canFire() {
		if (equipped == WEAPON_PISTOL)       return pistol.canFire();
		if (equipped == WEAPON_AK47)         return ak47.canFire();
		if (equipped == WEAPON_FLAMETHROWER) return flamethrower.canFire();
		return true;
	}

	// counts a bullet/round that actually left the barrel toward the
	// pistol's 6-shot or the AK47's 24-round cooldown limiter. No-op for
	// every other weapon.
	void registerShotFired() {
		if (equipped == WEAPON_PISTOL) pistol.registerShotFired();
		if (equipped == WEAPON_AK47)   ak47.registerShotFired();
	}

	// call every tick regardless of what's equipped, so a cooldown already
	// in progress keeps counting down even if the player switches to
	// another weapon mid-cooldown.
	void updateCooldowns() {
		pistol.updateCooldown();
		ak47.updateCooldown();
		flamethrower.updateCooldown();
	}

	// which sprite a bullet fired by the CURRENTLY equipped gun should use -
	// only meaningful while equipped is WEAPON_PISTOL or WEAPON_AK47 (the
	// only two weapons that fire a Bullet.hpp projectile). Resolved once at
	// the moment of firing (see Playercontroller.hpp) and baked into the
	// Bullet itself, so a bullet already in flight keeps its own sprite even
	// if the hero switches weapons afterward.
	unsigned int bulletImg(int facing) {
		if (equipped == WEAPON_AK47)        return (facing >= 0) ? ak47.bulletImgR : ak47.bulletImgL;
		// single shared sprite, no left/right variant on disk - see
		// PlasmaRifle::bulletImg in PlasmaRifle.hpp.
		if (equipped == WEAPON_PLASMARIFLE) return plasmarifle.bulletImg;
		return (facing >= 0) ? pistol.bulletImgR : pistol.bulletImgL;
	}

	int runImg(int facing, int index) {
		if (equipped == WEAPON_KNIFE)   return (facing >= 0) ? knife.runImgR[index] : knife.runImgL[index];
		if (equipped == WEAPON_AK47)    return (facing >= 0) ? ak47.runImgR[index] : ak47.runImgL[index];
		if (equipped == WEAPON_FLAMETHROWER) return (facing >= 0) ? flamethrower.runImgR[index] : flamethrower.runImgL[index];
		if (equipped == WEAPON_PLASMARIFLE)  return (facing >= 0) ? plasmarifle.runImgR[index] : plasmarifle.runImgL[index];
		// grenade has no running art of its own (see Grenade.hpp) - fall
		// back to the pistol's stance rather than showing nothing.
		return (facing >= 0) ? pistol.runImgR[index] : pistol.runImgL[index];
	}

	int jumpImg(int facing, int index) {
		if (equipped == WEAPON_KNIFE)   return (facing >= 0) ? knife.jumpImgR[index] : knife.jumpImgL[index];
		if (equipped == WEAPON_AK47)    return (facing >= 0) ? ak47.jumpImgR[index] : ak47.jumpImgL[index];
		if (equipped == WEAPON_FLAMETHROWER) return (facing >= 0) ? flamethrower.jumpImgR[index] : flamethrower.jumpImgL[index];
		if (equipped == WEAPON_PLASMARIFLE)  return (facing >= 0) ? plasmarifle.jumpImgR[index] : plasmarifle.jumpImgL[index];
		// grenade has no jumping art of its own (see Grenade.hpp) - same
		// pistol fallback as runImg() above.
		return (facing >= 0) ? pistol.jumpImgR[index] : pistol.jumpImgL[index];
	}

	int atkImg(int facing, int index) {
		if (equipped == WEAPON_KNIFE)        return (facing >= 0) ? knife.atkImgR[index] : knife.atkImgL[index];
		if (equipped == WEAPON_AK47)         return (facing >= 0) ? ak47.atkImgR[index] : ak47.atkImgL[index];
		if (equipped == WEAPON_GRENADE)      return (facing >= 0) ? grenade.atkImgR[index] : grenade.atkImgL[index];
		if (equipped == WEAPON_FLAMETHROWER) return (facing >= 0) ? flamethrower.atkImgR[index] : flamethrower.atkImgL[index];
		if (equipped == WEAPON_PLASMARIFLE)  return (facing >= 0) ? plasmarifle.atkImgR[index] : plasmarifle.atkImgL[index];
		return (facing >= 0) ? pistol.atkImgR[index] : pistol.atkImgL[index];
	}
};

#endif