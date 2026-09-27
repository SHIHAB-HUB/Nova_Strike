#ifndef HEALTH_HPP
#define HEALTH_HPP


#include "iGraphics.h"
#include "Utility.hpp"   
#include "Hero.hpp"     

// On-screen size of the bar ico
// 1869x666 (~2.807:1) - aspect ratio 
const int HEALTHBAR_WIDTH = 210;
const int HEALTHBAR_HEIGHT = (int)(HEALTHBAR_WIDTH / 2.807f);

// gap between x,y axis on up-left courner
const int HEALTHBAR_MARGIN_X = 20;
const int HEALTHBAR_MARGIN_Y = 20;

// Where the transparent hole sits inside healthBar.png, as a fraction of
// the PNG's own width/height - measured directly off the source image
// (1869x666): the hole runs from pixel x=516..1753 and y=252..429, with
// (0,0) at the image's TOP-LEFT corner. Kept as fractions (not raw pixels)
// so the fill lines up with the hole no matter what size HEALTHBAR_WIDTH/
// HEIGHT above draw the bar at.
const float HOLE_LEFT_FRAC = 516.0f / 1869.0f;
const float HOLE_RIGHT_FRAC = 1753.0f / 1869.0f;
const float HOLE_TOP_FRAC = 252.0f / 666.0f;    // distance DOWN from the image's top edge
const float HOLE_BOTTOM_FRAC = 429.0f / 666.0f; // distance DOWN from the image's top edge

// healthBar2.png (Images\hero\Health Bar\) replaces healthBar.png once the
// Space Stone lifts max HP above HERO_BASE_HP (see applySpaceStoneHPBoost()
// in Hero.hpp). It is a WIDER frame: 2417x666 (~3.629:1) versus 1869x666, so
// at the same on-screen height it draws ~269px wide instead of 210px, and
// its hole sits at pixel x=458..2300 (y is identical, 252..428). Measured
// directly off the PNG, same as the first bar's fractions above.
const int   HEALTHBAR2_WIDTH = (int)(HEALTHBAR_HEIGHT * (2417.0f / 666.0f));
const float HOLE2_LEFT_FRAC = 458.0f / 2417.0f;
const float HOLE2_RIGHT_FRAC = 2301.0f / 2417.0f;   // 2300 is the last transparent pixel
const float HOLE2_TOP_FRAC = 252.0f / 666.0f;
const float HOLE2_BOTTOM_FRAC = 429.0f / 666.0f;

static unsigned int healthBarImg = (unsigned int)-1;
static unsigned int healthBar2Img = (unsigned int)-1;


inline void loadHealthAssets() {
	healthBarImg = iLoadImage((char*)"Images\\hero\\Health Bar\\healthBar.png");
	healthBar2Img = iLoadImage((char*)"Images\\hero\\Health Bar\\healthBar2.png");
}


inline void drawHealthBar(Hero& hero) {
	int barX = HEALTHBAR_MARGIN_X;
	int barY = SCREEN_HEIGHT - HEALTHBAR_HEIGHT - HEALTHBAR_MARGIN_Y;

	// Which frame? Space Stone (max HP above base) -> healthBar2, else the
	// original healthBar. Each has its own width and hole position.
	bool stoneBar = (heroMaxHP > HERO_BASE_HP);
	unsigned int frameImg = stoneBar ? healthBar2Img : healthBarImg;
	int   barWidth = stoneBar ? HEALTHBAR2_WIDTH : HEALTHBAR_WIDTH;
	float holeLeftFrac = stoneBar ? HOLE2_LEFT_FRAC : HOLE_LEFT_FRAC;
	float holeRightFrac = stoneBar ? HOLE2_RIGHT_FRAC : HOLE_RIGHT_FRAC;
	float holeTopFrac = stoneBar ? HOLE2_TOP_FRAC : HOLE_TOP_FRAC;
	float holeBottomFrac = stoneBar ? HOLE2_BOTTOM_FRAC : HOLE_BOTTOM_FRAC;

	// the hole's own rectangle, in screen coordinates - fixed regardless of
	// HP; only the fill drawn inside it (below) actually shrinks.
	int holeLeftX = barX + (int)(barWidth * holeLeftFrac);
	int holeRightX = barX + (int)(barWidth * holeRightFrac);
	int holeWidth = holeRightX - holeLeftX;

	// the top/bottom fractions are measured from the image's TOP -
	// flip them to "height up from the bar's BOTTOM edge" (barY) before use,
	// since that's the space iFilledRectangle/iShowImage both draw in.
	int holeBottomY = barY + (int)(HEALTHBAR_HEIGHT * (1.0f - holeBottomFrac));
	int holeTopY = barY + (int)(HEALTHBAR_HEIGHT * (1.0f - holeTopFrac));
	int holeHeight = holeTopY - holeBottomY;

	// the actual HP gauge: a plain red rectangle, anchored to the hole's
	// LEFT edge, whose WIDTH is just holeWidth scaled by the hero's current
	// HP percentage - full HP fills the whole hole, half HP fills half of
	// it (shrinking from the right), zero HP draws nothing.
	int fillWidth = (int)(holeWidth * hero.healthPercent());

	iSetColor(220, 30, 30);
	iFilledRectangle(holeLeftX, holeBottomY, fillWidth, holeHeight);

	// the bar's frame draws on top, last - its hole is transparent, so the
	// red fill above shows through it, while the green frame/heart icon
	// covers the fill's edges everywhere outside the hole.
	if (frameImg != (unsigned int)-1) {
		iShowImage(barX, barY, barWidth, HEALTHBAR_HEIGHT, frameImg);
	}
}

// --- grenade-count readout, directly below the health bar ---
// Shows one GrenadeIcon.png per grenade the hero has left (see
// Header/Grenade.hpp - Grenade::grenadesRemaining, decremented every time
// TAB throws one - Grenade::startThrow(), called from Playercontroller.hpp).
// Uses the exact same icon texture Grenade::loadAssets() already loaded for
// the flying grenade sprite - no separate load call needed here.
const int GRENADE_HUD_ICON_WIDTH = 28;
const int GRENADE_HUD_ICON_HEIGHT = 40;
const int GRENADE_HUD_ICON_GAP = 8;
const int GRENADE_HUD_MARGIN_Y = 10; // gap between the health bar's bottom edge and these icons

inline void drawGrenadeHUD(Hero& hero) {
	int barY = SCREEN_HEIGHT - HEALTHBAR_HEIGHT - HEALTHBAR_MARGIN_Y; // same math as drawHealthBar()'s barY
	int iconY = barY - GRENADE_HUD_MARGIN_Y - GRENADE_HUD_ICON_HEIGHT;

	unsigned int icon = hero.weapon.grenade.iconImg;
	if (icon == (unsigned int)-1) return;

	for (int i = 0; i < hero.weapon.grenade.grenadesRemaining; i++) {
		int iconX = HEALTHBAR_MARGIN_X + i * (GRENADE_HUD_ICON_WIDTH + GRENADE_HUD_ICON_GAP);
		iShowImage(iconX, iconY, GRENADE_HUD_ICON_WIDTH, GRENADE_HUD_ICON_HEIGHT, icon);
	}
}

#endif