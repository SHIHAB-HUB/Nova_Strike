#ifndef UICOMPONENTS_HPP
#define UICOMPONENTS_HPP

// Menu stuff logic

#include "Variables.h"
#include "SoundManager.hpp"
#include "level.hpp"
#include "GameFlow.hpp"

// Bg image thakle render korbe, na thakle default dark color
inline void drawBackground() {
	if (bgImage != -1) {
		iShowImage(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, bgImage);
	}
	else {
		iSetColor(10, 15, 30);
		iFilledRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	}
}

inline void drawCoverPage() {
	drawBackground();
}

// =====================================================================
// SHARED UI PRIMITIVES
//
// iGraphics only gives us flat, fully-opaque rectangles. Everything that
// makes the menu pages read as a designed interface rather than a debug
// screen - translucent glass panels, vertical gradients, glowing headings -
// needs raw GL underneath, so it lives here once and every page reuses it.
// =====================================================================

// ---- HOUSE STYLE ----------------------------------------------------
// One palette, used by every subpage, so the whole menu feels like one
// product instead of six unrelated screens.
#define UI_ACCENT_R   0
#define UI_ACCENT_G   225
#define UI_ACCENT_B   255      // cyan - headings, borders, active state
#define UI_TEXT_R     205
#define UI_TEXT_G     220
#define UI_TEXT_B     238      // body copy
#define UI_DIM_R      112
#define UI_DIM_G      134
#define UI_DIM_B      160      // captions, hints, secondary info
#define UI_GOLD_R     255
#define UI_GOLD_G     198
#define UI_GOLD_B     72       // scores and anything "earned"

// Translucent filled rectangle.
inline void uiFillA(float x, float y, float w, float h, int r, int g, int b, float a) {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(r / 255.0f, g / 255.0f, b / 255.0f, a);
	glBegin(GL_QUADS);
	glVertex2f(x, y); glVertex2f(x + w, y);
	glVertex2f(x + w, y + h); glVertex2f(x, y + h);
	glEnd();
	glDisable(GL_BLEND);
}

// Vertical gradient - darker at the bottom, which is what gives the glass
// panels a sense of depth instead of looking like flat cut-outs.
inline void uiGradientV(float x, float y, float w, float h,
	int rTop, int gTop, int bTop, float aTop,
	int rBot, int gBot, int bBot, float aBot) {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);
	glColor4f(rBot / 255.0f, gBot / 255.0f, bBot / 255.0f, aBot);
	glVertex2f(x, y); glVertex2f(x + w, y);
	glColor4f(rTop / 255.0f, gTop / 255.0f, bTop / 255.0f, aTop);
	glVertex2f(x + w, y + h); glVertex2f(x, y + h);
	glEnd();
	glDisable(GL_BLEND);
}

// Corner brackets instead of a closed box. A sci-fi HUD convention, and it
// keeps the frame from fighting the artwork behind it.
inline void uiCornerBrackets(float x, float y, float w, float h, float len, float t,
	int r, int g, int b) {
	iSetColor(r, g, b);
	iFilledRectangle(x, y, len, t);                       // bottom-left
	iFilledRectangle(x, y, t, len);
	iFilledRectangle(x + w - len, y, len, t);             // bottom-right
	iFilledRectangle(x + w - t, y, t, len);
	iFilledRectangle(x, y + h - t, len, t);               // top-left
	iFilledRectangle(x, y + h - len, t, len);
	iFilledRectangle(x + w - len, y + h - t, len, t);     // top-right
	iFilledRectangle(x + w - t, y + h - len, t, len);
}

// Heading with a soft halo: the same stroke text stamped four times at low
// alpha around the centre, then solid on top. This is what makes the
// headings read as lit signage rather than plain coloured text - GLUT has
// no glow of its own.
inline void uiGlowTextCentered(float y, const char* text, float scale, float thickness,
	int r, int g, int b) {
	float x = (SCREEN_WIDTH - bigTextWidth(text, scale)) * 0.5f;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);          // additive - halos accumulate
	glColor4f(r / 255.0f, g / 255.0f, b / 255.0f, 0.16f);
	drawBigText(x - 2.0f, y, text, scale, thickness + 2.0f);
	drawBigText(x + 2.0f, y, text, scale, thickness + 2.0f);
	drawBigText(x, y - 2.0f, text, scale, thickness + 2.0f);
	drawBigText(x, y + 2.0f, text, scale, thickness + 2.0f);
	glDisable(GL_BLEND);

	iSetColor(r, g, b);
	drawBigText(x, y, text, scale, thickness);
}

// Left-aligned version, for section headings inside cards.
inline void uiGlowTextLeft(float x, float y, const char* text, float scale, float thickness,
	int r, int g, int b) {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glColor4f(r / 255.0f, g / 255.0f, b / 255.0f, 0.18f);
	drawBigText(x - 1.5f, y, text, scale, thickness + 1.8f);
	drawBigText(x + 1.5f, y, text, scale, thickness + 1.8f);
	glDisable(GL_BLEND);

	iSetColor(r, g, b);
	drawBigText(x, y, text, scale, thickness);
}

// A rule that fades out at both ends - far less heavy than a hard 1px line
// running the width of a panel.
inline void uiFadeRule(float cx, float y, float halfW, int r, int g, int b) {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);
	glColor4f(r / 255.0f, g / 255.0f, b / 255.0f, 0.0f);
	glVertex2f(cx - halfW, y); glVertex2f(cx - halfW, y + 2.0f);
	glColor4f(r / 255.0f, g / 255.0f, b / 255.0f, 0.85f);
	glVertex2f(cx, y + 2.0f); glVertex2f(cx, y);
	glEnd();
	glBegin(GL_QUADS);
	glColor4f(r / 255.0f, g / 255.0f, b / 255.0f, 0.85f);
	glVertex2f(cx, y); glVertex2f(cx, y + 2.0f);
	glColor4f(r / 255.0f, g / 255.0f, b / 255.0f, 0.0f);
	glVertex2f(cx + halfW, y + 2.0f); glVertex2f(cx + halfW, y);
	glEnd();
	glDisable(GL_BLEND);
}

// Drawn in exactly the same style as the other menu entries, but only while
// a run is parked (ESC during gameplay). Sits directly above "Start Game".
// Small debug skips, stacked in the bottom-right corner. Kept as defines so
// drawHomePage() and iMouse() can never disagree about where they are.
#define DEBUG_BTN_W 96
#define DEBUG_BTN_H 26
#define DEBUG_L2_X (SCREEN_WIDTH - DEBUG_BTN_W - 14)
#define DEBUG_L2_Y 48
#define DEBUG_L3_X (SCREEN_WIDTH - DEBUG_BTN_W - 14)
#define DEBUG_L3_Y 16

// Subpage widget geometry - shared by draw + click so they cannot drift.
#define SETTINGS_BTN_X 820
#define SETTINGS_BTN_W 180
#define SETTINGS_BTN_H 40
#define SETTINGS_FIRST_Y 520
#define SETTINGS_ROW_GAP 110
#define SETTINGS_STEP_W 52
#define SETTINGS_VOL_DOWN_X 690
#define SETTINGS_VOL_UP_X 950
#define SETTINGS_RESET_X 820
#define SCORES_RESET_Y 150

#define INSTR_ENTRY_X 260
#define INSTR_ENTRY_W 760
#define INSTR_ENTRY_H 78
#define INSTR_ENTRY_Y 430

#define RESUME_BTN_X 540
#define RESUME_BTN_Y 560
#define RESUME_BTN_W 200
#define RESUME_BTN_H 50

inline void drawResumeButton() {
	if (!gamePaused) return;

	bool hover = (MX >= RESUME_BTN_X && MX <= RESUME_BTN_X + RESUME_BTN_W) &&
		(MY >= RESUME_BTN_Y && MY <= RESUME_BTN_Y + RESUME_BTN_H);

	if (hover) {
		iSetColor(0, 0, 0);       iFilledRectangle(RESUME_BTN_X - 10, RESUME_BTN_Y - 5, RESUME_BTN_W + 20, RESUME_BTN_H + 10);
		iSetColor(120, 255, 200); iRectangle(RESUME_BTN_X - 10, RESUME_BTN_Y - 5, RESUME_BTN_W + 20, RESUME_BTN_H + 10);
		iSetColor(120, 255, 200); iText(RESUME_BTN_X + 55, RESUME_BTN_Y + 15, (char*)"Resume", GLUT_BITMAP_TIMES_ROMAN_24);
	}
	else {
		iSetColor(0, 0, 0);       iFilledRectangle(RESUME_BTN_X, RESUME_BTN_Y, RESUME_BTN_W, RESUME_BTN_H);
		iSetColor(120, 255, 200); iRectangle(RESUME_BTN_X, RESUME_BTN_Y, RESUME_BTN_W, RESUME_BTN_H);
		iSetColor(120, 255, 200); iText(RESUME_BTN_X + 62, RESUME_BTN_Y + 17, (char*)"Resume", GLUT_BITMAP_HELVETICA_18);
	}

	iSetColor(180, 200, 215);
	iText(465, RESUME_BTN_Y + 68, (char*)"GAME PAUSED - PRESS ESC OR CLICK RESUME", GLUT_BITMAP_HELVETICA_18);
}

// Persistent record, read from highscore.txt on first draw and refreshed
// whenever a run ends (commitScoreToHighScore() in GameFlow.hpp).
inline void drawHighestScoreBanner() {
	loadHighScore();

	char line[80];
	sprintf_s(line, sizeof(line), "HIGHEST SCORE   %d", highScore);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0.02f, 0.03f, 0.06f, 0.72f);
	glBegin(GL_QUADS);
	glVertex2f(SCREEN_WIDTH * 0.5f - 230, SCREEN_HEIGHT - 122);
	glVertex2f(SCREEN_WIDTH * 0.5f + 230, SCREEN_HEIGHT - 122);
	glVertex2f(SCREEN_WIDTH * 0.5f + 230, SCREEN_HEIGHT - 58);
	glVertex2f(SCREEN_WIDTH * 0.5f - 230, SCREEN_HEIGHT - 58);
	glEnd();
	glDisable(GL_BLEND);

	iSetColor(255, 205, 70);
	drawBigTextCentered(SCREEN_HEIGHT - 104.0f, line, 0.19f, 2.6f);
}

// Main menu screen + hover effects
inline void drawHomePage() {
	drawBackground();
	drawResumeButton();
	struct Button { int y; const char* text; int hoverTextX; int normalTextX; };
	Button buttons[6] = {
		{ 480, "Play Game", 582, 588 },
		{ 400, "High Scores", 575, 585 },
		{ 320, "Instructions", 575, 580 },
		{ 240, "Settings", 595, 595 },
		{ 160, "Credits", 600, 605 },
		{ 80, "Exit", 615, 620 }
	};
	for (int i = 0; i < 6; i++) {
		// Mouse hover hoile box ekprakar, na hoile normal box
		if ((MX >= 540 && MX <= 740) && (MY >= buttons[i].y && MY <= buttons[i].y + 50)) {
			iSetColor(0, 0, 0);       iFilledRectangle(530, buttons[i].y - 5, 220, 60);
			iSetColor(255, 255, 255); iRectangle(530, buttons[i].y - 5, 220, 60);
			iSetColor(255, 255, 255); iText(buttons[i].hoverTextX, buttons[i].y + 15, (char*)buttons[i].text, GLUT_BITMAP_TIMES_ROMAN_24);
		}
		else {
			iSetColor(0, 0, 0);       iFilledRectangle(540, buttons[i].y, 200, 50);
			iSetColor(255, 255, 255); iRectangle(540, buttons[i].y, 200, 50);
			iSetColor(255, 255, 255); iText(buttons[i].normalTextX, buttons[i].y + 17, (char*)buttons[i].text, GLUT_BITMAP_HELVETICA_18);
		}
	}

	// Testing er jonno skip buttons - chhoto kore bottom-right e rakha,
	// final build a baad dite hobe. Level 2 ar Level 3 duitar jonno alada.
	bool dbg2Hover = (MX >= DEBUG_L2_X && MX <= DEBUG_L2_X + DEBUG_BTN_W) &&
		(MY >= DEBUG_L2_Y && MY <= DEBUG_L2_Y + DEBUG_BTN_H);
	iSetColor(0, 0, 0);     iFilledRectangle(DEBUG_L2_X, DEBUG_L2_Y, DEBUG_BTN_W, DEBUG_BTN_H);
	iSetColor(150, 50, 50); iRectangle(DEBUG_L2_X, DEBUG_L2_Y, DEBUG_BTN_W, DEBUG_BTN_H);
	iSetColor(dbg2Hover ? 255 : 170, 70, 70);
	iText(DEBUG_L2_X + 12, DEBUG_L2_Y + 9, (char*)"DBG > Lv2", GLUT_BITMAP_HELVETICA_12);

	bool dbg3Hover = (MX >= DEBUG_L3_X && MX <= DEBUG_L3_X + DEBUG_BTN_W) &&
		(MY >= DEBUG_L3_Y && MY <= DEBUG_L3_Y + DEBUG_BTN_H);
	iSetColor(0, 0, 0);     iFilledRectangle(DEBUG_L3_X, DEBUG_L3_Y, DEBUG_BTN_W, DEBUG_BTN_H);
	iSetColor(150, 50, 50); iRectangle(DEBUG_L3_X, DEBUG_L3_Y, DEBUG_BTN_W, DEBUG_BTN_H);
	iSetColor(dbg3Hover ? 255 : 170, 70, 70);
	iText(DEBUG_L3_X + 12, DEBUG_L3_Y + 9, (char*)"DBG > Lv3", GLUT_BITMAP_HELVETICA_12);
}

// ---------------------------------------------------------------- chrome --
// Shared frame for every subpage: dimmed level art behind a bordered glass
// panel, neon title, and a BACK hint. Using the real background instead of
// a flat fill is what makes these read as part of the game rather than a
// debug menu.
inline void drawSubPageFrame(const char* title) {
	drawBackground();

	// Push the artwork back so text always wins the contrast fight, then
	// warm the bottom of the screen slightly - a flat black wash over a
	// whole 1280x720 reads as "unfinished", a graded one reads as designed.
	uiFillA(0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 3, 6, 14, 0.84f);
	uiGradientV(0, 0, (float)SCREEN_WIDTH, 260.0f, 0, 0, 0, 0.0f, 4, 14, 26, 0.55f);

	// ---- glass panel ----
	const float PX = 96.0f, PY = 54.0f;
	const float PW = SCREEN_WIDTH - 192.0f, PH = SCREEN_HEIGHT - 108.0f;

	uiGradientV(PX, PY, PW, PH, 16, 32, 52, 0.90f, 6, 12, 22, 0.94f);

	// Double hairline border - the inner line is what stops the panel edge
	// from looking soft against the dark backdrop.
	iSetColor(30, 96, 130); iRectangle(PX, PY, PW, PH);
	iSetColor(12, 44, 64);  iRectangle(PX + 5, PY + 5, PW - 10, PH - 10);
	uiCornerBrackets(PX, PY, PW, PH, 46.0f, 3.0f, UI_ACCENT_R, UI_ACCENT_G, UI_ACCENT_B);

	// ---- heading ----
	// Title band gives the heading somewhere to sit instead of floating.
	uiFillA(PX + 6, SCREEN_HEIGHT - 168.0f, PW - 12, 76.0f, 0, 140, 190, 0.10f);

	uiGlowTextCentered(SCREEN_HEIGHT - 148.0f, title, 0.30f, 3.4f,
		UI_ACCENT_R, UI_ACCENT_G, UI_ACCENT_B);
	uiFadeRule(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT - 168.0f, 330.0f,
		UI_ACCENT_R, UI_ACCENT_G, UI_ACCENT_B);

	// ---- footer hint ----
	iSetColor(UI_DIM_R, UI_DIM_G, UI_DIM_B);
	iText((int)PX + 26, (int)PY + 22, (char*)"[ESC]  BACK", GLUT_BITMAP_HELVETICA_12);
}

// ---------------------------------------------------------------- scores --
// #11: the persistent record lives HERE now, not as a banner on the menu.
inline void drawScoresPage() {
	drawSubPageFrame("HIGH SCORES");
	loadGameData();              // reads highscore.txt (see GameFlow.hpp)

	char line[96];

	// This page is READ-ONLY on purpose. Clearing the record is a settings
	// action - it does not belong one misclick away from the screen whose
	// only job is to display it.
	iSetColor(UI_DIM_R, UI_DIM_G, UI_DIM_B);
	drawBigTextCentered(SCREEN_HEIGHT * 0.5f + 148.0f, "ALL-TIME BEST", 0.16f, 2.0f);

	sprintf_s(line, sizeof(line), "%d", highScore);
	uiGlowTextCentered(SCREEN_HEIGHT * 0.5f + 20.0f, line, 0.86f, 5.4f,
		UI_GOLD_R, UI_GOLD_G, UI_GOLD_B);

	uiFadeRule(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f - 16.0f, 220.0f, 90, 110, 140);

	// Supporting statistics, pulled from the same text-file database.
	iSetColor(150, 168, 195);
	sprintf_s(line, sizeof(line), "THIS RUN   %d", score);
	drawBigTextCentered(SCREEN_HEIGHT * 0.5f - 66.0f, line, 0.15f, 1.9f);

	iSetColor(120, 140, 168);
	sprintf_s(line, sizeof(line), "PREVIOUS RUN   %d", lastScore);
	drawBigTextCentered(SCREEN_HEIGHT * 0.5f - 104.0f, line, 0.12f, 1.5f);

	sprintf_s(line, sizeof(line), "RUNS COMPLETED   %d", totalRuns);
	drawBigTextCentered(SCREEN_HEIGHT * 0.5f - 138.0f, line, 0.12f, 1.5f);

	iSetColor(74, 92, 116);
	drawBigTextCentered(SCREEN_HEIGHT * 0.5f - 184.0f,
		"SAVED TO HIGHSCORE.TXT - PERSISTS BETWEEN SESSIONS", 0.10f, 1.3f);
}

// ---------------------------------------------------------- instructions --
// Two entries, named by their level titles rather than "Level 1 / 2".
inline void drawInstructionsPage() {
	drawSubPageFrame("MISSION BRIEFING");

	for (int i = 0; i < 2; i++) {
		int y = INSTR_ENTRY_Y - i * (INSTR_ENTRY_H + 18);
		bool hov = (MX >= INSTR_ENTRY_X && MX <= INSTR_ENTRY_X + INSTR_ENTRY_W) &&
			(MY >= y && MY <= y + INSTR_ENTRY_H);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(hov ? 0.10f : 0.05f, hov ? 0.20f : 0.09f, hov ? 0.30f : 0.15f, 0.9f);
		glBegin(GL_QUADS);
		glVertex2f((float)INSTR_ENTRY_X, (float)y);
		glVertex2f((float)(INSTR_ENTRY_X + INSTR_ENTRY_W), (float)y);
		glVertex2f((float)(INSTR_ENTRY_X + INSTR_ENTRY_W), (float)(y + INSTR_ENTRY_H));
		glVertex2f((float)INSTR_ENTRY_X, (float)(y + INSTR_ENTRY_H));
		glEnd();
		glDisable(GL_BLEND);

		iSetColor(hov ? 0 : 40, hov ? 230 : 140, hov ? 255 : 190);
		iRectangle(INSTR_ENTRY_X, y, INSTR_ENTRY_W, INSTR_ENTRY_H);

		iSetColor(hov ? 255 : 200, hov ? 240 : 215, 255);
		iText(INSTR_ENTRY_X + 30, y + 44, (char*)(i == 0 ? "BREACH OF SANCTUARY II" : "VOID INCURSION"),
			GLUT_BITMAP_TIMES_ROMAN_24);
		iSetColor(110, 140, 170);
		iText(INSTR_ENTRY_X + 30, y + 18,
			(char*)(i == 0 ? "Boarding action - the orbital sanctuary" : "Deep core - the derelict Q-ship"),
			GLUT_BITMAP_HELVETICA_12);
	}

	iSetColor(120, 140, 165);
	iText(INSTR_ENTRY_X + 30, 160, (char*)"SELECT A DEPLOYMENT FOR FULL BRIEFING", GLUT_BITMAP_HELVETICA_12);
}

// Body text helper - wraps a fixed set of lines down the panel.
inline void drawBriefingLines(const char* const* lines, int count, int startY) {
	for (int i = 0; i < count; i++) {
		if (lines[i][0] == '>') {
			iSetColor(0, 215, 255);
			iText(210, startY - i * 30, (char*)(lines[i] + 1), GLUT_BITMAP_HELVETICA_18);
		}
		else {
			iSetColor(190, 200, 215);
			iText(210, startY - i * 30, (char*)lines[i], GLUT_BITMAP_HELVETICA_18);
		}
	}
}

inline void drawBriefingLevel1() {
	drawSubPageFrame("BREACH OF SANCTUARY II");
	static const char* L[] = {
		">SITUATION",
		"Thanos has thrown his army at Earth to seize the Mind and Soul",
		"Stones. The orbital station Sanctuary II is the staging ground.",
		"You are Nova. You go in alone, and you go in first.",
		"",
		">THE CLOCK",
		"The breach is venting atmosphere. You have four minutes of air.",
		"When it runs dry, you asphyxiate in seconds. Keep moving.",
		"",
		">OBJECTIVES",
		"Two sealed terminals hold the station's override codes. Crack",
		"both to earn a keycard each - the exit door needs two.",
		"Cracking them also releases sidearms from the armoury lockers.",
		"",
		">CONTROLS",
		"A / D  move      W  jump      SPACE  fire      E  interact",
		"TAB  throw grenade      1-5  switch weapon      ESC  pause"
	};
	drawBriefingLines(L, sizeof(L) / sizeof(L[0]), SCREEN_HEIGHT - 190);
}

inline void drawBriefingLevel2() {
	drawSubPageFrame("VOID INCURSION");
	static const char* L[] = {
		">SITUATION",
		"Past the airlock lies the Q-ship core - a derelict the enemy",
		"has been gutting for parts. Its reactors are already failing.",
		"",
		">THE CLOCK",
		"Toxic fallout is flooding the lower decks and climbing. Four",
		"minutes until saturation. Altitude is survival - go up.",
		"",
		">HAZARDS",
		"Gravity wells invert your footing. Thrust vents launch you far",
		"higher than a jump. Dead sections kill all light but your own.",
		"",
		">OBJECTIVES",
		"Two core nodes must be restored for two more keycards. A third",
		"sealed door is marked DO NOT ENTER - it is optional, it is",
		"where the heavy ordnance is, and it is not safe.",
		"With both keys, the rift on the highest right ledge opens.",
		"",
		">NEXT",
		"Through the rift: Titan. Thanos is waiting."
	};
	drawBriefingLines(L, sizeof(L) / sizeof(L[0]), SCREEN_HEIGHT - 190);
}

// -------------------------------------------------------------- settings --
inline void drawSettingsPage() {
	drawSubPageFrame("SETTINGS");

	int y = SETTINGS_FIRST_Y;

	// Each setting sits on its own row plate so the label, the control and
	// the caption underneath read as one unit.
	// ---------------------------------------------------------- audio --
	uiFillA(190.0f, (float)y - 26.0f, 900.0f, 74.0f, 20, 40, 62, 0.55f);
	iSetColor(0, 150, 200); iFilledRectangle(190, y - 26, 3, 74);

	bool hovMute = (MX >= SETTINGS_BTN_X && MX <= SETTINGS_BTN_X + SETTINGS_BTN_W) &&
		(MY >= y && MY <= y + SETTINGS_BTN_H);

	iSetColor(UI_TEXT_R, UI_TEXT_G, UI_TEXT_B);
	iText(222, y + 16, (char*)"MUSIC & SOUND", GLUT_BITMAP_HELVETICA_18);
	iSetColor(UI_DIM_R, UI_DIM_G, UI_DIM_B);
	iText(222, y - 10, (char*)"Master switch for background music and weapon effects",
		GLUT_BITMAP_HELVETICA_12);

	uiFillA((float)SETTINGS_BTN_X, (float)y, (float)SETTINGS_BTN_W, (float)SETTINGS_BTN_H,
		isMusicMuted ? 90 : 20, isMusicMuted ? 22 : 90, isMusicMuted ? 26 : 56, 0.85f);
	iSetColor(isMusicMuted ? 235 : 90, isMusicMuted ? 85 : 235, isMusicMuted ? 85 : 150);
	iRectangle(SETTINGS_BTN_X, y, SETTINGS_BTN_W, SETTINGS_BTN_H);
	if (hovMute) {
		iSetColor(UI_ACCENT_R, UI_ACCENT_G, UI_ACCENT_B);
		iRectangle(SETTINGS_BTN_X - 3, y - 3, SETTINGS_BTN_W + 6, SETTINGS_BTN_H + 6);
	}
	iSetColor(255, 255, 255);
	iText(SETTINGS_BTN_X + 74, y + 14, (char*)(isMusicMuted ? "OFF" : "ON"), GLUT_BITMAP_HELVETICA_18);

	// --------------------------------------------------------- volume --
	y -= SETTINGS_ROW_GAP;
	uiFillA(190.0f, (float)y - 26.0f, 900.0f, 74.0f, 20, 40, 62, 0.55f);
	iSetColor(0, 150, 200); iFilledRectangle(190, y - 26, 3, 74);

	iSetColor(UI_TEXT_R, UI_TEXT_G, UI_TEXT_B);
	iText(222, y + 16, (char*)"VOLUME", GLUT_BITMAP_HELVETICA_18);
	iSetColor(UI_DIM_R, UI_DIM_G, UI_DIM_B);
	iText(222, y - 10, (char*)"Applies to weapon effects and the Level 3 theme",
		GLUT_BITMAP_HELVETICA_12);

	bool hovDown = (MX >= SETTINGS_VOL_DOWN_X && MX <= SETTINGS_VOL_DOWN_X + SETTINGS_STEP_W) &&
		(MY >= y && MY <= y + SETTINGS_BTN_H);
	bool hovUp = (MX >= SETTINGS_VOL_UP_X && MX <= SETTINGS_VOL_UP_X + SETTINGS_STEP_W) &&
		(MY >= y && MY <= y + SETTINGS_BTN_H);

	uiFillA((float)SETTINGS_VOL_DOWN_X, (float)y, (float)SETTINGS_STEP_W, (float)SETTINGS_BTN_H,
		hovDown ? 0 : 14, hovDown ? 90 : 40, hovDown ? 120 : 58, 0.9f);
	iSetColor(hovDown ? UI_ACCENT_R : 80, hovDown ? UI_ACCENT_G : 150, hovDown ? UI_ACCENT_B : 185);
	iRectangle(SETTINGS_VOL_DOWN_X, y, SETTINGS_STEP_W, SETTINGS_BTN_H);
	iSetColor(255, 255, 255);
	iText(SETTINGS_VOL_DOWN_X + 21, y + 12, (char*)"-", GLUT_BITMAP_TIMES_ROMAN_24);

	// Track + fill. The fill is drawn inset by 2px on every side so the
	// border always stays visible, even at 100%.
	float barX = (float)(SETTINGS_VOL_DOWN_X + SETTINGS_STEP_W + 16);
	uiFillA(barX, (float)y + 8.0f, 196.0f, 18.0f, 6, 14, 24, 0.9f);
	iSetColor(0, 170, 225);
	iFilledRectangle(barX + 2, y + 10, 192.0f * (masterVolume / 100.0f), 14);
	iSetColor(90, 150, 190);
	iRectangle(barX, y + 8, 196, 18);

	uiFillA((float)SETTINGS_VOL_UP_X, (float)y, (float)SETTINGS_STEP_W, (float)SETTINGS_BTN_H,
		hovUp ? 0 : 14, hovUp ? 90 : 40, hovUp ? 120 : 58, 0.9f);
	iSetColor(hovUp ? UI_ACCENT_R : 80, hovUp ? UI_ACCENT_G : 150, hovUp ? UI_ACCENT_B : 185);
	iRectangle(SETTINGS_VOL_UP_X, y, SETTINGS_STEP_W, SETTINGS_BTN_H);
	iSetColor(255, 255, 255);
	iText(SETTINGS_VOL_UP_X + 19, y + 12, (char*)"+", GLUT_BITMAP_TIMES_ROMAN_24);

	char line[96];
	iSetColor(UI_ACCENT_R, UI_ACCENT_G, UI_ACCENT_B);
	sprintf_s(line, sizeof(line), "%d%%", masterVolume);
	iText(SETTINGS_VOL_UP_X + SETTINGS_STEP_W + 18, y + 14, line, GLUT_BITMAP_HELVETICA_18);

	// ---------------------------------------------- high score record --
	// The ONLY place in the game that can wipe the record. The HIGH SCORES
	// page is read-only, and the current value is deliberately NOT echoed
	// here - this row is an action, not a readout.
	y -= SETTINGS_ROW_GAP;
	uiFillA(190.0f, (float)y - 26.0f, 900.0f, 74.0f, 46, 20, 26, 0.50f);
	iSetColor(170, 60, 60); iFilledRectangle(190, y - 26, 3, 74);

	iSetColor(UI_TEXT_R, UI_TEXT_G, UI_TEXT_B);
	iText(222, y + 16, (char*)"HIGH SCORE RECORD", GLUT_BITMAP_HELVETICA_18);
	iSetColor(UI_DIM_R, UI_DIM_G, UI_DIM_B);
	iText(222, y - 10, (char*)"Clears the saved record in highscore.txt - cannot be undone",
		GLUT_BITMAP_HELVETICA_12);

	bool hovReset = (MX >= SETTINGS_RESET_X && MX <= SETTINGS_RESET_X + SETTINGS_BTN_W) &&
		(MY >= y && MY <= y + SETTINGS_BTN_H);
	uiFillA((float)SETTINGS_RESET_X, (float)y, (float)SETTINGS_BTN_W, (float)SETTINGS_BTN_H,
		hovReset ? 120 : 52, 20, 24, 0.9f);
	iSetColor(hovReset ? 255 : 190, 85, 85);
	iRectangle(SETTINGS_RESET_X, y, SETTINGS_BTN_W, SETTINGS_BTN_H);
	iSetColor(255, 228, 228);
	iText(SETTINGS_RESET_X + 62, y + 14, (char*)"RESET", GLUT_BITMAP_HELVETICA_18);

	if (settingsToastTicks > 0) {
		iSetColor(120, 255, 170);
		iText(222, y - 52, (char*)"RECORD CLEARED", GLUT_BITMAP_HELVETICA_18);
	}
}

// =====================================================================
// CREDITS
//
// Laid out as a real title page rather than a list of iText calls:
//
//        [ AUST crest, centred at the top ]
//     Ahsanullah University of Science & Technology        <- header
//     -----------------------------------------------
//     SD Lab 1200                                          <- project block,
//     Game Development Project                                left aligned
//     Project Name: Nova Strike: Ascension
//     -----------------------------------------------
//     +------------------+   +------------------+
//     | SUPERVISED BY    |   | DEVELOPED BY     |          <- 2 x 2 card grid
//     +------------------+   +------------------+
//     | SPECIAL THANKS   |   | LEGAL & COPYRIGHT|
//     +------------------+   +------------------+
//
// EVERY x/y below is derived from the constants in this block, not typed in
// by hand, which is what keeps the two columns and the four cards on exactly
// the same grid. Change CR_MARGIN and the whole page re-aligns itself.
// =====================================================================
#define CR_PANEL_X      60.0f
#define CR_PANEL_Y      36.0f
#define CR_PANEL_W      (SCREEN_WIDTH - 120.0f)     // 1160
#define CR_PANEL_H      (SCREEN_HEIGHT - 76.0f)     // 644  -> spans y 36..680

#define CR_MARGIN       40.0f                       // inset from the panel edge
#define CR_CONTENT_X    (CR_PANEL_X + CR_MARGIN)    // 100 - the left rail
#define CR_CONTENT_W    (CR_PANEL_W - CR_MARGIN * 2)// 1080

#define CR_COL_GAP      32.0f
#define CR_CARD_W       ((CR_CONTENT_W - CR_COL_GAP) * 0.5f)   // 524
#define CR_COL_A_X      CR_CONTENT_X                           // 100
#define CR_COL_B_X      (CR_CONTENT_X + CR_CARD_W + CR_COL_GAP)// 656

#define CR_CARD_H       162.0f
#define CR_ROW_1_Y      270.0f     // top row of cards
#define CR_ROW_2_Y      84.0f      // bottom row of cards

#define CR_LOGO_SIZE    80
#define CR_PLATE_SIZE   92.0f
#define CR_PLATE_Y      588.0f

// Right-hand column inside a card, where every role / ID / annotation sits.
// One number, so all four cards align down the same invisible line.
#define CR_VALUE_DX     296

__declspec(selectany) unsigned int imgAustLogo = (unsigned int)-1;
__declspec(selectany) bool austLogoLoaded = false;

// Same probe-then-load guard used for every optional asset in the project:
// iLoadImage() returns a usable-looking handle even for a missing file, so
// the probe is the only thing that makes the fallback below meaningful.
inline void loadAustLogo() {
	if (austLogoLoaded) return;
	austLogoLoaded = true;

	FILE* probe = NULL;
	fopen_s(&probe, "Images\\aust-logo.png", "rb");
	if (!probe) return;
	fclose(probe);
	imgAustLogo = iLoadImage((char*)"Images\\aust-logo.png");
}

// One card: tinted glass, a coloured spine on the left edge, the section
// heading, and a hairline under it. Returns nothing - the caller fills the
// body using the same cardX/cardY it passed in.
inline void drawCreditsCard(float x, float y, const char* title, float titleScale,
	int r, int g, int b) {
	uiGradientV(x, y, CR_CARD_W, CR_CARD_H, r / 6, g / 6, b / 6, 0.62f, 4, 9, 16, 0.72f);

	iSetColor(r / 3, g / 3, b / 3);
	iRectangle(x, y, CR_CARD_W, CR_CARD_H);

	// Coloured spine - what tells the four sections apart at a glance.
	iSetColor(r, g, b);
	iFilledRectangle(x, y, 4, CR_CARD_H);

	uiGlowTextLeft(x + 22.0f, y + 130.0f, title, titleScale, 2.2f, r, g, b);

	iSetColor(r / 3, g / 3, b / 3);
	iFilledRectangle(x + 22, y + 120, CR_CARD_W - 44, 1);
}

// One "name .......... role" line. The value column is a fixed offset from
// the card's left edge (CR_VALUE_DX), which is what makes all four cards
// line up instead of each one wandering off with its own text width.
inline void drawCreditsRow(float x, float y, const char* name, const char* value) {
	iSetColor(232, 240, 250);
	iText((int)x + 24, (int)y, (char*)name, GLUT_BITMAP_HELVETICA_18);

	if (value && value[0]) {
		iSetColor(UI_DIM_R + 20, UI_DIM_G + 20, UI_DIM_B + 20);
		iText((int)x + CR_VALUE_DX, (int)y + 3, (char*)value, GLUT_BITMAP_HELVETICA_12);
	}
}

inline void drawCreditsPage() {
	drawBackground();
	loadAustLogo();

	// Backdrop: darker and cooler than the other subpages, because this page
	// is a document rather than a menu and wants to read as paper-on-glass.
	uiFillA(0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 2, 5, 12, 0.90f);
	uiGradientV(0, 0, (float)SCREEN_WIDTH, 300.0f, 0, 0, 0, 0.0f, 6, 16, 30, 0.6f);

	uiGradientV(CR_PANEL_X, CR_PANEL_Y, CR_PANEL_W, CR_PANEL_H,
		14, 30, 50, 0.90f, 5, 10, 20, 0.94f);
	iSetColor(28, 92, 126); iRectangle(CR_PANEL_X, CR_PANEL_Y, CR_PANEL_W, CR_PANEL_H);
	iSetColor(10, 40, 58);  iRectangle(CR_PANEL_X + 5, CR_PANEL_Y + 5, CR_PANEL_W - 10, CR_PANEL_H - 10);
	uiCornerBrackets(CR_PANEL_X, CR_PANEL_Y, CR_PANEL_W, CR_PANEL_H, 46.0f, 3.0f,
		UI_ACCENT_R, UI_ACCENT_G, UI_ACCENT_B);

	// ------------------------------------------------------------ crest --
	// The crest artwork has an opaque white background, so it is mounted on
	// a deliberate white plate with a cyan keyline. Dropping it straight
	// onto the dark panel would read as a white box someone forgot to crop;
	// mounted like this it reads as an official seal.
	float plateX = SCREEN_WIDTH * 0.5f - CR_PLATE_SIZE * 0.5f;

	uiFillA(plateX - 5.0f, CR_PLATE_Y - 5.0f, CR_PLATE_SIZE + 10.0f, CR_PLATE_SIZE + 10.0f,
		UI_ACCENT_R, UI_ACCENT_G, UI_ACCENT_B, 0.22f);
	iSetColor(255, 255, 255);
	iFilledRectangle(plateX, CR_PLATE_Y, CR_PLATE_SIZE, CR_PLATE_SIZE);
	iSetColor(UI_ACCENT_R, UI_ACCENT_G, UI_ACCENT_B);
	iRectangle(plateX, CR_PLATE_Y, CR_PLATE_SIZE, CR_PLATE_SIZE);

	if (imgAustLogo != (unsigned int)-1) {
		iShowImage((int)(SCREEN_WIDTH * 0.5f - CR_LOGO_SIZE * 0.5f),
			(int)(CR_PLATE_Y + (CR_PLATE_SIZE - CR_LOGO_SIZE) * 0.5f),
			CR_LOGO_SIZE, CR_LOGO_SIZE, imgAustLogo);
	}
	else {
		iSetColor(20, 110, 60);
		drawBigTextCentered(CR_PLATE_Y + 34.0f, "AUST", 0.28f, 3.0f);
	}

	// ----------------------------------------------------------- header --
	uiGlowTextCentered(552.0f, "AHSANULLAH UNIVERSITY OF SCIENCE & TECHNOLOGY",
		0.185f, 2.4f, 235, 245, 255);
	uiFadeRule(SCREEN_WIDTH * 0.5f, 536.0f, 460.0f, UI_ACCENT_R, UI_ACCENT_G, UI_ACCENT_B);

	// ----------------------------------------------------- project block --
	// Left-aligned against the same rail the cards start from, so the page
	// has one continuous left edge from top to bottom.
	uiGlowTextLeft(CR_CONTENT_X, 500.0f, "SD LAB 1200", 0.155f, 2.2f,
		UI_ACCENT_R, UI_ACCENT_G, UI_ACCENT_B);

	iSetColor(210, 224, 240);
	iText((int)CR_CONTENT_X + 2, 470, (char*)"Game Development Project", GLUT_BITMAP_HELVETICA_18);

	iSetColor(UI_DIM_R, UI_DIM_G, UI_DIM_B);
	iText((int)CR_CONTENT_X + 2, 446, (char*)"Project Name:", GLUT_BITMAP_HELVETICA_18);
	iSetColor(UI_GOLD_R, UI_GOLD_G, UI_GOLD_B);
	iText((int)CR_CONTENT_X + 110, 446, (char*)"Nova Strike: Ascension", GLUT_BITMAP_HELVETICA_18);

	iSetColor(22, 70, 96);
	iFilledRectangle(CR_CONTENT_X, 424, CR_CONTENT_W, 1);

	// ================================================== SECTION 1 of 4 ==
	drawCreditsCard(CR_COL_A_X, CR_ROW_1_Y, "SUPERVISED BY", 0.128f, 0, 200, 255);
	drawCreditsRow(CR_COL_A_X, CR_ROW_1_Y + 88.0f, "Mr. Saha Reno", "Assistant Professor");
	drawCreditsRow(CR_COL_A_X, CR_ROW_1_Y + 60.0f, "Md. Zahid Hossain", "Lecturer, Grade-I");
	iSetColor(UI_DIM_R - 20, UI_DIM_G - 20, UI_DIM_B - 20);
	iText((int)CR_COL_A_X + 24, (int)CR_ROW_1_Y + 24,
		(char*)"Department of Computer Science & Engineering, AUST", GLUT_BITMAP_HELVETICA_12);

	// ================================================== SECTION 2 of 4 ==
	drawCreditsCard(CR_COL_B_X, CR_ROW_1_Y, "DEVELOPED BY", 0.128f, 110, 255, 190);
	drawCreditsRow(CR_COL_B_X, CR_ROW_1_Y + 92.0f, "Mahdin Al Rahman", "00725105101122");
	drawCreditsRow(CR_COL_B_X, CR_ROW_1_Y + 66.0f, "Abdullah Al Amin Monim", "00725105101127");
	drawCreditsRow(CR_COL_B_X, CR_ROW_1_Y + 40.0f, "Md. Shihabul Islam", "00725105101142");
	iSetColor(UI_DIM_R - 20, UI_DIM_G - 20, UI_DIM_B - 20);
	iText((int)CR_COL_B_X + 24, (int)CR_ROW_1_Y + 14,
		(char*)"B.Sc. in Computer Science & Engineering", GLUT_BITMAP_HELVETICA_12);

	// ================================================== SECTION 3 of 4 ==
	drawCreditsCard(CR_COL_A_X, CR_ROW_2_Y, "SPECIAL THANKS & DEVELOPER ASSISTANCE",
		0.108f, 190, 155, 255);
	drawCreditsRow(CR_COL_A_X, CR_ROW_2_Y + 92.0f, "Anthropic Claude", "Logic & Debugging");
	drawCreditsRow(CR_COL_A_X, CR_ROW_2_Y + 68.0f, "Google Gemini", "Asset Sourcing & AI Assistance");
	drawCreditsRow(CR_COL_A_X, CR_ROW_2_Y + 44.0f, "Google Search", "Asset Research & Sprite Sourcing");
	drawCreditsRow(CR_COL_A_X, CR_ROW_2_Y + 20.0f, "iGraphics Framework", "Engine & Rendering Base");

	// ================================================== SECTION 4 of 4 ==
	drawCreditsCard(CR_COL_B_X, CR_ROW_2_Y, "LEGAL & COPYRIGHT", 0.128f, 255, 180, 80);
	iSetColor(150, 120, 70);
	iText((int)CR_COL_B_X + CR_VALUE_DX, (int)CR_ROW_2_Y + 133,
		(char*)"( PLEASE DON'T SUE US )", GLUT_BITMAP_HELVETICA_12);

	iSetColor(255, 205, 130);
	iText((int)CR_COL_B_X + 24, (int)CR_ROW_2_Y + 92,
		(char*)"Marvel, Disney & Thanos", GLUT_BITMAP_HELVETICA_18);

	iSetColor(178, 190, 206);
	iText((int)CR_COL_B_X + 24, (int)CR_ROW_2_Y + 70,
		(char*)"\"Thanos\", \"Sanctuary II\" and \"Earth-616\" belong entirely",
		GLUT_BITMAP_HELVETICA_12);
	iText((int)CR_COL_B_X + 24, (int)CR_ROW_2_Y + 54,
		(char*)"to Marvel Studios and The Walt Disney Company.",
		GLUT_BITMAP_HELVETICA_12);
	iText((int)CR_COL_B_X + 24, (int)CR_ROW_2_Y + 32,
		(char*)"We are broke university students making a zero-budget",
		GLUT_BITMAP_HELVETICA_12);
	iText((int)CR_COL_B_X + 24, (int)CR_ROW_2_Y + 16,
		(char*)"game for a grade. Please don't snap us out of existence.",
		GLUT_BITMAP_HELVETICA_12);

	// ----------------------------------------------------------- footer --
	iSetColor(UI_DIM_R, UI_DIM_G, UI_DIM_B);
	iText((int)CR_CONTENT_X, (int)CR_PANEL_Y + 18, (char*)"[ESC]  BACK", GLUT_BITMAP_HELVETICA_12);

	iSetColor(70, 88, 112);
	iText((int)(CR_CONTENT_X + CR_CONTENT_W) - 150, (int)CR_PANEL_Y + 18,
		(char*)"NOVA STRIKE: ASCENSION  v1.0", GLUT_BITMAP_HELVETICA_12);
}

// Gameplay draw call
inline void drawGameplayPage() {
	drawGameLevel();
}

#endif