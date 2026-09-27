// ===== Puzzle2.hpp =====
#ifndef PUZZLE2_HPP
#define PUZZLE2_HPP

#include <cmath>
#include <cstdio>
#include "Variables.h"
#include "PuzzleCommon.hpp"

int isKeyPressed(unsigned char key); //prototype ta lagbe, direct poll korar jonno

#define PZ2_GRAPH_X 140
#define PZ2_GRAPH_Y 300
#define PZ2_GRAPH_W 1000
#define PZ2_GRAPH_H 260
#define PZ2_SAMPLES 60
#define PZ2_MATCH_THRESHOLD 95.0f

// Match korate hobe ei fixed target sin wave ta
#define PZ2_TARGET_AMP 70.0f
#define PZ2_TARGET_FREQ 2.0f
#define PZ2_TARGET_PHASE 1.2f

// Player wave er default values, base state standard na rakhle weird lagbe
__declspec(selectany) float pz2Amplitude = 25.0f;
__declspec(selectany) float pz2Frequency = 0.6f;
__declspec(selectany) float pz2Phase = 0.0f;
__declspec(selectany) bool pz2Solved = false;

#define PZ2_AMP_MIN 10.0f
#define PZ2_AMP_MAX 100.0f
#define PZ2_FREQ_MIN 0.3f
#define PZ2_FREQ_MAX 4.0f
#define PZ2_PHASE_STEP 0.03f
#define PZ2_AMP_STEP 0.6f
#define PZ2_FREQ_STEP 0.02f

// Sin wave calculation formula
inline float pz2WaveValue(float amp, float freq, float phase, float xt) {
	return amp * sinf((xt * freq * 6.2831853f) + phase);
}

// target wave ar amader player wave koto exact match kortese accuracy % calculate kore
inline float pz2MatchPercent() {
	float totalDiff = 0.0f;
	float maxPossibleDiff = PZ2_AMP_MAX * 2.0f;

	for (int i = 0; i < PZ2_SAMPLES; i++) {
		float xt = (float)i / (float)(PZ2_SAMPLES - 1);
		float target = pz2WaveValue(PZ2_TARGET_AMP, PZ2_TARGET_FREQ, PZ2_TARGET_PHASE, xt);
		float mine = pz2WaveValue(pz2Amplitude, pz2Frequency, pz2Phase, xt);
		totalDiff += fabsf(target - mine);
	}

	float avgDiff = totalDiff / PZ2_SAMPLES;
	float match = 100.0f * (1.0f - (avgDiff / maxPossibleDiff));
	if (match < 0.0f) match = 0.0f;
	if (match > 100.0f) match = 100.0f;
	return match;
}

// A/D, W/S, Q/E diye wave fine-tune korar loop
inline void updatePuzzle2Input() {
	if (pz2Solved) return;

	if (isKeyPressed('a') || isKeyPressed('A')) pz2Amplitude -= PZ2_AMP_STEP;
	if (isKeyPressed('d') || isKeyPressed('D')) pz2Amplitude += PZ2_AMP_STEP;
	if (pz2Amplitude < PZ2_AMP_MIN) pz2Amplitude = PZ2_AMP_MIN;
	if (pz2Amplitude > PZ2_AMP_MAX) pz2Amplitude = PZ2_AMP_MAX;

	if (isKeyPressed('w') || isKeyPressed('W')) pz2Frequency += PZ2_FREQ_STEP;
	if (isKeyPressed('s') || isKeyPressed('S')) pz2Frequency -= PZ2_FREQ_STEP;
	if (pz2Frequency < PZ2_FREQ_MIN) pz2Frequency = PZ2_FREQ_MIN;
	if (pz2Frequency > PZ2_FREQ_MAX) pz2Frequency = PZ2_FREQ_MAX;

	if (isKeyPressed('q') || isKeyPressed('Q')) pz2Phase -= PZ2_PHASE_STEP;
	if (isKeyPressed('e') || isKeyPressed('E')) pz2Phase += PZ2_PHASE_STEP;

	// Threshold cross korlei solve condition trigger hobe
	if (pz2MatchPercent() >= PZ2_MATCH_THRESHOLD) {
		pz2Solved = true;
		grantKeyAndHealth(); // Level 1 / Puzzle 2 -> Key + Health
	}
}

// iGraphics line helper to render wave points
inline void drawWaveLine(float amp, float freq, float phase, double r, double g, double b) {
	iSetColor(r, g, b);
	float prevX = 0, prevY = 0;
	for (int i = 0; i < PZ2_SAMPLES; i++) {
		float xt = (float)i / (float)(PZ2_SAMPLES - 1);
		float y = pz2WaveValue(amp, freq, phase, xt);
		float screenX = PZ2_GRAPH_X + xt * PZ2_GRAPH_W;
		float screenY = PZ2_GRAPH_Y + PZ2_GRAPH_H / 2.0f + y;
		if (i > 0) iLine(prevX, prevY, screenX, screenY);
		prevX = screenX; prevY = screenY;
	}
}

// Puzzle 2 graphic elements draw function
inline void drawPuzzle2Page() {
	iSetColor(10, 8, 16);
	iFilledRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	iSetColor(255, 120, 0);
	iText(PZ2_GRAPH_X, 620, (char*)"[ ARMORY - FREQUENCY LOCK ]", GLUT_BITMAP_TIMES_ROMAN_24);

	iSetColor(50, 50, 60);
	iRectangle(PZ2_GRAPH_X, PZ2_GRAPH_Y, PZ2_GRAPH_W, PZ2_GRAPH_H);
	iLine(PZ2_GRAPH_X, PZ2_GRAPH_Y + PZ2_GRAPH_H / 2.0f, PZ2_GRAPH_X + PZ2_GRAPH_W, PZ2_GRAPH_Y + PZ2_GRAPH_H / 2.0f);

	float match = pz2MatchPercent();

	drawWaveLine(PZ2_TARGET_AMP, PZ2_TARGET_FREQ, PZ2_TARGET_PHASE, 220, 40, 40);
	if (match >= PZ2_MATCH_THRESHOLD)
		drawWaveLine(pz2Amplitude, pz2Frequency, pz2Phase, 60, 255, 90);
	else
		drawWaveLine(pz2Amplitude, pz2Frequency, pz2Phase, 0, 220, 255);

	iSetColor(255, 255, 255);
	iText(PZ2_GRAPH_X, PZ2_GRAPH_Y - 40, (char*)"[A/D] Amplitude   [W/S] Frequency   [Q/E] Phase", GLUT_BITMAP_HELVETICA_18);

	char statusLine[80];
	sprintf_s(statusLine, sizeof(statusLine), "STATUS: %s (%.0f%% MATCH)",
		match >= PZ2_MATCH_THRESHOLD ? "RESONANCE LOCKED" : "RESONANCE MISMATCH", match);
	iSetColor(match >= PZ2_MATCH_THRESHOLD ? 60 : 255, match >= PZ2_MATCH_THRESHOLD ? 255 : 120, 90);
	iText(PZ2_GRAPH_X, PZ2_GRAPH_Y - 80, statusLine, GLUT_BITMAP_HELVETICA_18);
}

#endif