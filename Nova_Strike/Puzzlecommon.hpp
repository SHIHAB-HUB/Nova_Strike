// ===== Puzzlecommon.hpp =====
#ifndef PUZZLECOMMON_HPP
#define PUZZLECOMMON_HPP

#include "Variables.h"
#include "Header\Hero.hpp"
#include "level.hpp"

// Puzzle khola thakle health emnitei katbe, fast solver er jonno bonus
#define PUZZLE_DRAIN_INTERVAL_TICKS 20
#define PUZZLE_DRAIN_AMOUNT 2

// Puzzle 1 solve korle HP restore hobe
#define MEDKIT_HEAL_AMOUNT 150

// FixedUpdate theke split ticks, puzzle e time spent hole direct HP reduce hoy
inline void updatePuzzleHealthDrain() {
	static int tickCounter = 0;
	tickCounter++;
	if (tickCounter >= PUZZLE_DRAIN_INTERVAL_TICKS) {
		tickCounter = 0;
		hero.takeDamage(PUZZLE_DRAIN_AMOUNT);
	}
}

// ---- Puzzle rewards -------------------------------------------------------
// One function per reward package, so every puzzle site reads as exactly
// what it hands over. Every one of them also drops the player back into
// gameplay, and a reward is only ever granted from a SOLVED puzzle - quitting
// with ESC never calls these.
//
//   Level 1 / Puzzle 1 (Circuit)    -> grantPistolAndKey()
//   Level 1 / Puzzle 2 (Waveform)   -> grantKeyAndHealth()
//   Level 2 / Door 1 (Asteroids)    -> grantAK47AndKey()
//   Level 2 / Door 2 (Weather Node) -> grantFlamethrowerAndKey()
//
// The Plasma Rifle has no puzzle of its own - it unlocks once the Space
// Stone is secured (hasSpaceStone, set at the end of the Level 2 side
// quest - see SideQuest.hpp), checked directly at the key-'5' handler in
// Playercontroller.hpp rather than through a grant*() function here.

// Toast shown in the HUD right after a reward lands.
__declspec(selectany) int rewardToastTicks = 0;
__declspec(selectany) const char* rewardToastText = "";

// Optional SECOND line, drawn in its own box directly under the main reward
// toast (Playercontroller.hpp). Used by the Space Stone: "PLASMA RIFLE
// UNLOCKED" on top, "HEALTH INCREASED ..." below it. Any normal
// showRewardToast() clears it, so it can never linger under an unrelated
// reward.
__declspec(selectany) int rewardToast2Ticks = 0;
__declspec(selectany) const char* rewardToast2Text = "";

inline void showRewardToast(const char* text) {
	rewardToastText = text;
	rewardToastTicks = 110; // ~3.3s
	rewardToast2Ticks = 0;
}

inline void showRewardToast2(const char* text) {
	rewardToast2Text = text;
	rewardToast2Ticks = 110; // same length as the main toast, expires with it
}

inline void grantPistolAndKey() {
	hasPistolPickup = true;
	keysCollected++;
	showRewardToast("PISTOL UNLOCKED  [2]   +1 KEY");
	currentPage = PAGE_PLAYING;
}

inline void grantKeyAndHealth() {
	keysCollected++;
	hasMedkitPickup = true;
	hero.heal(MEDKIT_HEAL_AMOUNT);
	showRewardToast("+1 KEY   MEDKIT: HEALTH RESTORED");
	currentPage = PAGE_PLAYING;
}

inline void grantAK47AndKey() {
	hasAK47Pickup = true;
	keysCollected++;
	showRewardToast("AK47 UNLOCKED  [3]   +1 KEY");
	currentPage = PAGE_PLAYING;
}

// Level 2 / Door 2 (Weather Node, Puzzle4.hpp) - the flamethrower's source.
// It used to be auto-issued on stepping into the Level 2 side quest door
// (see the "issued on entry" comment that used to sit in enterSideQuest(),
// SideQuest.hpp); it is now gated behind actually solving this puzzle
// instead, same as every other weapon's pickup.
inline void grantFlamethrowerAndKey() {
	hasFlamethrowerPickup = true;
	keysCollected++;
	showRewardToast("FLAMETHROWER UNLOCKED  [4]   +1 KEY");
	currentPage = PAGE_PLAYING;
}

// Kept so any older call site still compiles: true = key+health,
// false = AK47+key.
inline void completePuzzle(bool grantsKeyAndMedkit) {
	if (grantsKeyAndMedkit) grantKeyAndHealth();
	else                    grantAK47AndKey();
}

#endif