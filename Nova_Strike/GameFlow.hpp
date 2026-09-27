// =====================================================
// GameFlow.hpp
// Owner: Mahdin (Map Design / Game Flow)
//
// One home for every system that cuts ACROSS levels, so none of it has to
// be smeared through level.hpp / Level2.hpp / Playercontroller.hpp (which
// teammates also edit). Everything here is additive - it reads existing
// state (score, currentLevel, hero.currentHP) and owns only its own.
//
// Contents:
//   1. Scalable big text (GLUT stroke fonts - real scaling, unlike the
//      bitmap fonts which cap out at 24px)
//   2. Level 1 oxygen clock + asphyxiation death
//   3. Level intro title cards + hazard warning banners
//   4. Game Over screen (full-screen, glitched red/cyan)
//   5. Persistent high score (highscore.txt)
//   6. Respawn matrix (side quest / L1 / L2 / L3 all differ)
//   7. Score-to-health recovery (every 200 pts -> +20 HP)
// =====================================================
// !! INCLUDE ORDER MATTERS !!
// This header must be #included AFTER "level.hpp", because it calls
// floodProgress() (defined in Level2.hpp, which level.hpp includes at its
// own bottom). Including it before level.hpp gives "identifier not found"
// on floodProgress. It deliberately does NOT #include level.hpp itself -
// that would be circular, since level.hpp's chain pulls this in too.
#ifndef GAMEFLOW_HPP
#define GAMEFLOW_HPP
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <math.h>
#include <string.h>   // _stricmp, used by the save-file reader
#include "Variables.h"
#include "Header\Hero.hpp"

// =====================================================================
// 1. SCALABLE BIG TEXT
//
// iText() only offers GLUT *bitmap* fonts, whose largest option
// (TIMES_ROMAN_24) is fixed at 24px and cannot be scaled - that is why
// every "big" message so far has looked small. GLUT *stroke* fonts are
// vector outlines, so glScalef() genuinely resizes them. These two helpers
// are what every headline in this file draws with.
// =====================================================================
inline float bigTextWidth(const char* text, float scale) {
	float w = 0.0f;
	for (const char* p = text; *p; ++p)
		w += (float)glutStrokeWidth(GLUT_STROKE_ROMAN, *p);
	return w * scale;
}

inline void drawBigText(float x, float y, const char* text, float scale, float thickness) {
	glPushMatrix();
	glTranslatef(x, y, 0.0f);
	glScalef(scale, scale, 1.0f);
	glLineWidth(thickness);
	for (const char* p = text; *p; ++p)
		glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
	glLineWidth(1.0f);
	glPopMatrix();
}

// Centered on the screen's X axis. Every banner/headline uses this so text
// stays centered on the VIEWPORT (which is what the player sees) rather
// than on world coordinates that scroll away with the camera.
inline void drawBigTextCentered(float y, const char* text, float scale, float thickness) {
	drawBigText((SCREEN_WIDTH - bigTextWidth(text, scale)) * 0.5f, y, text, scale, thickness);
}

// Chromatic-aberration / glitch pass: the same string stamped in red and
// cyan a few px apart, then white on top. Cheap, and reads as "corrupted
// terminal" rather than plain colored text.
inline void drawGlitchText(float y, const char* text, float scale, float thickness, float jitter) {
	float x = (SCREEN_WIDTH - bigTextWidth(text, scale)) * 0.5f;

	iSetColor(255, 30, 40);
	drawBigText(x - jitter, y + jitter * 0.4f, text, scale, thickness);

	iSetColor(0, 230, 255);
	drawBigText(x + jitter, y - jitter * 0.4f, text, scale, thickness);

	iSetColor(255, 245, 245);
	drawBigText(x, y, text, scale, thickness);
}

// =====================================================================
// 2. LEVEL 1 OXYGEN CLOCK
//
// 4 minutes of breathable air. When it runs out the hull breach finishes
// the job: HP drains to zero over ~10 seconds. Deliberately NOT a second
// visible countdown - the health bar itself becomes the timer, which is
// far more tense than another number on screen.
// =====================================================================
#define LEVEL_TIME_SECONDS 240
#define GF_TICKS_PER_SECOND 33                 // matches the 30ms fixedUpdate
#define LEVEL_TIME_TICKS (LEVEL_TIME_SECONDS * GF_TICKS_PER_SECOND)
#define ASPHYXIATION_SECONDS 10

__declspec(selectany) int level1ClockTicks = 0;

inline void resetLevel1Clock() { level1ClockTicks = 0; }

// 1.0 at full air -> 0.0 when the breach has vented everything.
inline float oxygenFraction() {
	float p = 1.0f - ((float)level1ClockTicks / (float)LEVEL_TIME_TICKS);
	if (p < 0.0f) p = 0.0f;
	if (p > 1.0f) p = 1.0f;
	return p;
}

inline int oxygenPercent() { return (int)(oxygenFraction() * 100.0f + 0.5f); }

inline int level1SecondsLeft() {
	int left = (LEVEL_TIME_TICKS - level1ClockTicks) / GF_TICKS_PER_SECOND;
	return left < 0 ? 0 : left;
}

inline bool isAsphyxiating() { return level1ClockTicks >= LEVEL_TIME_TICKS; }

// One call per fixed tick while Level 1 is actually live.
inline void updateLevel1Clock() {
	level1ClockTicks++;

	if (!isAsphyxiating()) return;

	// Out of air. Bleed the FULL health bar away across ASPHYXIATION_SECONDS
	// regardless of what max HP happens to be, so retuning HP later can't
	// accidentally make suffocation instant or endless. Accumulated in a
	// float because HP/(10*33) is well under 1 per tick - integer
	// subtraction would truncate to zero and never kill anyone.
	static float asphyxiaCarry = 0.0f;
	asphyxiaCarry += (float)heroMaxHP / (float)(ASPHYXIATION_SECONDS * GF_TICKS_PER_SECOND);
	if (asphyxiaCarry >= 1.0f) {
		int whole = (int)asphyxiaCarry;
		asphyxiaCarry -= (float)whole;
		hero.currentHP -= whole;
		if (hero.currentHP < 0) hero.currentHP = 0;
	}
}

// =====================================================================
// WHOLE-RUN CLOCK
//
// Separate from level1ClockTicks above (that one only runs during Level 1
// and drives the oxygen gauge). This one spans the ENTIRE run, start to
// finish, across every level and every puzzle/side-quest screen, and exists
// purely to feed the victory screen's TIME ELAPSED readout.
//
// Started by startFreshRun() (iMain.cpp) via resetRunClock(), ticked once
// per fixed 30ms step from fixedUpdate() - but only past the point where
// gameOverActive/victoryActive would otherwise freeze the world, so the
// displayed time stops the instant Thanos actually dies rather than
// continuing to climb while the victory screen sits on screen.
// =====================================================================
__declspec(selectany) int runClockTicks = 0;

inline void resetRunClock() { runClockTicks = 0; }
inline void updateRunClock() { runClockTicks++; }
inline int runElapsedSeconds() { return runClockTicks / GF_TICKS_PER_SECOND; }

// =====================================================================
// 3. LEVEL TITLE CARDS + HAZARD BANNERS
// =====================================================================
#define TITLE_CARD_TICKS (4 * GF_TICKS_PER_SECOND)   // ~4s on screen
#define HAZARD_BANNER_TICKS (5 * GF_TICKS_PER_SECOND)

__declspec(selectany) int titleCardTicks = 0;
__declspec(selectany) int titleCardLevel = 0;
__declspec(selectany) int hazardBannerTicks = 0;

// The hazard banner is QUEUED, not started - it only begins counting once
// the title card has fully expired. Running both at once stacked two big
// centered messages on top of each other.
__declspec(selectany) bool hazardBannerQueued = false;

inline void showLevelTitle(int lvl) {
	titleCardLevel = lvl;
	titleCardTicks = TITLE_CARD_TICKS;
	hazardBannerTicks = 0;
	hazardBannerQueued = true;
}

inline void updateBanners() {
	if (titleCardTicks > 0) {
		titleCardTicks--;
		// Title card still up - hold the hazard banner back.
		if (titleCardTicks == 0 && hazardBannerQueued) {
			hazardBannerQueued = false;
			hazardBannerTicks = HAZARD_BANNER_TICKS;
		}
		return;
	}
	if (hazardBannerTicks > 0) hazardBannerTicks--;
}

inline const char* levelTitleText(int lvl) {
	if (lvl == 1) return "BREACH OF SANCTUARY II";
	if (lvl == 2) return "VOID INCURSION";
	return "PLANET TITAN : ENDGAME";
}

// Each level's headline is tinted to its own environment palette: Level 1
// runs red/crimson (hull breach alarm), Level 2 toxic ash-green, Level 3
// the orange-gold of Titan's sky.
inline void levelTitleColor(int lvl, int& r, int& g, int& b) {
	if (lvl == 1) { r = 255; g = 70;  b = 60; }
	else if (lvl == 2) { r = 170; g = 255; b = 120; }
	else { r = 255; g = 180; b = 60; }
}

inline void drawLevelTitleCard() {
	if (titleCardTicks <= 0) return;

	// Fade in over the first ~0.4s and out over the last ~1s so it doesn't
	// pop on and off.
	float t = (float)titleCardTicks / (float)TITLE_CARD_TICKS;
	float alpha = 1.0f;
	if (t > 0.9f)      alpha = (1.0f - t) / 0.1f;
	else if (t < 0.25f) alpha = t / 0.25f;
	if (alpha < 0.0f) alpha = 0.0f;
	if (alpha > 1.0f) alpha = 1.0f;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Letterbox band behind the text, so the headline stays readable over
	// any background.
	glColor4f(0.0f, 0.0f, 0.0f, 0.62f * alpha);
	glBegin(GL_QUADS);
	glVertex2f(0, SCREEN_HEIGHT * 0.5f - 70);
	glVertex2f(SCREEN_WIDTH, SCREEN_HEIGHT * 0.5f - 70);
	glVertex2f(SCREEN_WIDTH, SCREEN_HEIGHT * 0.5f + 70);
	glVertex2f(0, SCREEN_HEIGHT * 0.5f + 70);
	glEnd();

	int r, g, b;
	levelTitleColor(titleCardLevel, r, g, b);

	const char* lvlTag = titleCardLevel == 1 ? "LEVEL 1"
		: titleCardLevel == 2 ? "LEVEL 2" : "LEVEL 3";

	glColor4f(r / 255.0f, g / 255.0f, b / 255.0f, alpha);
	drawBigText((SCREEN_WIDTH - bigTextWidth(lvlTag, 0.16f)) * 0.5f,
		SCREEN_HEIGHT * 0.5f + 28.0f, lvlTag, 0.16f, 2.0f);

	// Headline - thick + large. 0.34 scale on stroke glyphs lands around
	// 45px caps, roughly double what TIMES_ROMAN_24 could ever manage.
	const char* title = levelTitleText(titleCardLevel);
	glColor4f(r / 255.0f, g / 255.0f, b / 255.0f, alpha);
	drawBigText((SCREEN_WIDTH - bigTextWidth(title, 0.34f)) * 0.5f,
		SCREEN_HEIGHT * 0.5f - 34.0f, title, 0.34f, 3.4f);

	glDisable(GL_BLEND);
}

// The per-level environmental alert. Sits top-center of the VIEWPORT.
inline void drawHazardBanner() {
	if (hazardBannerTicks <= 0) return;
	if (currentLevel == 3) return; // Level 3 is a clean arena - no env hazard

	float t = (float)hazardBannerTicks / (float)HAZARD_BANNER_TICKS;
	float alpha = (t > 0.85f) ? (1.0f - t) / 0.15f : (t < 0.2f ? t / 0.2f : 1.0f);
	if (alpha < 0.0f) alpha = 0.0f;
	if (alpha > 1.0f) alpha = 1.0f;

	const char* line = (currentLevel == 1)
		? "ATMOSPHERIC BREACH - CRITICAL OXYGEN VENTING"
		: "BIO-HAZARD - TOXIC FALLOUT PENETRATING SUIT SEALS";

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glColor4f(0.0f, 0.0f, 0.0f, 0.55f * alpha);
	glBegin(GL_QUADS);
	glVertex2f(0, SCREEN_HEIGHT - 132);
	glVertex2f(SCREEN_WIDTH, SCREEN_HEIGHT - 132);
	glVertex2f(SCREEN_WIDTH, SCREEN_HEIGHT - 88);
	glVertex2f(0, SCREEN_HEIGHT - 88);
	glEnd();

	// Pulse between full and dim so it reads as a blinking alarm strip.
	float blink = 0.65f + 0.35f * (float)fabs(sin(pulseTimer * 3.0f));
	if (currentLevel == 1) glColor4f(1.0f, 0.25f * blink, 0.2f * blink, alpha);
	else                   glColor4f(0.7f * blink, 1.0f, 0.45f * blink, alpha);

	drawBigText((SCREEN_WIDTH - bigTextWidth(line, 0.15f)) * 0.5f,
		SCREEN_HEIGHT - 122.0f, line, 0.15f, 2.0f);

	glDisable(GL_BLEND);
}

// =====================================================================
// 5. PERSISTENT SAVE DATA - A TEXT FILE USED AS THE GAME'S DATABASE
//
// Course requirement: "usage of text or binary files as database (saving
// game data, progress, high score etc.)". This section is that database.
// It is a PLAIN TEXT file, human-readable, opened with the C standard
// library only (stdio.h) - no external library, no binary blob.
//
// ---------------------------------------------------------------------
// FILE : highscore.txt   (sits next to the .exe, in the project folder)
// ---------------------------------------------------------------------
// Every line is one record, in the form   <FIELD_NAME><space><INTEGER>.
// Lines starting with '#' are comments and are skipped by the reader.
//
//     # NovaStrike: Ascension - persistent save data
//     NOVASTRIKE_SAVE 1
//     HIGHSCORE 1450
//     TOTAL_RUNS 7
//     LAST_SCORE 300
//
//   NOVASTRIKE_SAVE  format version, so an older save can be migrated
//                    later instead of being silently misread
//   HIGHSCORE        the all-time best score - what the HIGH SCORES page
//                    and the Game Over screen show
//   TOTAL_RUNS       how many runs have been finished (statistic)
//   LAST_SCORE       the score of the most recent run (statistic)
//
// WHY FIELD NAMES INSTEAD OF BARE NUMBERS
// A file of three unlabelled numbers breaks the moment a field is added,
// removed or reordered. Reading by NAME means new fields can be appended
// at any time and old files still load - the reader simply never sees the
// new key and leaves that variable at its default. That is exactly how a
// real save system behaves, and it is why the loader below is a loop over
// lines rather than one fixed fscanf.
//
// FLOW
//   loadGameData()          <- called before anything is displayed
//   commitScoreToHighScore()<- called on every death and level completion
//   saveGameData()          <- writes the whole file back out
// =====================================================================
#define SAVE_FILE_PATH      "highscore.txt"
#define SAVE_FORMAT_VERSION 1

__declspec(selectany) int  highScore = 0;   // all-time best  (HIGHSCORE)
__declspec(selectany) int  totalRuns = 0;   // runs finished  (TOTAL_RUNS)
__declspec(selectany) int  lastScore = 0;   // previous run   (LAST_SCORE)
__declspec(selectany) bool saveDataLoaded = false;   // read-once guard

// --- run time, in whole seconds -------------------------------------------
// Added for the victory screen's TIME ELAPSED readout. Stored in the same
// text-file database as the scores, as two more named records, which is
// exactly the case the "read by NAME" loader above was written for: an old
// highscore.txt with neither field still loads cleanly and simply leaves
// both at 0.
//
//   LAST_TIME - how long the most recently FINISHED run took.
//   BEST_TIME - the fastest winning clear on record. 0 means "no clear yet",
//               which is why commitRunTime() below treats 0 as "any time
//               beats this" rather than as an unbeatable record of zero
//               seconds.
__declspec(selectany) int  lastTime = 0;    // seconds        (LAST_TIME)
__declspec(selectany) int  bestTime = 0;    // seconds, 0 = none yet (BEST_TIME)

// Latched by commitScoreToHighScore() the instant BEFORE it promotes the
// record, so the victory/game-over screens can say "NEW RECORD" truthfully.
// Checking score > highScore after the commit would always be false, since
// the commit is what makes them equal.
__declspec(selectany) bool lastRunBeatHighScore = false;

// ---------------------------------------------------------------- READ --
// Opens the file in text mode ("r"), walks it line by line, and pulls each
// record apart with sscanf_s into a field name + an integer value. Unknown
// field names are ignored rather than treated as an error, which is what
// makes the format forward-compatible.
//
// Guarded by saveDataLoaded so that the draw functions - which call this on
// every single frame - only ever touch the disk once per session.
inline void loadGameData() {
	if (saveDataLoaded) return;
	saveDataLoaded = true;

	// Defaults, used as-is if the file does not exist yet (very first launch).
	highScore = 0;
	totalRuns = 0;
	lastScore = 0;
	lastTime = 0;
	bestTime = 0;

	FILE* f = NULL;
	fopen_s(&f, SAVE_FILE_PATH, "r");
	if (!f) return;                     // no save file yet - keep the defaults

	char line[128];
	while (fgets(line, sizeof(line), f) != NULL) {
		if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;  // comment/blank

		char field[40] = "";
		int  value = 0;

		if (sscanf_s(line, "%39s %d", field, (unsigned)sizeof(field), &value) == 2) {
			if (_stricmp(field, "HIGHSCORE") == 0) highScore = value;
			else if (_stricmp(field, "TOTAL_RUNS") == 0) totalRuns = value;
			else if (_stricmp(field, "LAST_SCORE") == 0) lastScore = value;
			else if (_stricmp(field, "LAST_TIME") == 0) lastTime = value;
			else if (_stricmp(field, "BEST_TIME") == 0) bestTime = value;
			// NOVASTRIKE_SAVE and anything unrecognised: deliberately ignored
		}
		else if (sscanf_s(line, "%d", &value) == 1) {
			// Legacy save: the very first version of this file was a single
			// bare number and nothing else. Still readable, so an old
			// highscore.txt is never thrown away.
			highScore = value;
		}
	}
	fclose(f);

	// A hand-edited file could hold garbage - clamp before anything uses it.
	if (highScore < 0) highScore = 0;
	if (totalRuns < 0) totalRuns = 0;
	if (lastScore < 0) lastScore = 0;
	if (lastTime < 0)  lastTime = 0;
	if (bestTime < 0)  bestTime = 0;
}

// --------------------------------------------------------------- WRITE --
// "w" truncates and rewrites the whole file, so the database always ends up
// in a clean, fully-formed state - no half-updated record can survive a
// crash mid-write. Failing to open is not fatal (a read-only folder just
// means the record does not persist), so it returns quietly.
inline void saveGameData() {
	FILE* f = NULL;
	fopen_s(&f, SAVE_FILE_PATH, "w");
	if (!f) return;

	fprintf(f, "# NovaStrike: Ascension - persistent save data\n");
	fprintf(f, "# Format: <FIELD_NAME> <INTEGER VALUE>, one record per line.\n");
	fprintf(f, "NOVASTRIKE_SAVE %d\n", SAVE_FORMAT_VERSION);
	fprintf(f, "HIGHSCORE %d\n", highScore);
	fprintf(f, "TOTAL_RUNS %d\n", totalRuns);
	fprintf(f, "LAST_SCORE %d\n", lastScore);
	fprintf(f, "# Run times, in whole seconds. BEST_TIME 0 = no clear recorded yet.\n");
	fprintf(f, "LAST_TIME %d\n", lastTime);
	fprintf(f, "BEST_TIME %d\n", bestTime);

	fclose(f);
}

// Older call sites across the project use these two names; kept as thin
// wrappers so nothing else had to change when the format grew.
inline void loadHighScore() { loadGameData(); }
inline void saveHighScore() { saveGameData(); }

// Clears only the record, keeping the run statistics intact. This is what
// the RESET button in SETTINGS calls.
inline void resetHighScoreRecord() {
	loadGameData();
	highScore = 0;
	saveGameData();
}

// Called on every death and every level completion - i.e. once per run, not
// per frame. Updates the statistics, promotes the record if this run beat
// it, and flushes the whole database back to disk.
inline void commitScoreToHighScore() {
	loadGameData();

	totalRuns++;
	lastScore = score;

	// Captured BEFORE the promotion below, not after - checking score >
	// highScore once they've already been made equal would always read
	// false, which is exactly the bug that would make "NEW RECORD!" never
	// show on the victory screen even on a genuine best run.
	lastRunBeatHighScore = (score > highScore);
	if (lastRunBeatHighScore) highScore = score;

	// Run time, in whole seconds - see runElapsedSeconds() above. Recorded
	// on every commit (death or win), same as the score is; bestTime is
	// only ever tightened on a WIN (isLevel3Complete), since "best time" is
	// meant to answer "fastest clear", not "fastest death".
	lastTime = runElapsedSeconds();
	if (isLevel3Complete && (bestTime == 0 || lastTime < bestTime)) bestTime = lastTime;

	saveGameData();
}

// =====================================================================
// 7. SCORE-TO-HEALTH RECOVERY
//
// Every 200 points earned hands back HEAL_PER_MILESTONE HP. Tracked by a
// milestone counter rather than "score % 200 == 0" - score jumps in chunks
// of 5..100 per kill and would routinely skip straight past an exact
// multiple, silently costing the player the heal.
// =====================================================================
#define SCORE_HEAL_INTERVAL 100   // a heal every 100 pts keeps pressure off long fights
#define HEAL_PER_MILESTONE 12     // smaller chunk, twice as often = same rate, smoother
#define HEAL_TOAST_TICKS 60

__declspec(selectany) int lastHealMilestone = 0;
__declspec(selectany) int healToastTicks = 0;

inline void resetScoreHealing() { lastHealMilestone = 0; healToastTicks = 0; }

inline void updateScoreHealing() {
	if (healToastTicks > 0) healToastTicks--;

	int milestone = score / SCORE_HEAL_INTERVAL;
	while (lastHealMilestone < milestone) {
		lastHealMilestone++;
		if (hero.currentHP < heroMaxHP) {
			hero.currentHP += HEAL_PER_MILESTONE;
			if (hero.currentHP > heroMaxHP) hero.currentHP = heroMaxHP;
			healToastTicks = HEAL_TOAST_TICKS;
		}
	}
}

inline void drawHealToast() {
	if (healToastTicks <= 0) return;
	iSetColor(120, 255, 160);
	drawBigTextCentered(SCREEN_HEIGHT - 190.0f, "+12 HP - VITALS STABILIZING", 0.12f, 1.8f);
}

// =====================================================================
// SPACE STONE PICKUP TOAST
// Awarded by the side quest (SideQuest.hpp sets hasSpaceStone +
// spaceStoneToastTicks). Drawn over whatever page is showing, so it is
// visible both inside the quest and after the auto-return to Level 2.
// =====================================================================
__declspec(selectany) unsigned int imgSpaceStone = (unsigned int)-1;
__declspec(selectany) bool spaceStoneArtLoaded = false;

inline void loadSpaceStoneArt() {
	if (spaceStoneArtLoaded) return;
	spaceStoneArtLoaded = true;

	FILE* probe = NULL;
	fopen_s(&probe, "Images\\Level_2\\Space_Stone.png", "rb");
	if (!probe) return;
	fclose(probe);
	imgSpaceStone = iLoadImage((char*)"Images\\Level_2\\Space_Stone.png");
}

inline void updateSpaceStoneToast() {
	if (spaceStoneToastTicks > 0) spaceStoneToastTicks--;
}

inline void drawSpaceStoneToast() {
	if (spaceStoneToastTicks <= 0) return;
	loadSpaceStoneArt();

	// Fade computed from the ticks REMAINING rather than from a fixed total,
	// because the side quest now sets a shorter duration than the old
	// hard-coded 180 (the oxygen banner has to follow it). Working off the
	// remainder keeps the fade-out correct for any duration a caller picks.
	float alpha = 1.0f;
	if (spaceStoneToastTicks < 22) alpha = (float)spaceStoneToastTicks / 22.0f;
	if (alpha < 0.0f) alpha = 0.0f;
	if (alpha > 1.0f) alpha = 1.0f;

	float cy = SCREEN_HEIGHT * 0.5f;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0.03f, 0.0f, 0.09f, 0.80f * alpha);
	glBegin(GL_QUADS);
	glVertex2f(SCREEN_WIDTH * 0.5f - 330, cy - 162);
	glVertex2f(SCREEN_WIDTH * 0.5f + 330, cy - 162);
	glVertex2f(SCREEN_WIDTH * 0.5f + 330, cy + 130);
	glVertex2f(SCREEN_WIDTH * 0.5f - 330, cy + 130);
	glEnd();
	glDisable(GL_BLEND);

	// The stone itself, bobbing gently so it reads as a held artifact.
	if (imgSpaceStone != (unsigned int)-1) {
		float bob = 6.0f * (float)sin(pulseTimer * 2.0f);
		iShowImage((int)(SCREEN_WIDTH * 0.5f - 44), (int)(cy + 10 + bob), 88, 88, imgSpaceStone);
	}

	iSetColor(120, 200, 255);
	drawBigTextCentered(cy - 40.0f, "YOU GOT LOKI'S FAVOURITE STONE", 0.21f, 2.8f);

	iSetColor(170, 170, 200);
	drawBigTextCentered(cy - 82.0f, "SPACE STONE SECURED", 0.12f, 1.6f);

	// The stone is also the Plasma Rifle's unlock condition (key '5'
	// checks hasSpaceStone - see Playercontroller.hpp). Called out here so
	// the new weapon is announced the same way every puzzle-granted weapon
	// is, instead of quietly becoming available with no message at all.
	iSetColor(255, 215, 90);
	drawBigTextCentered(cy - 104.0f, "PLASMA RIFLE UNLOCKED  -  PRESS 5", 0.12f, 1.6f);

	// The stone's own power - see SpaceStone.hpp.
	iSetColor(120, 230, 255);
	drawBigTextCentered(cy - 126.0f, "SPACE STONE POWER  -  PRESS Q TO TELEPORT", 0.12f, 1.6f);

	// Third reward line: the stone also raised max HP
	// (applySpaceStoneHPBoost() in Hero.hpp, called from SideQuest.hpp).
	// Numbers come from the two HP constants so this text can never drift
	// out of sync with the real values.
	char hpLine[64];
	sprintf_s(hpLine, sizeof(hpLine), "HEALTH INCREASED FROM %d TO %d", HERO_BASE_HP, HERO_STONE_HP);
	iSetColor(120, 255, 160);
	drawBigTextCentered(cy - 148.0f, hpLine, 0.12f, 1.6f);
}

// =====================================================================
// 4. GAME OVER SCREEN + 6. RESPAWN MATRIX
//
// Death behaves differently depending on where it happened:
//   Side quest -> silent respawn at the entry block, no Game Over at all
//   Level 1    -> Game Over -> SPACE -> main menu
//   Level 2    -> Game Over -> SPACE -> restart at Level 1
//   Level 3    -> Game Over -> SPACE -> main menu
// deathOriginLevel is latched the moment the player dies so that the
// SPACE handler still knows where it happened even after state resets.
// =====================================================================
__declspec(selectany) bool gameOverActive = false;
__declspec(selectany) int deathOriginLevel = 1;
__declspec(selectany) int gameOverTicks = 0;

inline void triggerGameOver(int originLevel) {
	if (gameOverActive) return;
	gameOverActive = true;
	deathOriginLevel = originLevel;
	gameOverTicks = 0;
	commitScoreToHighScore();
}

inline void clearGameOver() {
	gameOverActive = false;
	gameOverTicks = 0;
}

inline void drawGameOverScreen() {
	if (!gameOverActive) return;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Heavy semi-transparent black over the frozen battlefield - the level
	// underneath stays faintly visible, which is what sells "frozen frame"
	// rather than a flat black screen.
	glColor4f(0.0f, 0.0f, 0.0f, 0.86f);
	glBegin(GL_QUADS);
	glVertex2f(0, 0);
	glVertex2f(SCREEN_WIDTH, 0);
	glVertex2f(SCREEN_WIDTH, SCREEN_HEIGHT);
	glVertex2f(0, SCREEN_HEIGHT);
	glEnd();

	// Terminal scanlines across the whole screen.
	glColor4f(0.0f, 0.0f, 0.0f, 0.22f);
	glBegin(GL_QUADS);
	for (int y = 0; y < SCREEN_HEIGHT; y += 4) {
		glVertex2f(0, (float)y);
		glVertex2f(SCREEN_WIDTH, (float)y);
		glVertex2f(SCREEN_WIDTH, (float)y + 2.0f);
		glVertex2f(0, (float)y + 2.0f);
	}
	glEnd();
	glDisable(GL_BLEND);

	// Jitter grows with a slow pulse so the glitch "breathes" instead of
	// vibrating at a constant, obviously-fake rate.
	float jitter = 3.0f + 2.5f * (float)fabs(sin(pulseTimer * 2.1f));

	drawGlitchText(SCREEN_HEIGHT * 0.5f + 60.0f,
		"YOU FAILED TO SAVE EARTH-616", 0.42f, 4.2f, jitter);

	iSetColor(255, 40, 45);
	drawBigTextCentered(SCREEN_HEIGHT * 0.5f - 20.0f,
		"4,022,673,294 LIVES TURNED TO ASH", 0.19f, 2.4f);

	char line[96];
	iSetColor(0, 220, 255);
	sprintf_s(line, sizeof(line), "FINAL SCORE  %d", score);
	drawBigTextCentered(SCREEN_HEIGHT * 0.5f - 80.0f, line, 0.15f, 2.0f);

	iSetColor(150, 170, 190);
	sprintf_s(line, sizeof(line), "BEST  %d", highScore);
	drawBigTextCentered(SCREEN_HEIGHT * 0.5f - 118.0f, line, 0.12f, 1.6f);

	// Prompt blinks so it never reads as part of the static art.
	if (((int)(pulseTimer * 2.0f)) % 2 == 0) {
		const char* prompt = (deathOriginLevel == 2)
			? "PRESS SPACE TO REDEPLOY FROM LEVEL 1"
			: "PRESS SPACE TO RETURN TO MAIN MENU";
		iSetColor(235, 235, 235);
		drawBigTextCentered(120.0f, prompt, 0.13f, 1.8f);
	}
}

// =====================================================================
// VICTORY SCREEN
//
// The mirror image of the Game Over screen above: Thanos's final death (see
// the THANOS_DEAD branch in Header/thanos.hpp, which flips isLevel3Complete)
// is a finished run same as a death is, it just ends the other way. Reuses
// the exact same "freeze the world behind a full-screen overlay, wait for a
// key, decide where the player goes next" shape - triggerVictory() is
// this screen's triggerGameOver(), victoryActive is this screen's
// gameOverActive - just kept as its own separate flag/screen rather than
// folded into the Game Over path, since the two need to look, sound and
// read completely differently: this one is a celebration, not a corrupted
// terminal.
//
// One-shot per run, guarded by updateGameFlow()'s level3ScoreCommitted the
// same way commitScoreToHighScore() already was for the Thanos-bonus score
// - see the bottom of this file.
// =====================================================================
__declspec(selectany) bool victoryActive = false;
__declspec(selectany) int victoryTicks = 0;

inline void triggerVictory() {
	if (victoryActive) return;
	victoryActive = true;
	victoryTicks = 0;
	// Snapshots score, run time and the high-score/best-time records into
	// highscore.txt right here - see the runElapsedSeconds()/isLevel3Complete
	// reads inside commitScoreToHighScore() itself.
	commitScoreToHighScore();
}

inline void clearVictory() {
	victoryActive = false;
	victoryTicks = 0;
}

// Clean and celebratory on purpose - no scanlines, no chromatic-aberration
// jitter, no "4 billion lives turned to ash" dread. A warm dark-green wash
// instead of Game Over's black, gold/cyan text instead of red/cyan glitch,
// and the two thin gold rules stand in for the "====" ASCII banner this
// layout was modeled on (GLUT's stroke font renders a run of literal '='
// characters as an ugly jagged line at this scale, so a real rectangle
// reads far cleaner than trying to draw the banner out of text).
inline void drawVictoryScreen() {
	if (!victoryActive) return;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0.02f, 0.05f, 0.03f, 0.90f);
	glBegin(GL_QUADS);
	glVertex2f(0, 0);
	glVertex2f(SCREEN_WIDTH, 0);
	glVertex2f(SCREEN_WIDTH, SCREEN_HEIGHT);
	glVertex2f(0, SCREEN_HEIGHT);
	glEnd();
	glDisable(GL_BLEND);

	// FIX: the top rule used to sit at +170, only 30px above the "YOU WON!"
	// baseline (+140) - but GLUT_STROKE_ROMAN capitals at this scale (0.40)
	// stand roughly 48px tall, so the rule was landing INSIDE the letters
	// instead of above them. Pushed both rules out to give the headline a
	// real ~20px clear margin on each side instead of guessing at a gap
	// that happened to be smaller than the text itself.
	iSetColor(255, 205, 70);
	iFilledRectangle(SCREEN_WIDTH * 0.5f - 360, SCREEN_HEIGHT * 0.5f + 210, 720, 3);
	iFilledRectangle(SCREEN_WIDTH * 0.5f - 360, SCREEN_HEIGHT * 0.5f + 122, 720, 3);

	iSetColor(255, 225, 120);
	drawBigTextCentered(SCREEN_HEIGHT * 0.5f + 140.0f, "YOU WON!", 0.40f, 3.8f);

	iSetColor(150, 225, 255);
	drawBigTextCentered(SCREEN_HEIGHT * 0.5f + 82.0f, "YOU SAVED EARTH-616!", 0.185f, 2.3f);

	iSetColor(195, 205, 220);
	drawBigTextCentered(SCREEN_HEIGHT * 0.5f + 42.0f,
		"THE MAD TITAN HAS FALLEN ON PLANET TITAN, AND", 0.100f, 1.4f);
	drawBigTextCentered(SCREEN_HEIGHT * 0.5f + 20.0f,
		"SANCTUARY II HAS BEEN BROUGHT DOWN.", 0.100f, 1.4f);

	iSetColor(70, 92, 84);
	iFilledRectangle(SCREEN_WIDTH * 0.5f - 300, SCREEN_HEIGHT * 0.5f - 26, 600, 2);

	char line[96];
	int secs = runElapsedSeconds();

	iSetColor(0, 220, 255);
	sprintf_s(line, sizeof(line), "FINAL SCORE  %d", score);
	drawBigTextCentered(SCREEN_HEIGHT * 0.5f - 64.0f, line, 0.15f, 2.0f);

	iSetColor(190, 200, 220);
	sprintf_s(line, sizeof(line), "TIME ELAPSED  %d:%02d", secs / 60, secs % 60);
	drawBigTextCentered(SCREEN_HEIGHT * 0.5f - 100.0f, line, 0.125f, 1.7f);

	if (lastRunBeatHighScore) {
		iSetColor(120, 255, 160);
		drawBigTextCentered(SCREEN_HEIGHT * 0.5f - 138.0f, "HIGH SCORE  -  NEW RECORD!", 0.13f, 1.8f);
	}
	else {
		iSetColor(150, 170, 190);
		sprintf_s(line, sizeof(line), "HIGH SCORE  %d", highScore);
		drawBigTextCentered(SCREEN_HEIGHT * 0.5f - 138.0f, line, 0.12f, 1.6f);
	}

	// Prompt blinks, same "don't read as static art" reasoning as Game
	// Over's own prompt just above.
	if (((int)(pulseTimer * 2.0f)) % 2 == 0) {
		iSetColor(230, 230, 230);
		drawBigTextCentered(120.0f, "[ ENTER / M ]  MAIN MENU      [ R ]  PLAY AGAIN", 0.120f, 1.6f);
	}
}

// Watches HP every tick. Side quest deaths are handled by SideQuest.hpp's
// own respawn, so they never reach here.
inline void updateDeathWatch() {
	if (gameOverActive) { gameOverTicks++; return; }

	// PAGE_SIDEQUEST is deliberately excluded - SideQuest.hpp respawns the
	// hero on its own entry platform instead of ending the run.
	if (currentPage != PAGE_PLAYING && currentPage != PAGE_LEVEL3) return;
	if (hero.currentHP > 0) return;

	triggerGameOver(currentPage == PAGE_LEVEL3 ? 3 : currentLevel);
}

// =====================================================================
// HUD - top-center score + the level's environmental bar
// =====================================================================
inline void drawEnvBar(float x, float y, float w, float h, float fill,
	int rFull, int gFull, int bFull) {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glColor4f(0.0f, 0.0f, 0.0f, 0.55f);
	glBegin(GL_QUADS);
	glVertex2f(x, y); glVertex2f(x + w, y);
	glVertex2f(x + w, y + h); glVertex2f(x, y + h);
	glEnd();
	glDisable(GL_BLEND);

	if (fill < 0.0f) fill = 0.0f;
	if (fill > 1.0f) fill = 1.0f;

	iSetColor(rFull, gFull, bFull);
	iFilledRectangle(x + 2, y + 2, (w - 4) * fill, h - 4);

	iSetColor(200, 215, 230);
	iRectangle(x, y, w, h);
}

inline void drawCenterHUD() {
	if (currentPage != PAGE_PLAYING && currentPage != PAGE_LEVEL3) return;
	if (gameOverActive) return;

	char line[96];

	// --- Score, top-center, on its own dark plate ---
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0.02f, 0.04f, 0.07f, 0.68f);
	glBegin(GL_QUADS);
	glVertex2f(SCREEN_WIDTH * 0.5f - 190, SCREEN_HEIGHT - 84);
	glVertex2f(SCREEN_WIDTH * 0.5f + 190, SCREEN_HEIGHT - 84);
	glVertex2f(SCREEN_WIDTH * 0.5f + 190, SCREEN_HEIGHT - 8);
	glVertex2f(SCREEN_WIDTH * 0.5f - 190, SCREEN_HEIGHT - 8);
	glEnd();
	glDisable(GL_BLEND);

	iSetColor(255, 205, 70);
	sprintf_s(line, sizeof(line), "SCORE  %d", score);
	drawBigTextCentered(SCREEN_HEIGHT - 44.0f, line, 0.17f, 2.4f);

	// --- Level-specific environmental readout, directly below ---
	if (currentLevel == 1) {
		int pct = oxygenPercent();
		bool critical = pct <= 20;

		if (critical) iSetColor(255, 60, 50);
		else          iSetColor(120, 220, 255);

		sprintf_s(line, sizeof(line), "OXYGEN  %d%%", pct);
		drawBigTextCentered(SCREEN_HEIGHT - 76.0f, line, 0.115f, 1.7f);

		float f = oxygenFraction();
		drawEnvBar(SCREEN_WIDTH * 0.5f - 150, SCREEN_HEIGHT - 100, 300, 14, f,
			critical ? 255 : 90, critical ? 55 : 200, critical ? 45 : 255);

		if (isAsphyxiating()) {
			iSetColor(255, 40, 40);
			drawBigTextCentered(SCREEN_HEIGHT - 132.0f, "ASPHYXIATION - VITALS FAILING", 0.14f, 2.2f);
		}
	}
	else if (currentLevel == 2) {
		// Toxicity is the inverse of the level-1 bar: it climbs toward 100%.
		// floodProgress() is Level2.hpp's authoritative clock, so the bar and
		// the gas can never disagree about how far along the level is.
		float f = floodProgress();
		int pct = (int)(f * 100.0f + 0.5f);
		bool critical = pct >= 80;

		if (critical) iSetColor(255, 90, 60);
		else          iSetColor(180, 230, 140);

		sprintf_s(line, sizeof(line), "AIR TOXICITY  %d%%", pct);
		drawBigTextCentered(SCREEN_HEIGHT - 76.0f, line, 0.115f, 1.7f);

		drawEnvBar(SCREEN_WIDTH * 0.5f - 150, SCREEN_HEIGHT - 100, 300, 14, f,
			critical ? 255 : 150, critical ? 80 : 210, critical ? 50 : 110);
	}
	// Level 3 deliberately has no environmental bar - clean combat HUD.

	drawHealToast();
}

// Convenience: everything this header wants drawn, in the right order,
// called once from iDraw() AFTER the level has rendered.
inline void drawGameFlowOverlays() {
	drawCenterHUD();
	drawHazardBanner();
	drawLevelTitleCard();
	drawSpaceStoneToast();
	drawGameOverScreen();
	drawVictoryScreen();
}

// Convenience: everything this header wants ticked, called once from
// fixedUpdate() while the world is running.
inline void updateGameFlow() {
	// Beating Thanos is a finished run, but nothing ever went through
	// triggerGameOver() for it, so its score (including the Thanos bonus, see
	// THANOS_SCORE_VALUE in Header/thanos.hpp) never reached highscore.txt.
	// Commit it once, on the tick isLevel3Complete flips to true, via
	// triggerVictory() (which also raises the victory screen itself - see
	// above). The flag re-arms when a fresh run clears isLevel3Complete
	// (startFreshRun()), same lifetime as before.
	static bool level3ScoreCommitted = false;
	if (isLevel3Complete && !level3ScoreCommitted) {
		level3ScoreCommitted = true;
		triggerVictory();
	}
	else if (!isLevel3Complete) {
		level3ScoreCommitted = false;
	}

	updateBanners();
	updateSpaceStoneToast();
	updateScoreHealing();
	updateDeathWatch();

	if (gameOverActive) return;   // world is frozen behind the Game Over screen
	if (victoryActive) return;    // world is frozen behind the victory screen too

	if (currentPage == PAGE_PLAYING && currentLevel == 1)
		updateLevel1Clock();
}

#endif