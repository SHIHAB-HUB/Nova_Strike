// Puzzle1.hpp
//Owner: Mahdin (Map Design)

#ifndef PUZZLE1_HPP
#define PUZZLE1_HPP

#include <cstdlib>
#include "Variables.h"
#include "PuzzleCommon.hpp"

#define PZ_N 1
#define PZ_E 2
#define PZ_S 4
#define PZ_W 8

#define PZ1_ORIGIN_X 460
#define PZ1_ORIGIN_Y 160
#define PZ1_CELL 100


__declspec(selectany) int pz1BaseMask[9] = {
	PZ_W | PZ_E, PZ_W | PZ_E, PZ_W | PZ_S,   // row 0 : (0,0) (0,1) (0,2)
	PZ_E | PZ_S, PZ_E | PZ_W, PZ_N | PZ_W,   // row 1 : (1,0) (1,1) (1,2)
	PZ_N | PZ_E, PZ_W | PZ_E, PZ_W | PZ_E    // row 2 : (2,0) (2,1) (2,2)
};


// is. Randomized on entry so the puzzle is never pre-solved, but always
__declspec(selectany) int pz1Rotation[9];
__declspec(selectany) bool pz1Initialized = false;
__declspec(selectany) bool pz1Solved = false;
__declspec(selectany) bool pz1TileConnected[9]; // or highlighting the live flow path

inline int rotateMaskCW(int mask, int times) {
	for (int i = 0; i < times; i++)
		mask = ((mask << 1) | (mask >> 3)) & 0xF;
	return mask;
}

inline int pz1CurrentMask(int idx) {
	return rotateMaskCW(pz1BaseMask[idx], pz1Rotation[idx]);
}

// IF puzzle connects then return true
inline bool pz1CheckFlow() {
	for (int i = 0; i < 9; i++) pz1TileConnected[i] = false;

	int mask0 = pz1CurrentMask(0);
	if (!(mask0 & PZ_W)) return false; // power never even enters the grid

	bool visited[9] = { false };
	int queue[9], qHead = 0, qTail = 0;
	queue[qTail++] = 0;
	visited[0] = true;

	while (qHead < qTail) {
		int idx = queue[qHead++];
		int row = idx / 3, col = idx % 3;
		int mask = pz1CurrentMask(idx);

		// North neighbor
		if ((mask & PZ_N) && row > 0) {
			int nIdx = (row - 1) * 3 + col;
			if (!visited[nIdx] && (pz1CurrentMask(nIdx) & PZ_S)) { visited[nIdx] = true; queue[qTail++] = nIdx; }
		}
		// South neighbor
		if ((mask & PZ_S) && row < 2) {
			int nIdx = (row + 1) * 3 + col;
			if (!visited[nIdx] && (pz1CurrentMask(nIdx) & PZ_N)) { visited[nIdx] = true; queue[qTail++] = nIdx; }
		}
		// East neighbor
		if ((mask & PZ_E) && col < 2) {
			int nIdx = row * 3 + (col + 1);
			if (!visited[nIdx] && (pz1CurrentMask(nIdx) & PZ_W)) { visited[nIdx] = true; queue[qTail++] = nIdx; }
		}
		// West neighbor
		if ((mask & PZ_W) && col > 0) {
			int nIdx = row * 3 + (col - 1);
			if (!visited[nIdx] && (pz1CurrentMask(nIdx) & PZ_E)) { visited[nIdx] = true; queue[qTail++] = nIdx; }
		}
	}

	for (int i = 0; i < 9; i++) pz1TileConnected[i] = visited[i];
	return visited[8] && (pz1CurrentMask(8) & PZ_E);
}

inline void initPuzzle1() {
	for (int i = 0; i < 9; i++) {
		// 1..3 so it never spawns already solved (0 would be solved).
		pz1Rotation[i] = 1 + (rand() % 3);
	}
	pz1Solved = false;
	pz1Initialized = true;
}

// Call from iMouse() when currentPage == PAGE_PUZZLE1 and the left button
// was just pressed. Rotates whichever tile the click landed on.
inline void handlePuzzle1Click(int mx, int my) {
	if (pz1Solved) return;

	for (int i = 0; i < 9; i++) {
		int row = i / 3, col = i % 3;
		int cellX = PZ1_ORIGIN_X + col * PZ1_CELL;
		int cellY = PZ1_ORIGIN_Y + (2 - row) * PZ1_CELL; // row 0 drawn at the top

		if (mx >= cellX && mx <= cellX + PZ1_CELL && my >= cellY && my <= cellY + PZ1_CELL) {
			pz1Rotation[i] = (pz1Rotation[i] + 1) % 4;
			if (pz1CheckFlow()) {
				pz1Solved = true;
				grantPistolAndKey(); // Level 1 / Puzzle 1 -> Pistol + Key
			}
			return;
		}
	}
}

inline void drawPuzzle1Page() {
	if (!pz1Initialized) initPuzzle1();

	iSetColor(8, 10, 18);
	iFilledRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	iSetColor(0, 220, 255);
	iText(PZ1_ORIGIN_X - 60, 620, (char*)"[ SECURITY TERMINAL - OVERRIDE ]", GLUT_BITMAP_TIMES_ROMAN_24);

	bool flowing = pz1CheckFlow();

	// Power source label (west of the grid)
	iSetColor(255, 255, 255);
	iText(PZ1_ORIGIN_X - 150, PZ1_ORIGIN_Y + PZ1_CELL * 2 + 30, (char*)"POWER", GLUT_BITMAP_HELVETICA_18);
	// Door lock label (east of the grid)
	iText(PZ1_ORIGIN_X + PZ1_CELL * 3 + 20, PZ1_ORIGIN_Y + 30, (char*)"DOOR LOCK", GLUT_BITMAP_HELVETICA_18);

	for (int i = 0; i < 9; i++) {
		int row = i / 3, col = i % 3;
		int cellX = PZ1_ORIGIN_X + col * PZ1_CELL;
		int cellY = PZ1_ORIGIN_Y + (2 - row) * PZ1_CELL;
		int cx = cellX + PZ1_CELL / 2;
		int cy = cellY + PZ1_CELL / 2;

		// Tile background + border
		iSetColor(20, 24, 34);
		iFilledRectangle(cellX + 4, cellY + 4, PZ1_CELL - 8, PZ1_CELL - 8);
		iSetColor(60, 70, 90);
		iRectangle(cellX + 4, cellY + 4, PZ1_CELL - 8, PZ1_CELL - 8);

		int mask = pz1CurrentMask(i);
		bool live = pz1TileConnected[i];
		if (live) iSetColor(0, 255, 200);  // energized - neon cyan/green
		else      iSetColor(90, 90, 95);   // dead - dim gray

		int half = PZ1_CELL / 2 - 6;
		if (mask & PZ_N) iLine(cx, cy, cx, cy + half);
		if (mask & PZ_S) iLine(cx, cy, cx, cy - half);
		if (mask & PZ_E) iLine(cx, cy, cx + half, cy);
		if (mask & PZ_W) iLine(cx, cy, cx - half, cy);
	}

	iSetColor(255, 255, 255);
	if (flowing)
		iText(PZ1_ORIGIN_X, 100, (char*)"STATUS: POWER CONNECTED", GLUT_BITMAP_HELVETICA_18);
	else
		iText(PZ1_ORIGIN_X, 100, (char*)"STATUS: POWER DISCONNECTED - CLICK A TILE TO ROTATE", GLUT_BITMAP_HELVETICA_18);
}

#endif