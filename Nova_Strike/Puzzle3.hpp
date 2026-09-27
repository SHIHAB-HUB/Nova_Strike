// =====================================================================
// Puzzle3.hpp - "CLEAR ASTEROIDS"  (Level 2, door 1 -> AK47 + Key)
//
// Translucent green tactical overlay. Debris drifts right-to-left across
// the scope; click it to destroy it. 20 confirmed kills completes the
// puzzle and hands over the AK47 + a key.
//
// Design notes that matter for feel:
//  * Every asteroid moves on the FIXED 30ms tick (updatePuzzle3), never on
//    the draw call - so the drift speed is identical no matter what frame
//    rate the machine renders at, and there is no jitter.
//  * Progress PERSISTS. Leaving with ESC keeps pz3Destroyed, so the player
//    can come back and finish. Only a solved puzzle resets nothing.
//  * Too many misses overloads the targeting scope: it shuts down for a
//    few seconds (clicks ignored, static drawn), then comes back with the
//    miss counter cleared. It costs time, not progress.
// =====================================================================
#ifndef PUZZLE3_HPP
#define PUZZLE3_HPP

#include <math.h>
#include <stdio.h>
#include "Variables.h"
#include "PuzzleCommon.hpp"

#define PZ3_TARGET_KILLS   20
#define PZ3_MAX_ROCKS      9
#define PZ3_MISS_LIMIT     5
#define PZ3_SHUTDOWN_TICKS 90   // ~2.7s scope lockout after too many misses

// Scope rectangle on screen (the translucent green viewport).
#define PZ3_VIEW_X 190
#define PZ3_VIEW_Y 110
#define PZ3_VIEW_W 900
#define PZ3_VIEW_H 480

struct Pz3Rock {
	float x, y;
	float vx, vy;    // px per tick
	float radius;
	float spin;      // current rotation, radians
	float spinRate;
	int popTicks;    // >0 = playing its destruction burst, not clickable
	bool active;
};

__declspec(selectany) Pz3Rock pz3Rocks[PZ3_MAX_ROCKS];
__declspec(selectany) bool pz3Initialized = false;
__declspec(selectany) bool pz3Solved = false;
__declspec(selectany) int pz3Destroyed = 0;   // persists across ESC
__declspec(selectany) int pz3Misses = 0;
__declspec(selectany) int pz3ShutdownTicks = 0;
__declspec(selectany) int pz3Seed = 12345;
__declspec(selectany) int pz3Tick = 0;

// Tiny deterministic RNG - rand() is seeded elsewhere by the enemy code and
// we don't want to disturb it, and this keeps spawn behaviour reproducible.
inline int pz3Rand(int lo, int hi) {
	pz3Seed = pz3Seed * 1103515245 + 12345;
	int v = (pz3Seed >> 16) & 0x7FFF;
	return lo + v % (hi - lo + 1);
}

// Respawns one rock off the right edge of the scope at a fresh height/speed.
inline void pz3SpawnRock(Pz3Rock& r, bool startOffscreen) {
	r.radius = (float)pz3Rand(18, 34);
	r.y = (float)pz3Rand(PZ3_VIEW_Y + 40, PZ3_VIEW_Y + PZ3_VIEW_H - 40);
	r.x = startOffscreen
		? (float)(PZ3_VIEW_X + PZ3_VIEW_W + pz3Rand(20, 400))
		: (float)pz3Rand(PZ3_VIEW_X + 60, PZ3_VIEW_X + PZ3_VIEW_W - 60);
	r.vx = -(float)pz3Rand(15, 38) / 10.0f;      // -1.5 .. -3.8 px/tick
	r.vy = (float)pz3Rand(-8, 8) / 10.0f;        // gentle vertical drift
	r.spin = (float)pz3Rand(0, 628) / 100.0f;
	r.spinRate = (float)pz3Rand(-4, 4) / 100.0f;
	r.popTicks = 0;
	r.active = true;
}

inline void initPuzzle3() {
	for (int i = 0; i < PZ3_MAX_ROCKS; i++) pz3SpawnRock(pz3Rocks[i], true);
	pz3Misses = 0;
	pz3ShutdownTicks = 0;
	pz3Initialized = true;
}

// Full reset - only used when a brand new run starts (see iMain.cpp).
inline void resetPuzzle3() {
	pz3Initialized = false;
	pz3Solved = false;
	pz3Destroyed = 0;
	pz3Misses = 0;
	pz3ShutdownTicks = 0;
	pz3Tick = 0;
}

// Fixed-tick update. Called ONLY while PAGE_PUZZLE3 is open, which is also
// why the rest of the world stays frozen while the player is in here.
inline void updatePuzzle3() {
	if (!pz3Initialized) initPuzzle3();
	pz3Tick++;

	if (pz3ShutdownTicks > 0) {
		pz3ShutdownTicks--;
		if (pz3ShutdownTicks == 0) pz3Misses = 0; // scope back online, slate clean
		return;                                    // nothing drifts while offline
	}

	for (int i = 0; i < PZ3_MAX_ROCKS; i++) {
		Pz3Rock& r = pz3Rocks[i];
		if (!r.active) continue;

		if (r.popTicks > 0) {
			r.popTicks--;
			if (r.popTicks == 0) pz3SpawnRock(r, true); // burst finished, send a new one in
			continue;
		}

		r.x += r.vx;
		r.y += r.vy;
		r.spin += r.spinRate;

		// Bounce softly off the top/bottom of the scope instead of vanishing.
		if (r.y < PZ3_VIEW_Y + r.radius) { r.y = PZ3_VIEW_Y + r.radius; r.vy = -r.vy; }
		if (r.y > PZ3_VIEW_Y + PZ3_VIEW_H - r.radius) { r.y = PZ3_VIEW_Y + PZ3_VIEW_H - r.radius; r.vy = -r.vy; }

		// Drifted out the left side - recycle it from the right.
		if (r.x < PZ3_VIEW_X - r.radius - 30.0f) pz3SpawnRock(r, true);
	}
}

inline void handlePuzzle3Click(int mx, int my) {
	if (pz3Solved) return;
	if (pz3ShutdownTicks > 0) return;               // scope is down, input dead
	if (mx < PZ3_VIEW_X || mx > PZ3_VIEW_X + PZ3_VIEW_W ||
		my < PZ3_VIEW_Y || my > PZ3_VIEW_Y + PZ3_VIEW_H) return; // outside the scope isn't a miss

	for (int i = 0; i < PZ3_MAX_ROCKS; i++) {
		Pz3Rock& r = pz3Rocks[i];
		if (!r.active || r.popTicks > 0) continue;

		float dx = mx - r.x, dy = my - r.y;
		// Slightly generous hitbox (+6px) so a click that visually grazes
		// the rock still counts - clicking small moving targets pixel-exact
		// is frustrating, not challenging.
		if (dx * dx + dy * dy <= (r.radius + 6.0f) * (r.radius + 6.0f)) {
			r.popTicks = 10;
			pz3Destroyed++;

			if (pz3Destroyed >= PZ3_TARGET_KILLS) {
				pz3Solved = true;
				grantAK47AndKey();  // Level 2 / Door 1 -> AK47 + Key
			}
			return;
		}
	}

	pz3Misses++;
	if (pz3Misses >= PZ3_MISS_LIMIT) pz3ShutdownTicks = PZ3_SHUTDOWN_TICKS;
}

// --- drawing ---------------------------------------------------------------

inline void pz3DrawRock(const Pz3Rock& r) {
	// Irregular 9-sided lump, radius wobbled per vertex so no two rocks look
	// the same, rotated by its own spin.
	double px[9], py[9];
	for (int i = 0; i < 9; i++) {
		float a = r.spin + (float)i * 0.6981317f; // 2pi/9
		float wob = 0.78f + 0.22f * (float)sin(i * 2.7f + r.radius);
		px[i] = r.x + cos(a) * r.radius * wob;
		py[i] = r.y + sin(a) * r.radius * wob;
	}
	iSetColor(26, 40, 28);
	iFilledPolygon(px, py, 9);
	iSetColor(90, 190, 110);
	iPolygon(px, py, 9);
}

inline void drawPuzzle3Page() {
	if (!pz3Initialized) initPuzzle3();

	// Panel chrome
	iSetColor(10, 14, 12);
	iFilledRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	iSetColor(190, 60, 25);
	iFilledRectangle(PZ3_VIEW_X - 20, PZ3_VIEW_Y + PZ3_VIEW_H + 20, PZ3_VIEW_W + 40, 60);
	iSetColor(255, 255, 255);
	iText(PZ3_VIEW_X + PZ3_VIEW_W / 2 - 90, PZ3_VIEW_Y + PZ3_VIEW_H + 42,
		(char*)"CLEAR ASTEROIDS", GLUT_BITMAP_TIMES_ROMAN_24);

	// Scope body + the translucent green tactical wash over it.
	iSetColor(6, 20, 10);
	iFilledRectangle(PZ3_VIEW_X, PZ3_VIEW_Y, PZ3_VIEW_W, PZ3_VIEW_H);
	fxFilledRectA((float)PZ3_VIEW_X, (float)PZ3_VIEW_Y, (float)PZ3_VIEW_W, (float)PZ3_VIEW_H,
		40, 210, 90, 0.18f + 0.05f * fxPulse());

	// Scan grid
	iSetColor(30, 90, 45);
	for (int gx = PZ3_VIEW_X; gx <= PZ3_VIEW_X + PZ3_VIEW_W; gx += 60)
		iLine(gx, PZ3_VIEW_Y, gx, PZ3_VIEW_Y + PZ3_VIEW_H);
	for (int gy = PZ3_VIEW_Y; gy <= PZ3_VIEW_Y + PZ3_VIEW_H; gy += 60)
		iLine(PZ3_VIEW_X, gy, PZ3_VIEW_X + PZ3_VIEW_W, gy);

	// Sweeping scan line, driven by the puzzle's own tick counter.
	int sweep = PZ3_VIEW_X + (int)((pz3Tick * 3) % PZ3_VIEW_W);
	fxFilledRectA((float)sweep, (float)PZ3_VIEW_Y, 3.0f, (float)PZ3_VIEW_H, 160, 255, 180, 0.35f);

	if (pz3ShutdownTicks > 0) {
		// Overload state: scope is dark, nothing is clickable.
		fxFilledRectA((float)PZ3_VIEW_X, (float)PZ3_VIEW_Y, (float)PZ3_VIEW_W, (float)PZ3_VIEW_H,
			0, 0, 0, 0.72f);
		iSetColor(255, 90, 90);
		iText(PZ3_VIEW_X + PZ3_VIEW_W / 2 - 150, PZ3_VIEW_Y + PZ3_VIEW_H / 2,
			(char*)"TARGETING SCOPE OVERLOADED", GLUT_BITMAP_TIMES_ROMAN_24);
		char back[48];
		sprintf_s(back, sizeof(back), "REBOOTING... %.1fs", pz3ShutdownTicks * 0.03f);
		iText(PZ3_VIEW_X + PZ3_VIEW_W / 2 - 70, PZ3_VIEW_Y + PZ3_VIEW_H / 2 - 40, back, GLUT_BITMAP_HELVETICA_18);
	}
	else {
		for (int i = 0; i < PZ3_MAX_ROCKS; i++) {
			const Pz3Rock& r = pz3Rocks[i];
			if (!r.active) continue;

			if (r.popTicks > 0) {
				// Destruction burst: expanding ring that fades out.
				float t = 1.0f - (r.popTicks / 10.0f);
				fxFilledRectA(r.x - r.radius * (1.0f + t), r.y - 2.0f, r.radius * 2.0f * (1.0f + t), 4.0f,
					255, 230, 120, 0.8f * (1.0f - t));
				iSetColor(255, 220, 120);
				iCircle(r.x, r.y, r.radius * (1.0f + t * 1.6f), 24);
				continue;
			}

			pz3DrawRock(r);

			// Lock-on brackets on the nearest few rocks so the scope reads
			// as a targeting system and not just floating shapes.
			iSetColor(120, 255, 150);
			float b = r.radius + 10.0f;
			iLine(r.x - b, r.y + b, r.x - b + 8, r.y + b);
			iLine(r.x - b, r.y + b, r.x - b, r.y + b - 8);
			iLine(r.x + b, r.y - b, r.x + b - 8, r.y - b);
			iLine(r.x + b, r.y - b, r.x + b, r.y - b + 8);
		}
	}

	// Crosshair follows the cursor inside the scope.
	if (MX >= PZ3_VIEW_X && MX <= PZ3_VIEW_X + PZ3_VIEW_W &&
		MY >= PZ3_VIEW_Y && MY <= PZ3_VIEW_Y + PZ3_VIEW_H) {
		iSetColor(200, 255, 210);
		iLine(MX - 16, MY, MX - 5, MY);
		iLine(MX + 5, MY, MX + 16, MY);
		iLine(MX, MY - 16, MX, MY - 5);
		iLine(MX, MY + 5, MX, MY + 16);
		iCircle(MX, MY, 18, 20);
	}

	// Counters
	char line[64];
	sprintf_s(line, sizeof(line), "Destroyed: %d / %d", pz3Destroyed, PZ3_TARGET_KILLS);
	iSetColor(255, 255, 255);
	iText(PZ3_VIEW_X + 14, PZ3_VIEW_Y + 16, line, GLUT_BITMAP_TIMES_ROMAN_24);

	sprintf_s(line, sizeof(line), "MISSES: %d / %d", pz3Misses, PZ3_MISS_LIMIT);
	iSetColor(pz3Misses >= PZ3_MISS_LIMIT - 1 ? 255 : 160, 120, 120);
	iText(PZ3_VIEW_X + PZ3_VIEW_W - 160, PZ3_VIEW_Y + 16, line, GLUT_BITMAP_HELVETICA_18);

	iSetColor(150, 200, 165);
	iText(PZ3_VIEW_X, PZ3_VIEW_Y - 34,
		(char*)"CLICK DEBRIS TO DESTROY IT   -   [ESC] LEAVE (PROGRESS IS KEPT)", GLUT_BITMAP_HELVETICA_18);
}

#endif