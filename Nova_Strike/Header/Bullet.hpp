#ifndef BULLET_HPP
#define BULLET_HPP

#include "iGraphics.h"
#include "Pistol.hpp"   // BULLET_SPEED lives there - it's a property of the gun(s) firing it

// Owner: [Gameplay / Physics teammate]
//
// Generic flying-projectile pool - any weapon that fires a straight-line
// bullet (currently the pistol and the AK47 - see Header/Pistol.hpp,
// Header/AK47.hpp) shares this same pool instead of each weapon needing its
// own. What sprite to draw and how much damage to deal are resolved by
// WHICHEVER WEAPON FIRED IT at the moment it's spawned (see spawnBullet()
// below, called from Playercontroller.hpp) and baked into the Bullet itself -
// that way a bullet already in flight keeps its own gun's look/damage even
// if the hero switches weapons before it lands.
//
// Every bullet flies at BULLET_SPEED px/tick (see Pistol.hpp) until either
// it leaves the visible screen, it hits an enemy (damageEnemiesInBox() in
// enemy.h, called from updateBullets() in level.hpp - chips its own dmg off
// that enemy's health, killing it once health hits 0), or it touches a
// solid (non-zero) tile in levelGrid - i.e. any non-zero value in
// data/levels/level1.txt - at which point it stops (goes inactive and is no
// longer drawn/updated, and per the "don't let a bullet reach off-screen
// enemies" fix, so does going past the edge of the visible screen instead of
// only the whole map's edge).
//
// Same shape as enemy.h's enemyList: a fixed-size pool + active flag, no
// dynamic allocation.

const int BULLET_WIDTH = 30;
const int BULLET_HEIGHT = 18;
const int MAX_BULLETS = 32;

// Raises the bullet's spawn point above the hero's gun-arm center so it
// lines up with the barrel a bit better - see muzzleY in
// Playercontroller.hpp, which is where this actually gets applied.
const int BULLET_MUZZLE_Y_OFFSET = 4;

struct Bullet {
	float x, y;
	int   dir;     // -1 = flying left, +1 = flying right
	int   dmg;     // damage this bullet deals on hit - whichever gun fired it (PISTOL_DMG/AK47_DMG/...)
	unsigned int img; // sprite to draw - resolved once at spawn time (see WeaponControl::bulletImg())
	bool  active;

	// true only for bolts fired by the Plasma Rifle (see PlasmaRifle.hpp) -
	// updateBullets() in level.hpp recomputes THIS bullet's dmg by
	// hero-to-target distance right before it can land a hit, instead of
	// just using the flat dmg baked in above at spawn time like every other
	// bullet does.
	bool  isPlasma;
};

static Bullet bulletList[MAX_BULLETS];
// bulletList is a static/global array, so every slot starts zeroed
// (active == false) automatically - no separate init pass needed before
// the first spawnBullet() call.

// fires a bullet from (muzzleX, muzzleY) - the hero's gun position at the
// moment he fired - flying in dir (-1 left / +1 right), dealing dmg on
// whatever it hits, drawn with img (both resolved by the caller from
// whichever weapon actually fired it - see WeaponControl::damage()/
// bulletImg() in WeaponControl.hpp). Drops silently if every slot is
// already in flight (32 at once is already far more than one hero could
// ever have on screen).
// isPlasma (defaults to false, so every existing pistol/AK47 call site is
// unaffected) marks a bolt fired by the Plasma Rifle - see Bullet::isPlasma
// above and updateBullets() in level.hpp, which is where that flag actually
// changes anything.
inline void spawnBullet(int muzzleX, int muzzleY, int dir, int dmg, unsigned int img, bool isPlasma = false) {
	for (int i = 0; i < MAX_BULLETS; i++) {
		if (!bulletList[i].active) {
			bulletList[i].x = (float)muzzleX;
			bulletList[i].y = (float)muzzleY;
			bulletList[i].dir = (dir >= 0) ? 1 : -1;
			bulletList[i].dmg = dmg;
			bulletList[i].img = img;
			bulletList[i].active = true;
			bulletList[i].isPlasma = isPlasma;
			return;
		}
	}
}

// --- plasma bolt draw size ---------------------------------------------
// The plasma bolt shares the generic Bullet pool (and therefore the generic
// BULLET_WIDTH x BULLET_HEIGHT collision box), but PlasmaBullet.png is a
// 188x111 sprite - squeezing it into the 30x18 pistol-round box made it a
// barely visible smear that read as "the plasma bullet isn't animating".
// It now draws at its own larger size and correct aspect ratio, CENTERED on
// the same collision box every other bullet uses, so the visual grew
// without the bolt's hitbox changing at all.
const int PLASMA_BULLET_DRAW_WIDTH = 54;
const int PLASMA_BULLET_DRAW_HEIGHT = 32;   // 54:32 ~= PlasmaBullet.png's 188:111

// Draws every bullet currently in flight, camera-relative like the tiles/
// hero/enemies in level.hpp's drawGameLevel().
inline void drawBullets(float cameraX, float cameraY) {
	for (int i = 0; i < MAX_BULLETS; i++) {
		if (!bulletList[i].active) continue;
		if (bulletList[i].img == (unsigned int)-1) continue;

		int drawW = BULLET_WIDTH;
		int drawH = BULLET_HEIGHT;

		if (bulletList[i].isPlasma) {
			drawW = PLASMA_BULLET_DRAW_WIDTH;
			drawH = PLASMA_BULLET_DRAW_HEIGHT;
		}

		// centre the (possibly larger) sprite on the collision box
		int drawX = (int)(bulletList[i].x - cameraX) - (drawW - BULLET_WIDTH) / 2;
		int drawY = (int)(bulletList[i].y - cameraY) - (drawH - BULLET_HEIGHT) / 2;

		// The plasma sprite has no left-facing variant on disk (see
		// PlasmaRifle::bulletImg), so a left-flying bolt is the same art
		// mirrored - a negative width hands iShowImage a reversed quad,
		// which works because nothing here enables GL_CULL_FACE.
		if (bulletList[i].isPlasma && bulletList[i].dir < 0)
			iShowImage(drawX + drawW, drawY, -drawW, drawH, bulletList[i].img);
		else
			iShowImage(drawX, drawY, drawW, drawH, bulletList[i].img);
	}
}

#endif
