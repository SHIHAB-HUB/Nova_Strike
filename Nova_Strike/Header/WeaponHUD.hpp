// ===== Header/WeaponHUD.hpp =====
#ifndef WEAPONHUD_HPP
#define WEAPONHUD_HPP

// Owner: [UI/Menus teammate]
//
// Top-right HUD readout for whichever weapon is currently equipped - same
// self-contained shape as Header/Health.hpp (owns its own icon textures,
// one loadXAssets() call + one drawX() call). Shows:
//   Knife        - Knife Icon.png + an infinity mark (unlimited swings)
//   Pistol/AK47  - their icon + "X <n>", remaining rounds before that
//                  gun's own magazine-cooldown kicks in
//   FlameThrower - FlameThrower Icon.png + "X <n>", seconds left before the
//                  10s continuous-fire limit forces a cooldown - or, once
//                  it's actually cooling down, seconds left on that instead
//                  (only ever equipped during the Level 2 side quest now -
//                  see WEAPON_FLAMETHROWER in WeaponControl.hpp)
//   PlasmaRifle  - PlasmaRifle.png + an infinity mark, same as the knife -
//                  no magazine/cooldown modeled for it (see PlasmaRifle.hpp)
//   Grenade      - no icon asset exists for it yet, so nothing is drawn

#include "iGraphics.h"
#include "Hero.hpp"
#include <cstdio>
#include <ctime>

const int WEAPON_ICON_SIZE = 56;
const int WEAPON_HUD_MARGIN_X = 20;
const int WEAPON_HUD_MARGIN_Y = 50;
const int WEAPON_HUD_TEXT_GAP = 12;   // gap between the icon's right edge and the label
const int WEAPON_HUD_TEXT_WIDTH = 80; // reserved space for the "X <n>" label / infinity mark

static unsigned int hudKnifeIcon = (unsigned int)-1;
static unsigned int hudPistolIcon = (unsigned int)-1;
static unsigned int hudAK47Icon = (unsigned int)-1;
static unsigned int hudFlameIcon = (unsigned int)-1;
static unsigned int hudPlasmaRifleIcon = (unsigned int)-1;

inline void loadWeaponHUDAssets() {
	hudKnifeIcon = iLoadImage((char*)"Images\\UI\\Knife Icon.png");
	// the file on disk is really named "Pristol Icon.png" (typo baked into
	// the asset itself) - loading that exact filename, not "fixing" it here.
	hudPistolIcon = iLoadImage((char*)"Images\\UI\\Pristol Icon.png");
	hudAK47Icon = iLoadImage((char*)"Images\\UI\\AK47 Icon.png");
	hudFlameIcon = iLoadImage((char*)"Images\\UI\\FlameThrower Icon.png");
	hudPlasmaRifleIcon = iLoadImage((char*)"Images\\UI\\PlasmaRifle.png");
}

// Small procedural infinity mark (two touching circle outlines) drawn next
// to the knife icon - there's no "infinity.png" in Images/UI, so this is
// built straight from iCircle() instead of a texture.
inline void drawInfinityMark(int centerX, int centerY, double r) {
	iSetColor(255, 255, 255);
	iCircle(centerX - r, centerY, r, 24);
	iCircle(centerX + r, centerY, r, 24);
}

inline void drawWeaponHUD(Hero& hero) {
	int iconX = SCREEN_WIDTH - WEAPON_HUD_MARGIN_X - WEAPON_HUD_TEXT_WIDTH - WEAPON_HUD_TEXT_GAP - WEAPON_ICON_SIZE;
	int iconY = SCREEN_HEIGHT - WEAPON_HUD_MARGIN_Y - WEAPON_ICON_SIZE;
	int textX = iconX + WEAPON_ICON_SIZE + WEAPON_HUD_TEXT_GAP;
	int textCenterY = iconY + WEAPON_ICON_SIZE / 2;

	WeaponType eq = hero.weapon.equipped;
	unsigned int icon = (unsigned int)-1;
	char label[32] = "";

	if (eq == WEAPON_KNIFE) {
		icon = hudKnifeIcon;
	}
	else if (eq == WEAPON_PISTOL) {
		icon = hudPistolIcon;
		Pistol& p = hero.weapon.pistol;
		// while it's cooling down there are effectively 0 rounds available
		// right now, even though bulletsFired itself was already reset back
		// to 0 the moment the cooldown started (see Pistol::registerShotFired()).
		int remaining = p.inCooldown ? 0 : (PISTOL_BULLETS_BEFORE_COOLDOWN - p.bulletsFired);
		sprintf_s(label, sizeof(label), "X %d", remaining);
	}
	else if (eq == WEAPON_AK47) {
		icon = hudAK47Icon;
		AK47& a = hero.weapon.ak47;
		int remaining = a.inCooldown ? 0 : (a.maxRounds - a.roundsFired);
		sprintf_s(label, sizeof(label), "X %d", remaining);
	}
	else if (eq == WEAPON_FLAMETHROWER) {
		icon = hudFlameIcon;
		FlameThrower& f = hero.weapon.flamethrower;
		long now = (long)(clock() * 1000L / CLOCKS_PER_SEC);
		long remainMs;

		if (f.inCooldown) {
			// already over the limit and cooling down - show time left on
			// THAT instead, since "time before cooldown" no longer applies.
			remainMs = FLAMETHROWER_COOLDOWN_MS - (now - f.cooldownStartMs);
		}
		else if (f.isActive()) {
			// currently streaming - counts down from 10s to 0.
			remainMs = FLAMETHROWER_MAX_FIRE_MS - (now - f.fireStartMs);
		}
		else {
			// idle and ready - the full window is available.
			remainMs = FLAMETHROWER_MAX_FIRE_MS;
		}
		if (remainMs < 0) remainMs = 0;

		int secsLeft = (int)((remainMs + 999) / 1000); // round up to whole seconds
		sprintf_s(label, sizeof(label), "X %d", secsLeft);
	}
	else if (eq == WEAPON_PLASMARIFLE) {
		// no magazine/cooldown modeled (see PlasmaRifle.hpp) - reads the
		// same as the knife's unlimited swings.
		icon = hudPlasmaRifleIcon;
	}
	else {
		return; // grenade (or anything future) - no icon asset for it yet
	}

	if (icon != (unsigned int)-1)
		iShowImage(iconX, iconY, WEAPON_ICON_SIZE, WEAPON_ICON_SIZE, icon);

	if (eq == WEAPON_KNIFE || eq == WEAPON_PLASMARIFLE) {
		drawInfinityMark(textX + WEAPON_HUD_TEXT_WIDTH / 2, textCenterY, 9.0);
	}
	else {
		iSetColor(255, 255, 255);
		iText(textX, textCenterY - 6, label, GLUT_BITMAP_HELVETICA_18);
	}
}

#endif