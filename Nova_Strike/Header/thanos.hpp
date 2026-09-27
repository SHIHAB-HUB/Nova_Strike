// ===== Header/thanos.hpp =====
#ifndef THANOS_HPP
#define THANOS_HPP

// Owner: [Shihabul Islam Shihab]
//
// Level 3's boss. Deliberately its own self-contained file/struct instead of
// one more row in enemy_properties.h's ENEMY_TYPE_COUNT arrays - Thanos only
// ever exists once, only in Level 3 (Header/Level3.hpp), and needs a
// two-phase death/revive sequence (Images\Thanos\dying,
// Images\Thanos\Reviving) the generic Enemy roster in Header/enemy.h has no
// concept of at all - every regular enemy just deactivates the instant its
// health hits 0 (see damageEnemiesInBox() in enemy.h). Giving him his own
// header keeps that out of the shared enemy system instead of bolting a 9th
// special case onto it.
//
// His moveset also changes between the two lives, not just his HP pool:
// first life is sword-only (Images\Thanos\Attack\Sword), close range.
// Second life (post-revive) ADDS a ranged beam (Images\Thanos\Attack\beam) -
// he charges it up and lets it fly whenever the hero is out of sword reach,
// and still falls back on the sword the instant the hero closes the
// distance. See the "close beats far" trigger logic in updateThanos().
//
// His third ability, the POWER STONE, is a meteor strike (art lives in the
// Power Strong folder under Images/Thanos/Attack): he roots himself for a 2
// second summoning clip, having registered the hero's x position the instant
// it began, and then a meteor drops out of the sky onto that exact x. It cannot steer, so the
// hero dodges by simply not being there. See the POWER STONE ATTACK block
// below and updateThanosMeteor().
//
// No dependency on Level3.hpp on purpose (arena width is passed in to
// updateThanos() rather than read from a Level3.hpp macro) - this file gets
// #included from level.hpp itself (see the "Dependent header includes"
// block there), which comes BEFORE Level3.hpp in every translation unit
// that pulls both in, so it can't reach back into Level3-only constants.
// Same reason drawThanosHPBar() below uses plain iSetColor/iFilledRectangle
// instead of level.hpp's fxFilledRectA() - that helper is defined further
// down in level.hpp than this file's #include line, so it isn't visible
// here yet.

#include "iGraphics.h"
#include "Hero.hpp"
#include <cstdio>
#include <ctime>
#include <cstring>   // strlen() - centering the HP bar's numeric label
#include <cmath>     // floorf() - pixel rounding in drawThanosFrame()

// --- COLLISION BOX ---------------------------------------------------
// This is his HITBOX / physics body only - it is NOT the size any sprite
// is drawn at any more (see the "GROUNDED ART" block below). Keeping the
// box a fixed, modest rectangle centered on his feet means a wide sword
// swing doesn't suddenly make him a wider wall for the hero to bump into.
const int THANOS_WIDTH = 140 * 0.7;
const int THANOS_HEIGHT = 220 * 0.7;

// --- GROUNDED, CONSISTENT-SIZE ART -------------------------------------
// The collision box above is his HITBOX only - sprites are NOT stretched to
// fill it. Three separate problems made the dying/reviving art (and, to a
// lesser degree, the sword clip) look wrong before:
//
//  1. Every frame used to be "contain-fit" into a box, i.e. scaled so its
//     whole PNG canvas filled the same height. That gives every frame a
//     DIFFERENT scale: the flat Reviving/1.png (395x182) was blown up ~3.4x
//     more than the standing Reviving/5.png (361x622), so he swelled and
//     shrank from frame to frame instead of just lying down and standing up.
//
//  2. Frames were anchored by the bottom edge of the PNG canvas. But the
//     canvases have transparent rows UNDER the boots (Reviving/5.png has
//     120px, dying/4.png 152px, Running/R_1.png 68px...), so anchoring the
//     canvas to the floor left him floating above the ground. What has to
//     touch the floor is the lowest VISIBLE pixel, so loadAssets() now
//     measures each PNG's real pixel bounds (see measureFrame()).
//
//  3. The art was generated in separate batches at different zoom levels:
//     the running clip is drawn ~1.5x bigger than the dying / reviving /
//     sword clips (his head is ~100px tall in Running/*.png but ~65px in the
//     others). One shared scale for everything therefore can't work either.
//
// THE FIX: every clip gets ONE fixed scale (screen px per source px), used
// for every frame in that clip, so a frame is exactly as big relative to its
// neighbours as the artist drew it. The scales are calibrated so his head is
// the same size on screen in every clip and a standing pose is exactly
// THANOS_HEIGHT tall (the hitbox):
//
//   Reviving/5.png is the upright pose: 445px of visible art -> THANOS_HEIGHT.
//   THANOS_ART_SCALE is that ratio, and is shared by dying / reviving / sword.
//   The running art is ~1.5x zoomed, so it is divided by THANOS_RUN_ART_ZOOM.
//
// If the art is ever regenerated at a different zoom, THESE THREE CONSTANTS
// are the only things to retune - nothing per-frame is hardcoded any more.
const float THANOS_ART_SCALE = (float)THANOS_HEIGHT / 445.0f;
const float THANOS_RUN_ART_ZOOM = 1.5f;
const float THANOS_RUN_SCALE = THANOS_ART_SCALE / THANOS_RUN_ART_ZOOM;

// A pixel counts as "part of Thanos" when its alpha is above this. Low enough
// to keep soft glow edges, high enough to ignore near-invisible fringe.
const int THANOS_ALPHA_THRESHOLD = 16;

// Where the visible art sits inside one PNG canvas. srcW/srcH are the file's
// pixel size; visL/visR are the first/one-past-last columns that contain
// visible pixels; visB is the one-past-last ROW (counted from the top of the
// image) that does - i.e. where his boots/body actually rest, which is the
// line that must sit on the floor.
struct ThanosFrameMeta {
	int srcW = 0, srcH = 0;
	int visL = 0, visR = 0;
	int visB = 0;

	// Fallback when a file can't be measured: treat the whole canvas as visible.
	void setCanvas(int w, int h) { srcW = w; srcH = h; visL = 0; visR = w; visB = h; }
};

inline int thanosRound(float v) { return (int)floorf(v + 0.5f); }


// Two lives. The FIRST time his health hits 0 he doesn't actually die - he
// plays the dying clip, then the reviving clip, and comes back at
// THANOS_REVIVE_MAX_HP. Only the SECOND time his health hits 0 (starting
// from that higher pool) does he go down for good. See damageThanos()/
// updateThanos() below.
const int THANOS_MAX_HP = 5000;          // starting health, first life
const int THANOS_REVIVE_MAX_HP = 10000;   // health after the first revive, second life

// Slower than the hero (Hero::speed, Hero.hpp is 5 px/tick) on purpose -
// he's always closing the distance, but never fast enough to be unavoidable.
// Kept well under half hero speed so he reads as a lumbering, heavy threat
// rather than a real footrace.
const float THANOS_SPEED_PX_PER_TICK = 1.0f;

// --- CONTACT DAMAGE: REMOVED -----------------------------------------
// He used to chip THANOS_CONTACT_DMG off the hero every
// THANOS_CONTACT_COOLDOWN_MS just for being touched. He doesn't any more -
// simply colliding with his body is now completely harmless, and the only
// way he hurts the hero is a deliberate, telegraphed attack (the sword
// swing below). Walking into him just stops the hero, it never damages him
// (see thanosBlocksHero() and heroMoveHorizontalL3() in Level3.hpp).
//
// The constants are kept (unused) so nothing that referenced them breaks.
const int THANOS_CONTACT_DMG = 0;
const long THANOS_CONTACT_COOLDOWN_MS = 700;

// Points awarded (and isLevel3Complete flipped) only on the FINAL kill - the
// second time his health hits 0 (see damageThanos() below), not the first
// (which revives instead of dying). Same "score on kill" idea as
// ENEMY_SCORE_VALUE in enemy_properties.h, just its own constant since
// Thanos isn't part of that array.
const int THANOS_SCORE_VALUE = 10000;   // was 1000. GameFlow.hpp's updateGameFlow() saves this run to highscore.txt the tick isLevel3Complete flips, so the scoreboard picks the bonus up

const int THANOS_RUN_FRAMES = 4;   // R_1..R_4 / L_1..L_4 under Thanos\Running
const int THANOS_RUN_FRAME_MS = 120;

const int THANOS_DYING_FRAMES = 4;      // 1..4.png under Thanos\dying
const int THANOS_DYING_FRAME_MS = 180;  // ms to hold each dying frame

const int THANOS_REVIVING_FRAMES = 5;      // 1..5.png under Thanos\Reviving
const int THANOS_REVIVING_FRAME_MS = 200;  // held a touch longer than a dying frame - a comeback should read as an EVENT

// --- SWORD ATTACK (EITHER LIFE) ---------------------------------------
// Close-range option in BOTH lives (see the "close beats far" trigger
// logic in updateThanos()) - draws the sword: Images\Thanos\Attack\Sword,
// R_1..R_8 while facing right, the L series while facing left (only L_1
// exists on disk, so the remaining left frames come from the R series - see
// THANOS_SWORD_R_FACES_LEFT_FROM below, which is why the swing is drawn
// mirrored on the right and NOT on the left). Second life additionally
// gets the beam for when the hero is out of this reach - see the BEAM
// ATTACK block below.
//
// TIMING: he is completely rooted for the whole swing - no chasing, no
// turning, no re-aiming (see updateThanos()). The clip is deliberately
// long enough that a hero who starts running the instant the swing begins
// is clear of THANOS_SWORD_RANGE well before the blade actually lands:
// 8 frames x 115ms = 920ms of animation, with the hit landing on frame 5
// (~460ms in). The hero moves several times faster than Thanos, so ~460ms
// of warning is more than enough distance to escape - standing still is
// what gets punished, not being nearby.
const int THANOS_SWORD_FRAMES = 8;

// The sword art is NOT consistently oriented: R_1 (and L_1) face the way their
// names say, but R_2..R_8 were drawn facing LEFT - blade, helmet and slash arc
// all point left even though the files are called R_n. Drawing them "as is"
// while facing right therefore swung the blade backwards, and mirroring them
// for the left-facing swing (the old fallback) swung that one backwards too.
//
// This is the 0-based index of the first frame whose R_n file faces left.
// Frames from here on are flipped for a right-facing Thanos and drawn as-is
// for a left-facing one. If the R_n art is ever regenerated facing right, set
// this to THANOS_SWORD_FRAMES (or higher) and nothing else has to change.
const int THANOS_SWORD_R_FACES_LEFT_FROM = 1;
const int THANOS_SWORD_FRAME_MS = 115;
const int THANOS_SWORD_HIT_FRAME = 4;       // 0-based - the 5th frame is the actual blade contact
const int THANOS_SWORD_DMG = 100;

// How far in front of his body the blade reaches, and how close the hero
// has to be for him to decide to start a swing at all. The trigger range
// is the tighter of the two so a swing is never started from a distance
// the blade couldn't reach even if the hero stood perfectly still.
const int THANOS_SWORD_RANGE = 60;         // px in front of the collision box - blade reach at the hit frame
const int THANOS_SWORD_TRIGGER_RANGE = 80; // px - how close the hero must be to make him start swinging

// Minimum gap between the END of one swing and the START of the next, so
// he can't chain swings back to back and lock the hero in place.
const long THANOS_SWORD_COOLDOWN_MS = 1400;

// --- BEAM ATTACK (SECOND LIFE ONLY) -----------------------------------
// The revive's payoff isn't just a bigger HP bar - round two he can also
// zap the hero from range. While the hero is OUTSIDE sword range
// (THANOS_SWORD_TRIGGER_RANGE - see the trigger logic in updateThanos())
// he charges up (Images\Thanos\Attack\beam, R_1..R_8 while facing right,
// L_1..L_8 while facing left - unlike the sword clip, BOTH sides have a
// full 8-frame set on disk, so this one never needs the mirror fallback)
// and then a slow-moving bolt (Beam_R.png / Beam_L.png) leaves him and
// flies in a straight line along the floor.
//
// Same "rooted, telegraphed, escapable" shape as the sword: he cannot
// move or turn mid-charge, and the charge + bolt travel time together give
// the hero plenty of warning - the bolt is slow enough that simply
// JUMPING clears it (see THANOS_BEAM_SPEED_PX_PER_TICK below), the same
// way a standing-still hero eats the sword but a moving one doesn't.
//
// If the hero closes the gap into sword range - whether before a charge
// starts or while a bolt is already in flight - the NEXT decision
// updateThanos() makes (once he's free to act again) is the sword, not
// another beam: see the "close beats far" ordering in updateThanos().
const int THANOS_BEAM_CHARGE_FRAMES = 8;      // R_1..R_8 / L_1..L_8 under Thanos\Attack\beam
const int THANOS_BEAM_CHARGE_FRAME_MS = 90;  // a touch slower than the sword's 115ms - a beam should feel heavier to wind up
const int THANOS_BEAM_DMG = 60;               // less than the sword's 30 - it's the easier-to-dodge, longer-range option
const long THANOS_BEAM_COOLDOWN_MS = 1700;    // gap between one bolt landing/expiring and the next charge being allowed to start

// Deliberately slower than THANOS_SPEED_PX_PER_TICK (his own walk speed)
// AND far slower than Hero::speed - a hero who just stands there gets hit,
// but a single jump (Hero.hpp's JUMP_SPEED/GRAVITY) comfortably clears a
// bolt moving this slowly since the hitbox rides low along the floor
// (see THANOS_BEAM_HEIGHT / spawnThanosBeam()).
const float THANOS_BEAM_SPEED_PX_PER_TICK = 5.0f;

// Hitbox: low and floor-hugging on purpose - tall enough to catch a
// grounded hero, short enough that a jump takes the hero's own hitbox
// clean above it well before the bolt arrives.
const int THANOS_BEAM_WIDTH = 100*0.8;
const int THANOS_BEAM_HEIGHT = 46*0.8;

// Draw size: Beam_R.png/Beam_L.png are 364x164 canvases (~2.22:1) - drawn
// bigger than the hitbox and centered on it, same "bigger sprite, honest
// hitbox" idea as the plasma bolt in Header/Bullet.hpp.
const int THANOS_BEAM_DRAW_WIDTH = 160;
const int THANOS_BEAM_DRAW_HEIGHT = 72;

const int THANOS_MAX_BEAMS = 2; // he only ever winds up one at a time, but a small pool costs nothing and avoids a hard cap on rapid boss phases later

// --- POWER STONE ATTACK (THIRD ABILITY) --------------------------------
// He raises the stone (Images\Thanos\Attack\Power Strong, R_n while facing
// right, L_n while facing left) for THANOS_POWER_STONE_SUMMON_MS. The instant
// that clip STARTS he registers the hero's x position (powerStoneTargetX); when
// the clip ends a meteor (Falling.png) is released from the top of the screen
// and drops straight down onto that registered x, then plays Smassing.png where
// it lands.
//
// The meteor never re-aims - its x is fixed for its whole fall - so the hero
// dodges by walking out of that column during the summon. It falls far too fast
// to react to once it is released; the telegraph IS the 2 second summoning
// clip. He is completely rooted for the whole clip: no chasing, no turning
// (see updateThanos()), same rule as the sword and the beam charge.
//
// Only 7 frames per side exist on disk: _1.._5, _7 and _8 - there is no _6.
// THANOS_POWER_STONE_FILE_NUM maps frame slot -> file number for that reason.
const int THANOS_POWER_STONE_FRAMES = 7;
const int THANOS_POWER_STONE_FILE_NUM[THANOS_POWER_STONE_FRAMES] = { 1, 2, 3, 4, 5, 7, 8 };

// The whole clip is spread evenly over this duration (frame index is derived
// from elapsed time, not a per-frame delay), so retuning it stretches the
// animation with it and it always lasts exactly this long.
const long THANOS_POWER_STONE_SUMMON_MS = 2000;

// Gap between the summoning clip ENDING (meteor released) and the next
// summon being allowed to start. Also applies from the moment he spawns, so
// the fight does not open with a meteor.
const long THANOS_POWER_STONE_COOLDOWN_MS = 10000;

const int THANOS_POWER_STONE_DMG = 200;

// true = only after the revive (second life), like the beam - the first life is
// sword-only. false = he can use it in both lives.
const bool THANOS_POWER_STONE_SECOND_LIFE_ONLY = true;

// The Power Strong art was drawn at roughly half the zoom of the sword/beam/
// reviving art (his helmet is ~35px tall in these PNGs vs ~65px in those - same
// story as THANOS_RUN_ART_ZOOM above, just the other direction), so it gets its
// own scale to keep him the same size on screen as in every other clip.
const float THANOS_POWER_ART_ZOOM = 0.54f;
const float THANOS_POWER_SCALE = THANOS_ART_SCALE / THANOS_POWER_ART_ZOOM;

// Meteor. Speed is in px per SECOND and applied against the real clock (not
// per tick like the beam), so "too fast to react to" holds no matter what
// frame rate the draw loop is running at. 1800 px/s crosses the 720px screen
// in well under half a second.
const float THANOS_METEOR_SPEED_PX_PER_SEC = 3240.0f;

// Hitbox: a column centered on the registered x. Narrower than the drawn
// sprite (~150px visible) so a graze on the edge of the trails is not a hit.
const int THANOS_METEOR_HIT_WIDTH = 120;
const int THANOS_METEOR_HIT_HEIGHT = 240;   // how far above its leading edge the meteor is solid

const float THANOS_METEOR_DRAW_SCALE = 0.8f;    // screen px per source px for Falling.png and Smassing.png
const long THANOS_METEOR_SMASH_MS = 500;        // how long the impact image stays up
const float THANOS_METEOR_MAX_DT_SEC = 0.1f;    // clamp for one tick's time step, so a hitch cannot teleport the meteor

// --- ENRAGE (SECOND LIFE, HALF HEALTH) -----------------------------------
// Once he has revived AND is knocked down to half of the second-life pool
// (THANOS_REVIVE_MAX_HP), he speeds up by THANOS_ENRAGE_SPEED_MULT for the rest
// of the fight. "Speed" here means how fast he MOVES and ACTS: walking speed,
// the run cycle, the sword swing, the beam charge-up and the power stone
// summon all play that much faster (durations divided by the multiplier).
// Deliberately NOT sped up: cooldowns between attacks, the beam bolt's flight
// speed, the meteor's fall speed and every damage number - so the hero still
// gets the same windows to recover and the same dodge rules, just less time
// to react to each telegraph.
//
// An attack already in progress finishes at the pace it started (see
// ThanosBoss::actionSpeedMult) so crossing the threshold mid-swing can't make
// an animation jump; the new speed applies from his next action.
const float THANOS_ENRAGE_HP_FRACTION = 0.5f;
const float THANOS_ENRAGE_SPEED_MULT = 1.3f;

// ALIVE       - chasing/standing, taking hits, running animation. Also
//               where the sword-vs-beam-vs-chase decision is made every
//               tick (see the trigger logic in updateThanos()).
// ATTACK      - the one-shot sword clip is playing. He cannot move, cannot
//               turn, and cannot start another swing until it finishes and
//               THANOS_SWORD_COOLDOWN_MS has elapsed. Available in EITHER
//               life, whenever the hero is close enough.
// BEAM_CHARGE - the one-shot beam charge-up clip is playing. Same rooted/
//               no-retarget rule as ATTACK. Only reachable in his SECOND
//               life (livesLeft == 0) and only while the hero is out of
//               sword range - see updateThanos(). Releases a slow bolt
//               (spawnThanosBeam()) the instant the clip finishes.
// POWER_STONE - the one-shot summoning clip is playing (THANOS_POWER_STONE_SUMMON_MS).
//               Same rooted/no-retarget rule as ATTACK. The hero's x is registered
//               when it starts; the meteor is released when it ends. Only reachable
//               in his SECOND life (THANOS_POWER_STONE_SECOND_LIFE_ONLY), at any
//               range, whenever the 10 second cooldown has expired.
// DYING       - health hit 0 this tick - the one-shot dying clip is playing.
//               Where it leads depends on livesLeft (see updateThanos()):
//               still has a life left -> REVIVING; out of lives -> DEAD.
// REVIVING    - the one-shot Reviving clip is playing. Only reachable after a
//               DYING clip that wasn't his last life. Ends back at ALIVE with
//               hp reset to THANOS_REVIVE_MAX_HP.
// DEAD        - the final dying clip finished - thanos.active goes false and
//               he's gone for good, same end state the regular enemy roster
//               reaches immediately (see killEnemy() in enemy.h), just delayed
//               until the animation actually finishes playing here.
enum ThanosState { THANOS_ALIVE = 0, THANOS_DYING = 1, THANOS_REVIVING = 2, THANOS_DEAD = 3, THANOS_ATTACK = 4, THANOS_BEAM_CHARGE = 5, THANOS_POWER_STONE = 6 };

struct ThanosBoss {
	bool active = false;       // spawned yet? (see spawnThanos())
	ThanosState state = THANOS_ALIVE;

	int x = 0, y = 0;
	float xPos = 0.0f;
	int direction = -1;        // -1 = facing/moving left, +1 = right

	int hp = THANOS_MAX_HP;
	int maxHp = THANOS_MAX_HP; // current life's HP pool - THANOS_MAX_HP first life, THANOS_REVIVE_MAX_HP second (drives the HP bar's fraction)

	// How many revives are still owed to him. Starts at 1 (one revive
	// available) - hits 0 the first time he dies, and it's THAT transition
	// (livesLeft going from 1 to 0) that sends him to REVIVING instead of
	// DEAD. The second death, with livesLeft already 0, goes straight to DEAD.
	// It also doubles as the "which life is he on" flag the BEAM gates off:
	// livesLeft == 0 means second life (post-revive), which is the only life
	// he has the beam in - the sword, unlike before, now works in both.
	int livesLeft = 1;

	int runFrame = 0;
	long lastRunFrameMs = 0;

	int dyingFrame = 0;
	long lastDyingFrameMs = 0;

	int revivingFrame = 0;
	long lastRevivingFrameMs = 0;

	// --- sword swing state ---
	int swordFrame = 0;
	long lastSwordFrameMs = 0;
	long lastSwordEndMs = 0;   // when the last swing FINISHED - the cooldown counts from here
	bool swordHitLanded = false; // one hit per swing, same idea as enemy.h's hasHitHero

	long lastContactHitMs = 0; // legacy - contact damage is gone, kept so nothing referencing it breaks

	// --- beam charge state (second life only) --------------------------
	int beamChargeFrame = 0;
	long lastBeamChargeFrameMs = 0;
	long lastBeamEndMs = 0;   // when the last charge FINISHED (bolt released) - the cooldown counts from here, same idea as lastSwordEndMs

	// --- enrage (see THANOS_ENRAGE_* above) ----------------------------
	bool enraged = false;
	// Speed multiplier captured when the CURRENT attack started (1.0 normally,
	// THANOS_ENRAGE_SPEED_MULT once enraged). Sword/beam/power stone timing all
	// read this rather than the live flag so an attack never changes pace
	// halfway through its own clip.
	float actionSpeedMult = 1.0f;

	// --- power stone summon state --------------------------------------
	long powerStoneStartMs = 0;
	int powerStoneFrame = 0;
	int powerStoneTargetX = 0;     // hero's CENTER x, registered the instant the summon starts - the meteor lands here
	long lastPowerStoneEndMs = 0;  // when the last summon FINISHED (meteor released) - the cooldown counts from here

	int runImgR[THANOS_RUN_FRAMES];
	int runImgL[THANOS_RUN_FRAMES];

	int dyingImg[THANOS_DYING_FRAMES];
	int revivingImg[THANOS_REVIVING_FRAMES];

	int swordImgR[THANOS_SWORD_FRAMES];
	int swordImgL[THANOS_SWORD_FRAMES];
	// Per-frame "draw this image flipped" flags, one per facing. See
	// THANOS_SWORD_R_FACES_LEFT_FROM for why they are not simply
	// R = as-is / L = mirrored. swordMirrorR is set for R_n files whose art
	// really faces left; swordMirrorL is set for a left-facing frame that has
	// no real L_n on disk and is covered by the matching R_n instead (and only
	// when that R_n faces right). drawThanos() flips accordingly.
	bool swordMirrorR[THANOS_SWORD_FRAMES];
	bool swordMirrorL[THANOS_SWORD_FRAMES];

	// Beam charge-up clip. Unlike the sword, BOTH R_1..R_8 and L_1..L_8
	// exist for real on disk (see loadAssets()), so there is no mirror
	// fallback array here - every left frame is genuine left-facing art.
	int beamChargeImgR[THANOS_BEAM_CHARGE_FRAMES];
	int beamChargeImgL[THANOS_BEAM_CHARGE_FRAMES];

	// The bolt itself is a single sprite per side (Beam_R.png/Beam_L.png),
	// not a per-frame clip - it doesn't need a ThanosFrameMeta because it's
	// drawn at a fixed size (THANOS_BEAM_DRAW_WIDTH/HEIGHT) centered on its
	// hitbox, the same "bigger sprite, honest hitbox" idiom the plasma
	// bolt uses in Header/Bullet.hpp, rather than grounded/foot-anchored
	// like every other Thanos clip.
	int beamProjImgR = -1;
	int beamProjImgL = -1;

	// Power stone summoning clip - BOTH sides are real art on disk, so no
	// mirror fallback (same as the beam charge).
	int powerStoneImgR[THANOS_POWER_STONE_FRAMES];
	int powerStoneImgL[THANOS_POWER_STONE_FRAMES];

	// The meteor and its impact are single images, drawn by
	// drawThanosMeteor() with the same visible-bounds anchoring as the clips.
	int meteorFallImg = -1;
	int meteorSmashImg = -1;

	// --- per-frame image metrics -------------------------------------
	// iGraphics' iLoadImage() hands back a bare GL texture id and gives no
	// way to ask how big the file was or where the art sits in it, so
	// loadAssets() measures each PNG itself (measureFrame() below) and keeps
	// the results here. drawThanosFrame() needs them to scale a frame and to
	// put its feet - not its canvas edge - on the floor.
	ThanosFrameMeta runMetaR[THANOS_RUN_FRAMES], runMetaL[THANOS_RUN_FRAMES];
	ThanosFrameMeta dyingMeta[THANOS_DYING_FRAMES];
	ThanosFrameMeta revivingMeta[THANOS_REVIVING_FRAMES];
	ThanosFrameMeta swordMetaR[THANOS_SWORD_FRAMES], swordMetaL[THANOS_SWORD_FRAMES];
	ThanosFrameMeta beamChargeMetaR[THANOS_BEAM_CHARGE_FRAMES], beamChargeMetaL[THANOS_BEAM_CHARGE_FRAMES];
	ThanosFrameMeta powerStoneMetaR[THANOS_POWER_STONE_FRAMES], powerStoneMetaL[THANOS_POWER_STONE_FRAMES];
	ThanosFrameMeta meteorFallMeta, meteorSmashMeta;

	// Defensive default - if loadAssets() below is ever skipped (as it was
	// until this bug was found), these images stay -1 instead of holding
	// uninitialized garbage, so drawThanos()'s "if (img != -1)" fallback
	// draws the placeholder rectangle instead of an invalid texture handle
	// (which is what silently rendered nothing before).
	ThanosBoss() {
		for (int i = 0; i < THANOS_RUN_FRAMES; i++) { runImgR[i] = -1; runImgL[i] = -1; }
		for (int i = 0; i < THANOS_DYING_FRAMES; i++) dyingImg[i] = -1;
		for (int i = 0; i < THANOS_REVIVING_FRAMES; i++) revivingImg[i] = -1;
		for (int i = 0; i < THANOS_SWORD_FRAMES; i++) { swordImgR[i] = -1; swordImgL[i] = -1; swordMirrorR[i] = false; swordMirrorL[i] = false; }
		for (int i = 0; i < THANOS_BEAM_CHARGE_FRAMES; i++) { beamChargeImgR[i] = -1; beamChargeImgL[i] = -1; }
		for (int i = 0; i < THANOS_POWER_STONE_FRAMES; i++) { powerStoneImgR[i] = -1; powerStoneImgL[i] = -1; }

		// Canvas sizes of the PNGs as a fallback only. loadAssets() overwrites
		// these with the measured canvas AND visible bounds; if a file can't be
		// decoded the frame degrades to "whole canvas visible" instead of
		// breaking.
		const int rw[THANOS_RUN_FRAMES] = { 534, 420, 498, 555 };
		const int rh[THANOS_RUN_FRAMES] = { 684, 590, 641, 625 };
		for (int i = 0; i < THANOS_RUN_FRAMES; i++) { runMetaR[i].setCanvas(rw[i], rh[i]); runMetaL[i].setCanvas(rw[i], rh[i]); }

		const int dw[THANOS_DYING_FRAMES] = { 386, 418, 419, 482 };
		const int dh[THANOS_DYING_FRAMES] = { 524, 494, 428, 429 };
		for (int i = 0; i < THANOS_DYING_FRAMES; i++) dyingMeta[i].setCanvas(dw[i], dh[i]);

		const int vw[THANOS_REVIVING_FRAMES] = { 395, 356, 358, 335, 361 };
		const int vh[THANOS_REVIVING_FRAMES] = { 182, 272, 407, 384, 622 };
		for (int i = 0; i < THANOS_REVIVING_FRAMES; i++) revivingMeta[i].setCanvas(vw[i], vh[i]);

		const int sw[THANOS_SWORD_FRAMES] = { 400, 348, 471, 507, 454, 332, 439, 298 };
		const int sh[THANOS_SWORD_FRAMES] = { 430, 493, 358, 402, 245, 265, 356, 301 };
		for (int i = 0; i < THANOS_SWORD_FRAMES; i++) { swordMetaR[i].setCanvas(sw[i], sh[i]); swordMetaL[i].setCanvas(sw[i], sh[i]); }

		// R_n and L_n are separate art (not mirrors of each other) but happen
		// to share the same canvas dimensions per frame, same as the sword's
		// fallback above - measureFrame() in loadAssets() overwrites these
		// with each file's real measured bounds regardless.
		const int bw[THANOS_BEAM_CHARGE_FRAMES] = { 256, 268, 314, 308, 358, 450, 419, 531 };
		const int bh[THANOS_BEAM_CHARGE_FRAMES] = { 294, 295, 300, 281, 337, 284, 249, 299 };
		for (int i = 0; i < THANOS_BEAM_CHARGE_FRAMES; i++) { beamChargeMetaR[i].setCanvas(bw[i], bh[i]); beamChargeMetaL[i].setCanvas(bw[i], bh[i]); }

		// Power Strong: R_n and L_n share canvas sizes per frame, in slot order
		// (files 1,2,3,4,5,7,8). Falling.png / Smassing.png are single images.
		const int pw[THANOS_POWER_STONE_FRAMES] = { 161, 209, 225, 237, 217, 249, 303 };
		const int ph[THANOS_POWER_STONE_FRAMES] = { 252, 245, 269, 304, 314, 318, 379 };
		for (int i = 0; i < THANOS_POWER_STONE_FRAMES; i++) { powerStoneMetaR[i].setCanvas(pw[i], ph[i]); powerStoneMetaL[i].setCanvas(pw[i], ph[i]); }
		meteorFallMeta.setCanvas(254, 338);
		meteorSmashMeta.setCanvas(297, 225);
	}

	// Same file-probe idiom Level3.hpp's l3SafeLoad() uses: iLoadImage()
	// returns a "valid looking" texture id even for a file that isn't
	// there, so probing first is the only way the "!= -1" fallbacks in
	// drawThanos() mean anything.
	static int probeLoad(const char* path) {
		FILE* probe = NULL;
		fopen_s(&probe, path, "rb");
		if (!probe) return -1;
		fclose(probe);
		return iLoadImage((char*)path);
	}

	// Decodes the PNG once and records its canvas size plus the bounding
	// columns / bottom row of everything above THANOS_ALPHA_THRESHOLD.
	// Returns false (leaving m untouched) if the file can't be decoded or is
	// completely transparent. Runs once per file inside loadAssets(), which is
	// itself one-time (see initLevel3Assets() in Level3.hpp).
	static bool measureFrame(const char* path, ThanosFrameMeta& m) {
		int w = 0, h = 0, n = 0;
		unsigned char* data = stbi_load(path, &w, &h, &n, 4);
		if (!data) return false;

		int minX = w, maxX = -1, maxY = -1;
		for (int y = 0; y < h; y++) {
			for (int x = 0; x < w; x++) {
				if (data[(y * w + x) * 4 + 3] > THANOS_ALPHA_THRESHOLD) {
					if (x < minX) minX = x;
					if (x > maxX) maxX = x;
					if (y > maxY) maxY = y;
				}
			}
		}
		stbi_image_free(data);

		if (maxX < 0) return false;
		m.srcW = w; m.srcH = h;
		m.visL = minX; m.visR = maxX + 1;
		m.visB = maxY + 1;
		return true;
	}

	// probeLoad() + measureFrame() in one step.
	static int loadFrame(const char* path, ThanosFrameMeta& m) {
		int img = probeLoad(path);
		if (img != -1) measureFrame(path, m);
		return img;
	}

	void loadAssets() {
		for (int i = 0; i < THANOS_RUN_FRAMES; i++) {
			char path[128];
			sprintf_s(path, sizeof(path), "Images\\Thanos\\Running\\R_%d.png", i + 1);
			runImgR[i] = loadFrame(path, runMetaR[i]);
			sprintf_s(path, sizeof(path), "Images\\Thanos\\Running\\L_%d.png", i + 1);
			runImgL[i] = loadFrame(path, runMetaL[i]);
			// if a left frame is missing, cover it with the right one (and
			// its metrics) rather than freezing the whole left run.
			if (runImgL[i] == -1) { runImgL[i] = runImgR[i]; runMetaL[i] = runMetaR[i]; }
		}

		for (int i = 0; i < THANOS_DYING_FRAMES; i++) {
			char path[128];
			sprintf_s(path, sizeof(path), "Images\\Thanos\\dying\\%d.png", i + 1);
			dyingImg[i] = loadFrame(path, dyingMeta[i]);
		}

		for (int i = 0; i < THANOS_REVIVING_FRAMES; i++) {
			char path[128];
			sprintf_s(path, sizeof(path), "Images\\Thanos\\Reviving\\%d.png", i + 1);
			revivingImg[i] = loadFrame(path, revivingMeta[i]);
		}

		// Sword: R_1..R_8 all exist. On the left only L_1 does, so every
		// other left frame falls back to the matching R frame - that keeps
		// the left-facing swing a real 8-frame animation instead of a single
		// frozen pose. Which way an R_n file actually faces is NOT what its
		// name says for R_2..R_8 (see THANOS_SWORD_R_FACES_LEFT_FROM), so the
		// flips are worked out from that instead of assumed:
		//   rFacesLeft  -> facing right must flip it; facing left draws it as-is
		//   !rFacesLeft -> facing right draws it as-is; facing left must flip it
		// A real L_n (assumed left-facing, as named) is never flipped. A
		// flipped frame reuses the R frame's metrics; drawThanosFrame() flips
		// the visible columns to match.
		for (int i = 0; i < THANOS_SWORD_FRAMES; i++) {
			char path[128];
			sprintf_s(path, sizeof(path), "Images\\Thanos\\Attack\\Sword\\R_%d.png", i + 1);
			swordImgR[i] = loadFrame(path, swordMetaR[i]);

			bool rFacesLeft = (i >= THANOS_SWORD_R_FACES_LEFT_FROM);
			swordMirrorR[i] = rFacesLeft;

			sprintf_s(path, sizeof(path), "Images\\Thanos\\Attack\\Sword\\L_%d.png", i + 1);
			swordImgL[i] = loadFrame(path, swordMetaL[i]);

			if (swordImgL[i] == -1) {
				swordImgL[i] = swordImgR[i];
				swordMetaL[i] = swordMetaR[i];
				swordMirrorL[i] = !rFacesLeft;
			}
		}

		// Beam charge-up: R_1..R_8 and L_1..L_8 both exist for real, so -
		// unlike the sword - there's no missing-side mirror fallback needed.
		for (int i = 0; i < THANOS_BEAM_CHARGE_FRAMES; i++) {
			char path[128];
			sprintf_s(path, sizeof(path), "Images\\Thanos\\Attack\\beam\\R_%d.png", i + 1);
			beamChargeImgR[i] = loadFrame(path, beamChargeMetaR[i]);

			sprintf_s(path, sizeof(path), "Images\\Thanos\\Attack\\beam\\L_%d.png", i + 1);
			beamChargeImgL[i] = loadFrame(path, beamChargeMetaL[i]);
		}

		// The bolt sprite itself - one fixed-size image per side, not a
		// per-frame clip, so a plain probeLoad() (no ThanosFrameMeta) is
		// enough; drawThanosBeams() draws it at a fixed size regardless of
		// the PNG's own canvas bounds.
		beamProjImgR = probeLoad("Images\\Thanos\\Attack\\beam\\Beam_R.png");
		beamProjImgL = probeLoad("Images\\Thanos\\Attack\\beam\\Beam_L.png");

		// Power stone summon: slot i -> file THANOS_POWER_STONE_FILE_NUM[i]
		// (there is no _6.png). The folder really is spelled "Power Strong" on disk.
		for (int i = 0; i < THANOS_POWER_STONE_FRAMES; i++) {
			char path[128];
			sprintf_s(path, sizeof(path), "Images\\Thanos\\Attack\\Power Strong\\R_%d.png", THANOS_POWER_STONE_FILE_NUM[i]);
			powerStoneImgR[i] = loadFrame(path, powerStoneMetaR[i]);

			sprintf_s(path, sizeof(path), "Images\\Thanos\\Attack\\Power Strong\\L_%d.png", THANOS_POWER_STONE_FILE_NUM[i]);
			powerStoneImgL[i] = loadFrame(path, powerStoneMetaL[i]);
		}

		meteorFallImg = loadFrame("Images\\Thanos\\Attack\\Power Strong\\Falling.png", meteorFallMeta);
		meteorSmashImg = loadFrame("Images\\Thanos\\Attack\\Power Strong\\Smassing.png", meteorSmashMeta);
	}
};

static ThanosBoss thanos;

inline long thanosClockMs() {
	return (long)(clock() * 1000L / CLOCKS_PER_SEC);
}

// Current speed multiplier: 1.0 until enraged (see THANOS_ENRAGE_* above).
inline float thanosSpeedMult() {
	return thanos.enraged ? THANOS_ENRAGE_SPEED_MULT : 1.0f;
}

// A duration in ms played at a given speed multiplier - 1.3x speed = 1/1.3 of
// the time. Rounded to the nearest ms.
inline long thanosScaledMs(long ms, float mult) {
	return (long)((float)ms / mult + 0.5f);
}

// --- LEVEL 3 PACING (FRAME-RATE INDEPENDENT) ---------------------------
// Thanos's walk, his beam bolt and every bullet in the Titan arena used to
// move a fixed number of pixels per RENDERED frame (updateThanos(),
// updateThanosBeams() and updateBullets() are all called from drawLevel3(),
// and the draw loop runs off glutIdleFunc, i.e. as fast as the machine can
// draw). Level 3 is a light scene, so it renders far more frames per second
// than Levels 1/2 - and everything moved that many times faster. The hero
// moves per fixedUpdate() tick, so the boss and the bullets also drifted out
// of proportion with him.
//
// l3BeginFrame() (called once at the top of drawLevel3()) measures the real
// time since the previous frame and stores it as l3FrameTicks: "how many
// reference ticks went by". Every per-tick step in Level 3 is multiplied by
// that, so speed is now the same on a 60Hz laptop and a 240Hz monitor.
//
// TO RETUNE: the three *_SCALE constants below are the only knobs.
//   1.0 = the speed the original px-per-tick constants were designed for
//         (60 ticks/sec), smaller = slower, larger = faster.
const float L3_REF_TICKS_PER_SEC = 60.0f;
const float L3_MAX_FRAME_TICKS = 6.0f;      // clamp for one frame's time step (window drag / hitch must not teleport anything)

const float L3_BULLET_SPEED_SCALE = 0.78f;   // hero bullets in Level 3 (BULLET_SPEED px/tick x this)
const float L3_THANOS_WALK_SCALE  = 1.4f;    // Thanos's chase speed (THANOS_SPEED_PX_PER_TICK x this)
const float L3_THANOS_BEAM_SCALE  = 1.4f;    // his beam bolt's flight speed (THANOS_BEAM_SPEED_PX_PER_TICK x this)

static float l3FrameTicks = 1.0f;           // reference ticks elapsed since the previous Level 3 frame
static LONGLONG l3LastCounter = 0;          // QueryPerformanceCounter reading at the previous frame (0 = none yet)

inline void l3BeginFrame() {
	LARGE_INTEGER freq, now;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&now);

	if (l3LastCounter == 0 || freq.QuadPart <= 0) {
		l3FrameTicks = 1.0f;   // first frame after entering the level - no history to measure
	}
	else {
		double dt = (double)(now.QuadPart - l3LastCounter) / (double)freq.QuadPart;
		if (dt < 0.0) dt = 0.0;
		l3FrameTicks = (float)(dt * L3_REF_TICKS_PER_SEC);
		if (l3FrameTicks > L3_MAX_FRAME_TICKS) l3FrameTicks = L3_MAX_FRAME_TICKS;
	}
	l3LastCounter = now.QuadPart;
}

// Multiplier for a "px per tick" constant this frame. Pass the matching
// L3_*_SCALE knob above.
inline float l3Step(float scale) {
	return l3FrameTicks * scale;
}

// --- BEAM BOLT POOL -----------------------------------------------------
// Small fixed-size pool, same shape as Header/Bullet.hpp's bulletList - no
// dynamic allocation, active flag decides what's actually flying. Kept
// self-contained here (rather than reusing Bullet.hpp) because this file
// deliberately has no dependency on anything outside Hero.hpp/iGraphics.h
// (see the top-of-file comment) and the bolt doesn't need anything
// Bullet.hpp's pool offers beyond "flies in a straight line, hits the hero
// once, then goes away".
struct ThanosBeamBolt {
	float x = 0.0f;
	int   y = 0;       // fixed for the bolt's whole flight - it doesn't arc
	int   dir = 1;      // -1 = flying left, +1 = flying right
	bool  active = false;
	bool  hasHit = false; // one hit per bolt, same idea as swordHitLanded
};

static ThanosBeamBolt thanosBeams[THANOS_MAX_BEAMS];

// --- POWER STONE METEOR -------------------------------------------------
// One meteor at a time is plenty: the cooldown is 10 seconds and a meteor
// lives well under one. FALLING drops it straight down at
// THANOS_METEOR_SPEED_PX_PER_SEC; the tick it reaches the floor it becomes
// SMASHING (impact image, no more damage) for THANOS_METEOR_SMASH_MS, then IDLE.
enum ThanosMeteorPhase { THANOS_METEOR_IDLE = 0, THANOS_METEOR_FALLING = 1, THANOS_METEOR_SMASHING = 2 };

struct ThanosMeteor {
	int phase = THANOS_METEOR_IDLE;
	int centerX = 0;         // registered x - never changes while it falls
	int floorY = 0;          // where its leading edge stops
	float y = 0.0f;          // y of the meteor's leading (lowest) edge
	long lastTickMs = 0;
	long smashStartMs = 0;
	bool hasHit = false;     // one hit per meteor
};

static ThanosMeteor thanosMeteor;

// Called once from Level3.hpp's initLevel3() - stands him up full health,
// alive, facing the hero's side of the arena, with his one revive still
// unspent. y is the arena FLOOR line (L3_FLOOR_TOP_Y), which is both the
// bottom of his collision box and the line every sprite's feet are pinned
// to - see drawThanos().
inline void spawnThanos(int x, int y) {
	thanos.active = true;
	thanos.state = THANOS_ALIVE;
	thanos.x = x;
	thanos.xPos = (float)x;
	thanos.y = y;
	thanos.direction = -1;
	thanos.hp = THANOS_MAX_HP;
	thanos.maxHp = THANOS_MAX_HP;
	thanos.livesLeft = 1;
	thanos.runFrame = 0;
	thanos.lastRunFrameMs = thanosClockMs();
	thanos.dyingFrame = 0;
	thanos.revivingFrame = 0;
	thanos.swordFrame = 0;
	thanos.lastSwordFrameMs = 0;
	thanos.lastSwordEndMs = 0;
	thanos.swordHitLanded = false;
	thanos.lastContactHitMs = 0;
	thanos.beamChargeFrame = 0;
	thanos.lastBeamChargeFrameMs = 0;
	thanos.lastBeamEndMs = 0;
	thanos.enraged = false;
	thanos.actionSpeedMult = 1.0f;
	thanos.powerStoneFrame = 0;
	thanos.powerStoneTargetX = 0;
	thanos.lastPowerStoneEndMs = thanosClockMs();   // the 10s cooldown runs from the start of the fight

	// A fresh entry into Level 3 also clears any bolt left flying from a
	// previous attempt at the fight - see the ThanosBeamBolt pool below.
	for (int i = 0; i < THANOS_MAX_BEAMS; i++) thanosBeams[i].active = false;
	thanosMeteor.phase = THANOS_METEOR_IDLE;
}

// --- SOLID BODY -------------------------------------------------------
// The hero can no longer walk THROUGH him. Level3.hpp's
// heroMoveHorizontalL3() calls this with the hero's PROSPECTIVE box after
// a move and reverts the move if it comes back true, exactly the way it
// already reverts a move into a solid tile. Note this only BLOCKS - being
// blocked does no damage at all (see the contact-damage note above).
//
// Only a Thanos who is actually up counts as solid: while the dying or
// reviving clip is playing, or once he's dead, the hero can walk straight
// through the space he occupies.
inline bool thanosBlocksHero(int x, int y, int w, int h) {
	if (!thanos.active) return false;
	if (thanos.state == THANOS_DEAD || thanos.state == THANOS_DYING || thanos.state == THANOS_REVIVING)
		return false;

	return (x < thanos.x + THANOS_WIDTH) && (x + w > thanos.x) &&
	       (y < thanos.y + THANOS_HEIGHT) && (y + h > thanos.y);
}

// Whichever weapon damaged him - the knife's melee hitbox and every bullet
// (see level.hpp's updateBullets() and the knife-hit branch in
// Playercontroller.hpp) both funnel through here the same way they funnel
// through enemy.h's damageEnemiesInBox() for the regular roster. Returns
// true if the hit actually landed (so a bullet caller knows to stop),
// same contract as damageEnemiesInBox(). Lands while he's ALIVE, mid-sword-
// swing, mid-beam-charge, or mid-power-stone-summon - no hits register while a dying/reviving clip
// is playing.
inline bool damageThanos(int x1, int y1, int x2, int y2, int dmg) {
	if (!thanos.active) return false;
	if (thanos.state != THANOS_ALIVE && thanos.state != THANOS_ATTACK && thanos.state != THANOS_BEAM_CHARGE && thanos.state != THANOS_POWER_STONE) return false;

	bool overlaps = (x1 < thanos.x + THANOS_WIDTH) && (x2 > thanos.x) &&
	                 (y1 < thanos.y + THANOS_HEIGHT) && (y2 > thanos.y);
	if (!overlaps) return false;

	thanos.hp -= dmg;

	// Second life only (livesLeft == 0 - the revive has been spent; during his
	// first life it is still 1) and only once: crossing half of the revived HP
	// pool flips him into the faster phase. hp > 0 so the killing blow itself
	// doesn't enrage a boss who is about to go down.
	if (thanos.livesLeft == 0 && !thanos.enraged && thanos.hp > 0 &&
		(float)thanos.hp <= (float)thanos.maxHp * THANOS_ENRAGE_HP_FRACTION) {
		thanos.enraged = true;
	}

	if (thanos.hp <= 0) {
		thanos.hp = 0;
		thanos.state = THANOS_DYING;
		thanos.dyingFrame = 0;
		thanos.lastDyingFrameMs = thanosClockMs();
		// what happens once the dying clip finishes - revive or stay dead -
		// is decided in updateThanos() below, based on thanos.livesLeft.
	}

	return true;
}

// true if the hero's body is inside the blade's arc: within reachPx in
// FRONT of Thanos (the side he's facing) and vertically overlapping him.
inline bool heroInThanosSwordRange(Hero& hero, int reachPx) {
	int boxLeft = thanos.x;
	int boxRight = thanos.x + THANOS_WIDTH;

	int arcLeft = (thanos.direction >= 0) ? boxLeft : (boxLeft - reachPx);
	int arcRight = (thanos.direction >= 0) ? (boxRight + reachPx) : boxRight;

	return (hero.posX < arcRight) && (hero.posX + hero.WIDTH > arcLeft) &&
	       (hero.posY < thanos.y + THANOS_HEIGHT) && (hero.posY + hero.HEIGHT > thanos.y);
}

// Fires one bolt from Thanos's leading edge (the side he's facing),
// flying along the floor in dir. Called once, the instant a beam charge
// clip finishes (see updateThanos()). Drops silently if every pool slot is
// already in flight - with THANOS_MAX_BEAMS == 2 and only one charge ever
// active at a time that should never actually happen.
inline void spawnThanosBeam(int dir) {
	for (int i = 0; i < THANOS_MAX_BEAMS; i++) {
		if (thanosBeams[i].active) continue;

		int boxLeft = thanos.x;
		int boxRight = thanos.x + THANOS_WIDTH;

		thanosBeams[i].x = (float)((dir >= 0) ? boxRight : (boxLeft - THANOS_BEAM_WIDTH));
		thanosBeams[i].y = thanos.y; // floor-hugging - see THANOS_BEAM_HEIGHT
		thanosBeams[i].dir = (dir >= 0) ? 1 : -1;
		thanosBeams[i].active = true;
		thanosBeams[i].hasHit = false;
		return;
	}
}

// Advances every bolt in flight, lands its damage on the hero at most
// once each, and retires bolts that either connected or left the arena.
// Call once per tick regardless of Thanos's own state - a bolt already
// released keeps flying even if he's mid-swing, mid-charge, or (in the
// unlikely case he dies to a bullet the same tick) even after he's gone,
// so the hero can't cheese a dodge by killing him mid-flight.
inline void updateThanosBeams(Hero& hero, int arenaWidth) {
	for (int i = 0; i < THANOS_MAX_BEAMS; i++) {
		if (!thanosBeams[i].active) continue;

		thanosBeams[i].x += THANOS_BEAM_SPEED_PX_PER_TICK * l3Step(L3_THANOS_BEAM_SCALE) * thanosBeams[i].dir;

		int bx = (int)thanosBeams[i].x;
		int by = thanosBeams[i].y;

		if (!thanosBeams[i].hasHit) {
			bool overlapsHero = (bx < hero.posX + hero.WIDTH) && (bx + THANOS_BEAM_WIDTH > hero.posX) &&
			                     (by < hero.posY + hero.HEIGHT) && (by + THANOS_BEAM_HEIGHT > hero.posY);
			if (overlapsHero) {
				thanosBeams[i].hasHit = true;
				thanosBeams[i].active = false;
				hero.takeDamage(THANOS_BEAM_DMG);
				continue;
			}
		}

		// Left the arena on either side - same "gone once off the
		// relevant screen space" idea as updateBullets()'s screen check in
		// level.hpp, just against the arena bounds since Level 3's camera
		// never moves (see Level3.hpp's updateCameraL3()).
		if (thanosBeams[i].x < -THANOS_BEAM_WIDTH || thanosBeams[i].x > (float)arenaWidth) {
			thanosBeams[i].active = false;
		}
	}
}

// Releases the meteor at the TOP of the screen over centerX - the x the hero
// was standing at when the summon began. Called once, the instant the
// summoning clip finishes (see updateThanos()).
inline void spawnThanosMeteor(int centerX) {
	thanosMeteor.phase = THANOS_METEOR_FALLING;
	thanosMeteor.centerX = centerX;
	thanosMeteor.floorY = thanos.y;          // arena floor line, same one his boots stand on
	thanosMeteor.y = (float)SCREEN_HEIGHT;   // leading edge starts at the top edge, so the sprite is fully above the screen
	thanosMeteor.lastTickMs = thanosClockMs();
	thanosMeteor.smashStartMs = 0;
	thanosMeteor.hasHit = false;
}

// Advances the meteor and lands its damage on the hero at most once. Call
// once per tick regardless of Thanos's own state (same reasoning as
// updateThanosBeams(): a meteor already released keeps falling even if he
// is mid-swing or has just been killed).
//
// The x never changes - only y. The hit test is a column centered on that x
// covering everything the leading edge swept through THIS tick (from where it
// was to where it is, plus the meteor's own height above that), so even a
// long frame cannot step a fast meteor clean over the hero.
inline void updateThanosMeteor(Hero& hero) {
	if (thanosMeteor.phase == THANOS_METEOR_IDLE) return;

	long now = thanosClockMs();

	if (thanosMeteor.phase == THANOS_METEOR_SMASHING) {
		if (now - thanosMeteor.smashStartMs >= THANOS_METEOR_SMASH_MS) thanosMeteor.phase = THANOS_METEOR_IDLE;
		return;
	}

	float dt = (float)(now - thanosMeteor.lastTickMs) / 1000.0f;
	thanosMeteor.lastTickMs = now;
	if (dt < 0.0f) dt = 0.0f;
	if (dt > THANOS_METEOR_MAX_DT_SEC) dt = THANOS_METEOR_MAX_DT_SEC;

	float prevY = thanosMeteor.y;
	float newY = prevY - THANOS_METEOR_SPEED_PX_PER_SEC * dt;

	bool landed = false;
	if (newY <= (float)thanosMeteor.floorY) {
		newY = (float)thanosMeteor.floorY;
		landed = true;
	}
	thanosMeteor.y = newY;

	if (!thanosMeteor.hasHit) {
		int hitLeft = thanosMeteor.centerX - THANOS_METEOR_HIT_WIDTH / 2;
		int hitRight = hitLeft + THANOS_METEOR_HIT_WIDTH;
		int hitBottom = (int)newY;
		int hitTop = (int)prevY + THANOS_METEOR_HIT_HEIGHT;

		bool overlapsHero = (hitLeft < hero.posX + hero.WIDTH) && (hitRight > hero.posX) &&
		                     (hitBottom < hero.posY + hero.HEIGHT) && (hitTop > hero.posY);
		if (overlapsHero) {
			thanosMeteor.hasHit = true;
			hero.takeDamage(THANOS_POWER_STONE_DMG);
		}
	}

	if (landed) {
		thanosMeteor.phase = THANOS_METEOR_SMASHING;
		thanosMeteor.smashStartMs = now;
	}
}

// Chases the hero at THANOS_SPEED_PX_PER_TICK - slower than the hero's own
// Hero::speed (Hero.hpp), so he's always catchable/outrunnable rather than
// an inevitable wall. arenaWidth clamps him to the boss arena (pass
// L3_ARENA_WIDTH from Level3.hpp - see the top-of-file comment for why this
// file doesn't read that constant directly).
inline void updateThanos(Hero& hero, int arenaWidth) {
	if (!thanos.active || thanos.state == THANOS_DEAD) return;

	long now = thanosClockMs();

	if (thanos.state == THANOS_DYING) {
		if (now - thanos.lastDyingFrameMs >= THANOS_DYING_FRAME_MS) {
			thanos.lastDyingFrameMs = now;
			thanos.dyingFrame++;
			if (thanos.dyingFrame >= THANOS_DYING_FRAMES) {
				if (thanos.livesLeft > 0) {
					// First death - spend the revive and switch straight
					// into the reviving clip instead of going DEAD.
					// Spending it is also what UNLOCKS the beam: the charge
					// is gated on livesLeft == 0 (see the trigger logic
					// below), so round two trades the sword-only moveset
					// for sword-when-close + beam-when-far on top of the
					// much bigger HP pool.
					thanos.livesLeft--;
					thanos.state = THANOS_REVIVING;
					thanos.revivingFrame = 0;
					thanos.lastRevivingFrameMs = now;
				}
				else {
					// Second death - no lives left, down for good.
					thanos.state = THANOS_DEAD;
					thanos.active = false;

					isLevel3Complete = true;
					score += THANOS_SCORE_VALUE;
				}
			}
		}
		return;
	}

	if (thanos.state == THANOS_REVIVING) {
		if (now - thanos.lastRevivingFrameMs >= THANOS_REVIVING_FRAME_MS) {
			thanos.lastRevivingFrameMs = now;
			thanos.revivingFrame++;
			if (thanos.revivingFrame >= THANOS_REVIVING_FRAMES) {
				// Back on his feet - bigger HP pool for round two.
				thanos.state = THANOS_ALIVE;
				thanos.maxHp = THANOS_REVIVE_MAX_HP;
				thanos.hp = THANOS_REVIVE_MAX_HP;
			}
		}
		return;
	}

	// --- sword swing (either life) --------------------------------------
	// Rooted in place for the whole clip: no movement, no turning. The
	// facing captured when the swing started is the facing it plays out
	// with, so a hero who runs around behind him mid-swing does not get
	// tracked.
	if (thanos.state == THANOS_ATTACK) {
		if (now - thanos.lastSwordFrameMs >= thanosScaledMs(THANOS_SWORD_FRAME_MS, thanos.actionSpeedMult)) {
			thanos.lastSwordFrameMs = now;
			thanos.swordFrame++;

			// The blade only connects on one specific frame, and only if
			// the hero is STILL inside its reach at that moment - which is
			// the whole point of the long windup. One hit per swing.
			if (thanos.swordFrame == THANOS_SWORD_HIT_FRAME && !thanos.swordHitLanded) {
				thanos.swordHitLanded = true;
				if (heroInThanosSwordRange(hero, THANOS_SWORD_RANGE))
					hero.takeDamage(THANOS_SWORD_DMG);
			}

			if (thanos.swordFrame >= THANOS_SWORD_FRAMES) {
				thanos.state = THANOS_ALIVE;
				thanos.swordFrame = 0;
				thanos.lastSwordEndMs = now;   // cooldown starts counting here
				thanos.swordHitLanded = false;
			}
		}
		return;   // rooted - nothing below this line runs mid-swing
	}

	// --- beam charge (second life only) --------------------------------
	// Same rooted/no-retarget shape as the sword: no movement, no turning,
	// facing locked to whatever it was the instant the charge started. The
	// bolt itself doesn't land a hit here - it's a free-flying projectile
	// from this point on (see updateThanosBeams()), so all this branch
	// does is advance the clip and, on the final frame, release it.
	if (thanos.state == THANOS_BEAM_CHARGE) {
		if (now - thanos.lastBeamChargeFrameMs >= thanosScaledMs(THANOS_BEAM_CHARGE_FRAME_MS, thanos.actionSpeedMult)) {
			thanos.lastBeamChargeFrameMs = now;
			thanos.beamChargeFrame++;

			if (thanos.beamChargeFrame >= THANOS_BEAM_CHARGE_FRAMES) {
				spawnThanosBeam(thanos.direction);

				thanos.state = THANOS_ALIVE;
				thanos.beamChargeFrame = 0;
				thanos.lastBeamEndMs = now;   // cooldown starts counting here, same idea as lastSwordEndMs
			}
		}
		return;   // rooted - nothing below this line runs mid-charge
	}

	// --- power stone summon --------------------------------------------
	// Rooted, facing locked - he cannot move or turn for the whole clip. The
	// frame is derived from elapsed time so the 7 frames always fill exactly
	// THANOS_POWER_STONE_SUMMON_MS. The hero's x was already registered when
	// the summon started (below), so nothing here re-reads where he is: the
	// meteor lands on the spot he WAS standing, and moving away is the dodge.
	if (thanos.state == THANOS_POWER_STONE) {
		long elapsed = now - thanos.powerStoneStartMs;
		long summonMs = thanosScaledMs(THANOS_POWER_STONE_SUMMON_MS, thanos.actionSpeedMult);

		int f = (int)(elapsed * THANOS_POWER_STONE_FRAMES / summonMs);
		if (f >= THANOS_POWER_STONE_FRAMES) f = THANOS_POWER_STONE_FRAMES - 1;
		if (f < 0) f = 0;
		thanos.powerStoneFrame = f;

		if (elapsed >= summonMs) {
			spawnThanosMeteor(thanos.powerStoneTargetX);

			thanos.state = THANOS_ALIVE;
			thanos.powerStoneFrame = 0;
			thanos.lastPowerStoneEndMs = now;   // cooldown starts counting here, same idea as lastSwordEndMs
		}
		return;   // rooted - nothing below this line runs mid-summon
	}

	// --- chase (THANOS_ALIVE only) ---
	int heroCenterX = hero.posX + hero.WIDTH / 2;
	int thanosCenterX = thanos.x + THANOS_WIDTH / 2;

	thanos.direction = (heroCenterX >= thanosCenterX) ? 1 : -1;

	// close beats far: if the hero is within blade reach, the sword always
	// wins over the beam, in EITHER life - a beam charge is exactly the
	// kind of long windup that should never start (or get preferred over
	// the already-in-reach sword) once he's toe-to-toe with the hero.
	bool heroClose = heroInThanosSwordRange(hero, THANOS_SWORD_TRIGGER_RANGE);

	// Start a swing instead of stepping, if the hero is close enough and
	// the cooldown from the last swing has expired. Available in EITHER
	// life now - only the beam below is second-life-only.
	if (heroClose && (now - thanos.lastSwordEndMs) >= THANOS_SWORD_COOLDOWN_MS) {
		thanos.state = THANOS_ATTACK;
		thanos.actionSpeedMult = thanosSpeedMult();
		thanos.swordFrame = 0;
		thanos.lastSwordFrameMs = now;
		thanos.swordHitLanded = false;
		return;
	}

	// Start the power stone summon instead of stepping, if he is on his second
	// life (livesLeft == 0 - see THANOS_POWER_STONE_SECOND_LIFE_ONLY) and the
	// cooldown has expired. Any range - the meteor only cares where the hero IS at this
	// moment. Checked after the sword (close beats far, so a hero toe-to-toe
	// with him still eats the blade first) and before the beam (otherwise the
	// beam's short cooldown would keep re-triggering ahead of it). The hero's
	// center x is registered right here, once, and never updated.
	if ((!THANOS_POWER_STONE_SECOND_LIFE_ONLY || thanos.livesLeft == 0) &&
		thanosMeteor.phase == THANOS_METEOR_IDLE &&
		(now - thanos.lastPowerStoneEndMs) >= THANOS_POWER_STONE_COOLDOWN_MS) {

		thanos.state = THANOS_POWER_STONE;
		thanos.actionSpeedMult = thanosSpeedMult();
		thanos.powerStoneFrame = 0;
		thanos.powerStoneStartMs = now;
		thanos.powerStoneTargetX = heroCenterX;
		return;
	}

	// Start a beam charge instead of stepping, if he's on his second life
	// (livesLeft == 0 - see the revive comment above), the hero is OUTSIDE
	// sword range (no point zapping from range when the blade already
	// reaches), and the cooldown from the last bolt has expired.
	if (thanos.livesLeft == 0 && !heroClose &&
		(now - thanos.lastBeamEndMs) >= THANOS_BEAM_COOLDOWN_MS) {

		thanos.state = THANOS_BEAM_CHARGE;
		thanos.actionSpeedMult = thanosSpeedMult();
		thanos.beamChargeFrame = 0;
		thanos.lastBeamChargeFrameMs = now;
		return;
	}

	float nextX = thanos.xPos + THANOS_SPEED_PX_PER_TICK * thanosSpeedMult() * l3Step(L3_THANOS_WALK_SCALE) * thanos.direction;

	// Arena walls.
	if (nextX < 0.0f) nextX = 0.0f;
	if (nextX > (float)(arenaWidth - THANOS_WIDTH)) nextX = (float)(arenaWidth - THANOS_WIDTH);

	// Don't walk INTO the hero either - the block works both ways, so he
	// stops flush against him instead of overlapping (and, with contact
	// damage gone, being stopped by him costs the hero nothing).
	int candidate = (int)(nextX + 0.5f);
	bool wouldOverlapHero =
		(candidate < hero.posX + hero.WIDTH) && (candidate + THANOS_WIDTH > hero.posX) &&
		(thanos.y < hero.posY + hero.HEIGHT) && (thanos.y + THANOS_HEIGHT > hero.posY);

	if (!wouldOverlapHero) {
		thanos.xPos = nextX;
		thanos.x = candidate;
	}

	// running animation - R series while facing right, L series while
	// facing left (thanos.direction, flipped by the chase logic above).
	// Run-cycle frame time is stretched by the same walk-speed scale, so the
	// legs keep matching the ground speed instead of sliding.
	if (now - thanos.lastRunFrameMs >= thanosScaledMs(THANOS_RUN_FRAME_MS, thanosSpeedMult() * L3_THANOS_WALK_SCALE)) {
		thanos.lastRunFrameMs = now;
		thanos.runFrame = (thanos.runFrame + 1) % THANOS_RUN_FRAMES;
	}
}

// --- drawing ---------------------------------------------------------
// Draws ONE frame at the given fixed scale (screen px per source px - see
// the GROUNDED, CONSISTENT-SIZE ART block at the top; never a per-frame fit),
// standing on the floor:
//
//   * vertically: the lowest VISIBLE row of the art (m.visB) is placed
//     exactly on thanos.y - the arena floor line handed to spawnThanos() - so
//     the transparent padding under his boots hangs below the floor instead of
//     lifting him off it. This is what keeps the reviving clip on the ground.
//   * horizontally: the middle of the VISIBLE art is centered on the
//     COLLISION box's center, so a wide lying-down pose spreads out
//     symmetrically from where he "is" rather than drifting sideways.
//
// mirror flips the sprite left-to-right by handing iShowImage a NEGATIVE
// width - the quad it emits is simply wound the other way, and since
// nothing in this project enables GL_CULL_FACE it still rasterizes, just
// with the texture reversed. That's what covers the sword's missing left
// frames (see ThanosBoss::loadAssets()). Mirroring also mirrors which columns
// are visible, hence the visL/visR swap below.
inline void drawThanosFrame(int img, const ThanosFrameMeta& m, bool mirror, float scale, float cameraX, float cameraY) {
	int drawW = thanosRound(m.srcW * scale);
	int drawH = thanosRound(m.srcH * scale);
	if (drawW < 1) drawW = 1;
	if (drawH < 1) drawH = 1;

	int visL = mirror ? (m.srcW - m.visR) : m.visL;
	int visR = mirror ? (m.srcW - m.visL) : m.visR;

	float visCenterPx = (visL + visR) * 0.5f * scale;   // canvas left edge -> middle of the visible art
	float footPadPx = (m.srcH - m.visB) * scale;      // transparent rows below the boots

	int centerX = thanos.x + THANOS_WIDTH / 2;
	int screenX = thanosRound(centerX - visCenterPx - cameraX);
	int screenY = thanosRound(thanos.y - footPadPx - cameraY);   // boots land ON the floor

	if (img != -1) {
		if (mirror) iShowImage(screenX + drawW, screenY, -drawW, drawH, (unsigned int)img);
		else        iShowImage(screenX, screenY, drawW, drawH, (unsigned int)img);
	}
	else {
		iSetColor(120, 40, 140);
		iFilledRectangle((int)(thanos.x - cameraX), (int)(thanos.y - cameraY), THANOS_WIDTH, THANOS_HEIGHT);
	}
}

inline void drawThanos(float cameraX, float cameraY) {
	if (!thanos.active) return;

	int img = -1;
	const ThanosFrameMeta* meta = &thanos.runMetaR[0];
	float scale = THANOS_RUN_SCALE;
	bool mirror = false;

	if (thanos.state == THANOS_DYING) {
		int f = thanos.dyingFrame;
		if (f >= THANOS_DYING_FRAMES) f = THANOS_DYING_FRAMES - 1;
		if (f < 0) f = 0;
		img = thanos.dyingImg[f];
		meta = &thanos.dyingMeta[f];
		scale = THANOS_ART_SCALE;
	}
	else if (thanos.state == THANOS_REVIVING) {
		int f = thanos.revivingFrame;
		if (f >= THANOS_REVIVING_FRAMES) f = THANOS_REVIVING_FRAMES - 1;
		if (f < 0) f = 0;
		img = thanos.revivingImg[f];
		meta = &thanos.revivingMeta[f];
		scale = THANOS_ART_SCALE;
	}
	else if (thanos.state == THANOS_ATTACK) {
		int f = thanos.swordFrame;
		if (f >= THANOS_SWORD_FRAMES) f = THANOS_SWORD_FRAMES - 1;
		if (f < 0) f = 0;
		scale = THANOS_ART_SCALE;
		if (thanos.direction >= 0) {
			img = thanos.swordImgR[f];
			meta = &thanos.swordMetaR[f];
			mirror = thanos.swordMirrorR[f];
		}
		else {
			img = thanos.swordImgL[f];
			meta = &thanos.swordMetaL[f];
			mirror = thanos.swordMirrorL[f];
		}
	}
	else if (thanos.state == THANOS_BEAM_CHARGE) {
		int f = thanos.beamChargeFrame;
		if (f >= THANOS_BEAM_CHARGE_FRAMES) f = THANOS_BEAM_CHARGE_FRAMES - 1;
		if (f < 0) f = 0;
		scale = THANOS_ART_SCALE;
		// no mirror fallback needed here - both sides are real art (see
		// ThanosBoss::loadAssets()).
		if (thanos.direction >= 0) {
			img = thanos.beamChargeImgR[f];
			meta = &thanos.beamChargeMetaR[f];
		}
		else {
			img = thanos.beamChargeImgL[f];
			meta = &thanos.beamChargeMetaL[f];
		}
	}
	else if (thanos.state == THANOS_POWER_STONE) {
		int f = thanos.powerStoneFrame;
		if (f >= THANOS_POWER_STONE_FRAMES) f = THANOS_POWER_STONE_FRAMES - 1;
		if (f < 0) f = 0;
		scale = THANOS_POWER_SCALE;
		// R series while facing right, L series while facing left - both are
		// real art (see ThanosBoss::loadAssets()). Facing was locked when the
		// summon started, so it can't flip mid-clip.
		if (thanos.direction >= 0) {
			img = thanos.powerStoneImgR[f];
			meta = &thanos.powerStoneMetaR[f];
		}
		else {
			img = thanos.powerStoneImgL[f];
			meta = &thanos.powerStoneMetaL[f];
		}
	}
	else {
		int f = thanos.runFrame;
		if (f >= THANOS_RUN_FRAMES) f = THANOS_RUN_FRAMES - 1;
		if (f < 0) f = 0;
		if (thanos.direction >= 0) {
			img = thanos.runImgR[f];
			meta = &thanos.runMetaR[f];
		}
		else {
			img = thanos.runImgL[f];
			meta = &thanos.runMetaL[f];
		}
		scale = THANOS_RUN_SCALE;
	}

	drawThanosFrame(img, *meta, mirror, scale, cameraX, cameraY);
}

// Draws every bolt currently in flight - camera-relative, same idiom as
// level.hpp's drawBullets(). Call every frame regardless of Thanos's own
// state (see updateThanosBeams()'s comment for why). Drawn BIGGER than the
// hitbox and centered on it (THANOS_BEAM_DRAW_WIDTH/HEIGHT vs
// THANOS_BEAM_WIDTH/HEIGHT), same "bigger sprite, honest hitbox" idiom the
// plasma bolt uses in Header/Bullet.hpp.
inline void drawThanosBeams(float cameraX, float cameraY) {
	for (int i = 0; i < THANOS_MAX_BEAMS; i++) {
		if (!thanosBeams[i].active) continue;

		int img = (thanosBeams[i].dir >= 0) ? thanos.beamProjImgR : thanos.beamProjImgL;

		int drawX = (int)(thanosBeams[i].x - cameraX) - (THANOS_BEAM_DRAW_WIDTH - THANOS_BEAM_WIDTH) / 2;
		int drawY = (int)(thanosBeams[i].y - cameraY) - (THANOS_BEAM_DRAW_HEIGHT - THANOS_BEAM_HEIGHT) / 2;

		if (img != -1) {
			iShowImage(drawX, drawY, THANOS_BEAM_DRAW_WIDTH, THANOS_BEAM_DRAW_HEIGHT, (unsigned int)img);
		}
		else {
			iSetColor(120, 200, 255);
			iFilledRectangle((int)(thanosBeams[i].x - cameraX), (int)(thanosBeams[i].y - cameraY), THANOS_BEAM_WIDTH, THANOS_BEAM_HEIGHT);
		}
	}
}

// Draws the falling meteor, or the impact image once it has landed.
// Anchored like a grounded Thanos frame: the middle of the VISIBLE art on the
// registered x (so the trails sit over the hitbox column, not the PNG's
// transparent margins), and the lowest visible row on the meteor's leading
// edge - which is the floor line for the impact image.
inline void drawThanosMeteor(float cameraX, float cameraY) {
	if (thanosMeteor.phase == THANOS_METEOR_IDLE) return;

	bool smashing = (thanosMeteor.phase == THANOS_METEOR_SMASHING);
	int img = smashing ? thanos.meteorSmashImg : thanos.meteorFallImg;
	const ThanosFrameMeta& m = smashing ? thanos.meteorSmashMeta : thanos.meteorFallMeta;
	float bottomY = smashing ? (float)thanosMeteor.floorY : thanosMeteor.y;

	if (img != -1) {
		float scale = THANOS_METEOR_DRAW_SCALE;
		int drawW = thanosRound(m.srcW * scale);
		int drawH = thanosRound(m.srcH * scale);
		if (drawW < 1) drawW = 1;
		if (drawH < 1) drawH = 1;

		float visCenterPx = (m.visL + m.visR) * 0.5f * scale;
		float footPadPx = (m.srcH - m.visB) * scale;

		int screenX = thanosRound(thanosMeteor.centerX - visCenterPx - cameraX);
		int screenY = thanosRound(bottomY - footPadPx - cameraY);
		iShowImage(screenX, screenY, drawW, drawH, (unsigned int)img);
	}
	else {
		// placeholder = the real hitbox column, so a missing PNG is still playable
		iSetColor(170, 80, 255);
		iFilledRectangle((int)(thanosMeteor.centerX - THANOS_METEOR_HIT_WIDTH / 2 - cameraX), (int)(bottomY - cameraY),
		                 THANOS_METEOR_HIT_WIDTH, smashing ? 40 : THANOS_METEOR_HIT_HEIGHT);
	}
}

// --- HP bar -----------------------------------------------------------
// Same layout idea as SideQuest.hpp's oxygen gauge (dark backing + filled
// bar + outline + centered label). Sits directly BELOW the scoreboard
// plate GameFlow.hpp's drawCenterHUD() draws top-center for every level
// (including Level 3 - see its "SCORE %d" plate, which spans roughly
// SCREEN_HEIGHT-84 down to SCREEN_HEIGHT-8) rather than overlapping it.
//
// Two-layer fill so the color itself tells the story: a full-width RED base
// (the "damage taken" layer) sits underneath, and a YELLOW overlay scaled to
// the current HP fraction sits on top of it. At full health the yellow
// overlay covers the whole bar; as HP drops the yellow shrinks from the
// right and the red base shows through in its place - "yellow draining down
// to red" without needing two separate colors picked by a threshold.
//
// The LABEL now reads his real numbers - "THANOS HP  740 / 1000" - instead
// of the percentage it used to show. The fraction still drives the bar
// itself; the text just reports the actual health, which also makes the
// jump to the 5000-point second-life pool visible rather than resetting to
// a meaningless "100%".
const int THANOS_HP_BAR_WIDTH = 420;
const int THANOS_HP_BAR_HEIGHT = 26;
// Measured from the TOP of the screen (this coordinate system has y=0 at
// the bottom), not as a margin from drawCenterHUD()'s plate directly, since
// this file doesn't include GameFlow.hpp - just picked to clear that
// plate's bottom edge (SCREEN_HEIGHT-84) with room to spare.
const int THANOS_HP_BAR_TOP_OFFSET = 130;

inline void drawThanosHPBar() {
	if (!thanos.active) return;
	// Once he's fully dead there's nothing left to show a bar for. While
	// DYING/REVIVING the bar keeps showing whatever hp/maxHp currently holds
	// (0 during the dying clip, THANOS_REVIVE_MAX_HP the instant reviving
	// finishes), which reads correctly on screen without any special-casing.
	if (thanos.state == THANOS_DEAD) return;

	int barX = SCREEN_WIDTH / 2 - THANOS_HP_BAR_WIDTH / 2;
	int barY = SCREEN_HEIGHT - THANOS_HP_BAR_TOP_OFFSET - THANOS_HP_BAR_HEIGHT;

	int shownHp = thanos.hp;
	if (shownHp < 0) shownHp = 0;
	if (shownHp > thanos.maxHp) shownHp = thanos.maxHp;

	float hpFrac = (thanos.maxHp > 0) ? ((float)shownHp / (float)thanos.maxHp) : 0.0f;
	if (hpFrac < 0.0f) hpFrac = 0.0f;
	if (hpFrac > 1.0f) hpFrac = 1.0f;

	// dark backing, a shade darker than either fill color so the bar has an edge
	iSetColor(35, 10, 10);
	iFilledRectangle(barX - 2, barY - 2, THANOS_HP_BAR_WIDTH + 4, THANOS_HP_BAR_HEIGHT + 4);

	// red base - the full pool, always drawn at full width
	iSetColor(200, 35, 35);
	iFilledRectangle(barX, barY, THANOS_HP_BAR_WIDTH, THANOS_HP_BAR_HEIGHT);

	// yellow overlay - only the current HP fraction, shrinking from the
	// right as he takes damage and exposing the red base beneath it
	iSetColor(255, 215, 0);
	iFilledRectangle(barX, barY, (int)(THANOS_HP_BAR_WIDTH * hpFrac), THANOS_HP_BAR_HEIGHT);

	iSetColor(230, 230, 230);
	iRectangle(barX, barY, THANOS_HP_BAR_WIDTH, THANOS_HP_BAR_HEIGHT);

	// ACTUAL health, not a percentage.
	char line[64];
	sprintf_s(line, sizeof(line), "THANOS HP   %d / %d", shownHp, thanos.maxHp);

	// Rough centering: ~9px per character at HELVETICA_18.
	int textPx = (int)strlen(line) * 9;
	iSetColor(255, 255, 255);
	iText(barX + THANOS_HP_BAR_WIDTH / 2 - textPx / 2, barY + THANOS_HP_BAR_HEIGHT / 2 - 6, line, GLUT_BITMAP_HELVETICA_18);
}

#endif
