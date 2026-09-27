#ifndef HERO_HPP
#define HERO_HPP

#include "Utility.hpp"
#include "WeaponControl.hpp"

// jump physics - tune these to taste. JUMP_SPEED needs to be roughly
// sqrt(2 * GRAVITY * height) to be able to reach a platform that high.
const int GRAVITY = 1;       // px/tick^2, pulls the hero back down
const int JUMP_SPEED = 20;   // px/tick, upward speed when the jump starts

// How long the attack animation takes, and which tick lands the hit, are
// now per-weapon (see WeaponControl::attackTicks()/attackHitTick() -
// KNIFE_ATTACK_TICKS/PISTOL_ATTACK_TICKS in Knife.hpp/Pistol.hpp) instead of
// one shared pace for every weapon - that's what let the pistol get its own
// faster firing animation.
const int ATTACK_RANGE_X = 70;
const int ATTACK_RANGE_Y = 40;

// Hero's max HP - Health.hpp reads this (alongside Hero::currentHP) to draw
// the health bar, and enemy.h's updateEnemies() calls hero.takeDamage() to
// reduce it whenever an enemy's attack lands.
//
// TUNING HISTORY
//   500 -> far too much; the hero could face-tank the mini-boss five times
//          over and positioning stopped mattering at all.
//   180 -> the opposite problem once the full enemy roster was in: the
//          heavy tiers deleted the bar faster than the level could be read.
//   230 -> base HP (HERO_BASE_HP). Raised together with the ~25% cut to the
//          enemy damage matrix in enemy_properties.h, so the two changes
//          compound into a real survivability increase without touching
//          enemy HP, speed or spawn counts. Every tier still matters,
//          mistakes are still punished, but a single bad corner no longer
//          ends the run.
//   230 -> was the previous base value.
//   300 -> CURRENT base HP (HERO_BASE_HP). Raised again so the Level 3 wave
//          and the two-life Thanos fight are survivable without needing the
//          Space Stone boost first. Nothing else had to change: every
//          "percent of max" read in the game goes through heroMaxHP (health
//          bar, kill-heals, score-heals, asphyxiation drain), so they all
//          rescale themselves to the new number automatically.
//   500 -> HERO_STONE_HP. NOT the starting value (that was the mistake
//          above) - it is a reward: the moment the Level 2 side quest hands
//          over the Space Stone, applySpaceStoneHPBoost() below lifts the
//          max to 500 and Health.hpp swaps in the bigger healthBar2 frame.
//          UNCHANGED by the 230 -> 300 base bump above - the Level 3 "500 HP
//          once the stone is held" logic is exactly as it was.
//
// Also worth knowing: GameFlow.hpp hands back HEAL_PER_MILESTONE HP every
// SCORE_HEAL_INTERVAL points, so a bigger bar makes those heals feel like a
// smaller-but-steadier top-up rather than a full reset.
const int HERO_BASE_HP = 300;
const int HERO_STONE_HP = 500;

// =====================================================================
// LOW-HP ADRENALINE SURGE (Level 3 only)
//
// Once the hero drops below HERO_LOW_HP_FRACTION of his current max HP in
// the Titan arena, every kind of movement he has gets HERO_LOW_HP_SPEED_MULT
// faster - walking AND the jump impulse. It is a last-stand mechanic: the
// closer he is to dying, the harder he is to corner.
//
// The multiplier is applied through Hero::speedScale below rather than by
// overwriting hero.speed, so nothing has to "undo" the boost when he heals
// back up - Playercontroller.hpp just recomputes the scale every tick and it
// falls back to 1.0 on its own. Levels 1 and 2 never set it, so their
// movement is bit-for-bit what it always was.
// =====================================================================
const float HERO_LOW_HP_FRACTION = 0.40f;   // below 40% of max HP
const float HERO_LOW_HP_SPEED_MULT = 1.30f;   // +30% movement

// The hero's CURRENT max HP. This replaced the old `const int HP` - every
// "cap at max" / "percent of max" read (heal(), healthPercent(), the health
// bar, kill-heals, score-heals, the side quest respawn) now goes through
// this so the Space Stone boost reaches all of them at once.
// Reset to HERO_BASE_HP by resetGameState() in level.hpp on every new run.
__declspec(selectany) int heroMaxHP = HERO_BASE_HP;

struct Hero {
	int posX = 5;
	int posY = 176;
	int speed = 5;

	const int HEIGHT = 70;
	const int WIDTH = (int)(HEIGHT * 0.938);

	// Current HP, starts full. Kept separate from heroMaxHP above so
	// "max health" and "health right now" are two different numbers - see
	// takeDamage()/healthPercent() below.
	int currentHP = HERO_BASE_HP;

	// Animation frames + damage now live per-weapon (Knife.hpp/Pistol.hpp)
	// behind this one WeaponControl - Hero just asks it for whichever
	// image the equipped weapon needs. See WeaponControl.hpp to switch
	// weapons ("grab" a different one).
	WeaponControl weapon;

	int runIndx = 0;
	int atkIndx = 0;

	int facing = 1; // 1 = right, -1 = left

	bool isGrounded = true;
	int velocityY = 0;

	bool isMoving = false; // true only on ticks a direction key is actually held

	bool isAttacking = false;
	int atkTimer = 0;
	bool hasHit = false;

	int animTick = 0; // frame-skip counter for the running animation

	// Movement multiplier - 1.0 normally, HERO_LOW_HP_SPEED_MULT while the
	// Level 3 adrenaline surge is up (see the constants above).
	// Playercontroller.hpp recomputes this every tick.
	float speedScale = 1.0f;

	// posX is an int, but speed * speedScale usually isn't (5 * 1.3 = 6.5).
	// Truncating every tick would quietly turn a +30% boost into +20%, and
	// rounding up would make it +40%, so the leftover fraction is carried
	// over to the next tick instead. Over any run of ticks the hero covers
	// exactly speed * speedScale px each, which is what makes "30% faster"
	// actually mean 30%.
	float moveCarry = 0.0f;

	Hero() {}

	// One place that decides how many whole pixels this tick's step is
	// worth. At speedScale 1.0 this returns exactly `speed` every time
	// (carry stays 0.0), so normal movement is unchanged down to the pixel.
	int stepPixels() {
		if (speedScale == 1.0f) return speed;

		moveCarry += (float)speed * speedScale;
		int step = (int)moveCarry;
		moveCarry -= (float)step;
		return step;
	}

	// The jump impulse for this tick, scaled the same way. Used by
	// startJump() below and by Level 2's own gravity-aware jump.
	int jumpImpulse() {
		return (int)((float)JUMP_SPEED * speedScale + 0.5f);
	}

	void loadAssets() {
		weapon.loadAssets();
	}

	// moves left (dir = -1) or right (dir = +1) at a fixed speed, keeps the
	// hero within [0, maxX] (whatever coordinate space the caller uses -
	// e.g. the world width, so he doesn't walk off the edge of the level),
	// updates which way he's facing, and advances the running animation
	// (only while grounded - no run-cycling mid-air).
	//
	// Only call this on ticks a movement key is actually held - it's what
	// sets isMoving true, which is how currentImage() knows to show the
	// running cycle instead of standing still.
	void moveHorizontal(int dir, int maxX) {
		if (dir == 0) return;
		if (isAttacking && weapon.isMelee()) return; // knife swing still locks movement; pistol doesn't - can run and fire at once

		isMoving = true;
		posX += dir * stepPixels();
		facing = dir;

		if (posX < 0) posX = 0;
		if (posX + WIDTH > maxX) posX = maxX - WIDTH;

		if (isGrounded) {
			animTick++;
			if (animTick >= 6) {
				animTick = 0;
				runIndx = (runIndx + 1) % 4;
			}
		}
	}

	// call once on any tick where no movement key is held - stops the run
	// cycle and drops him back to a standing frame instead of freezing
	// mid-stride (which is what made the animation look like it never
	// stopped running even with no key pressed).
	void stopMoving() {
		isMoving = false;
		runIndx = 0;
		animTick = 0;
		moveCarry = 0.0f;   // start the next burst of movement from a clean fraction
	}

	// launches the hero straight up. Only works while grounded. A knife
	// swing still locks him in place (needs footing, no jumping mid-slash);
	// firing the pistol doesn't - he can jump while shooting or shoot while
	// already airborne.
	void startJump() {
		if (!isGrounded) return;
		if (isAttacking && weapon.isMelee()) return;

		isGrounded = false;
		velocityY = jumpImpulse();
	}

	// NOTE: landing/gravity and "is he still standing on something" are
	// no longer decided in here - they depend on levelGrid (which block
	// tiles are 0 vs solid), so that logic lives in level.hpp as
	// heroIsSupported() / heroUpdateGravity(). This keeps Hero.hpp only
	// knowing about the hero himself, not the level layout.

	// plays the knife slash / pistol shot. Blocked while the pistol is on
	// its post-6-shots cooldown (weapon.canFire()) - the knife has no such
	// limit, so it's unaffected. Melee (knife) still needs solid footing
	// (can't start a slash mid-air); the pistol can be fired while running
	// or jumping - see moveHorizontal()/startJump() above, which likewise
	// only lock for melee.
	//
	// Returns true the one tick an attack actually starts, so the caller
	// (Playercontroller.hpp) knows a pistol shot needs to fire right now -
	// before the firing animation even plays its first frame.
	bool startAttack() {
		if (isAttacking) return false;
		if (weapon.isMelee() && !isGrounded) return false;
		if (!weapon.canFire()) return false;

		isAttacking = true;
		atkTimer = 0;
		atkIndx = 0;
		hasHit = false;
		return true;
	}

	// call every tick while isAttacking is true.
	// returns true on the one tick the attack actually lands
	bool updateAttack() {
		atkTimer++;

		int frames = weapon.atkFrameCount();   // 6 for the knife, 4 for the pistol
		int ticks = weapon.attackTicks();     // how long this weapon's animation takes
		atkIndx = (atkTimer * frames) / ticks;
		if (atkIndx > frames - 1) atkIndx = frames - 1;

		bool hitsNow = false;
		if (!hasHit && atkTimer >= weapon.attackHitTick()) {
			hasHit = true;
			hitsNow = true;
		}

		if (atkTimer >= ticks)
			isAttacking = false;

		return hitsNow;
	}

	// the slash's hitbox for this instant, in front of the hero
	void attackHitbox(int& x1, int& y1, int& x2, int& y2) {
		int centerY = posY + HEIGHT / 2;

		if (facing >= 0) {
			x1 = posX + WIDTH;
			x2 = posX + WIDTH + ATTACK_RANGE_X;
		}
		else {
			x1 = posX - ATTACK_RANGE_X;
			x2 = posX;
		}

		y1 = centerY - ATTACK_RANGE_Y;
		y2 = centerY + ATTACK_RANGE_Y;
	}

	// same idea as attackHitbox() above, but sized for the flamethrower's
	// longer reach (FLAME_RANGE_X/Y - see Header/FlameThrower.hpp) instead
	// of the knife's melee range.
	void flameHitbox(int& x1, int& y1, int& x2, int& y2) {
		int centerY = posY + HEIGHT / 2;

		if (facing >= 0) {
			x1 = posX + WIDTH;
			x2 = posX + WIDTH + FLAME_RANGE_X;
		}
		else {
			x1 = posX - FLAME_RANGE_X;
			x2 = posX;
		}

		y1 = centerY - FLAME_RANGE_Y;
		y2 = centerY + FLAME_RANGE_Y;
	}

	// called by an enemy's landed attack (see updateEnemies() in enemy.h).
	// Clamped at 0 so currentHP never goes negative - a negative value
	// would make healthPercent() below go negative too, which would draw
	// the health bar's fill rectangle with a negative width.
	void takeDamage(int dmg) {
		currentHP -= dmg;
		if (currentHP < 0) currentHP = 0;
	}

	// Restores HP, capped at heroMaxHP so a medkit can never push the
	// bar past full. Added for the Puzzle 1 reward (Circuit Rerouting drops
	// a Medkit) - see completePuzzle() in Puzzlecommon.hpp.
	void heal(int amount) {
		currentHP += amount;
		if (currentHP > heroMaxHP) currentHP = heroMaxHP;
	}

	bool isDead() {
		return currentHP <= 0;
	}

	// 0.0 (dead) .. 1.0 (full health) - Health.hpp scales the health bar's
	// fill rectangle's width by this every frame.
	float healthPercent() {
		return (float)currentHP / (float)heroMaxHP;
	}

	// which picture to show right now
	int currentImage() {
		// grenade windup (see Header/Grenade.hpp) wins over EVERYTHING else,
		// even mid weapon-attack pose - it's a separate action bound to TAB
		// (Playercontroller.hpp) that works no matter what's equipped, so it
		// has to be able to override whatever atkImg()/jumpImg()/runImg()
		// would otherwise show.
		if (weapon.grenade.isThrowing) return weapon.grenade.throwImg(facing, weapon.grenade.throwFrame);
		if (isAttacking) return weapon.atkImg(facing, atkIndx); // fire/slash pose wins even mid-air, so a jump-shot actually reads as a shot
		if (!isGrounded) return weapon.jumpImg(facing, 2);      // stick to frame 3 while airborne
		if (isMoving) return weapon.runImg(facing, runIndx);
		return weapon.runImg(facing, 0); // idle - hold a still standing frame, don't keep cycling
	}
};

// Level 2 side quest reward: securing the Space Stone raises max HP to
// HERO_STONE_HP and tops the hero up to it. Health.hpp watches heroMaxHP and
// swaps healthBar.png for healthBar2.png once it is above HERO_BASE_HP.
inline void applySpaceStoneHPBoost(Hero& h) {
	heroMaxHP = HERO_STONE_HP;
	h.currentHP = heroMaxHP;
}

inline void resetHeroMaxHP() {
	heroMaxHP = HERO_BASE_HP;
}


#endif