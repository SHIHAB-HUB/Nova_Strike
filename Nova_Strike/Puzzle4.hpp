// =====================================================================
// Puzzle4.hpp - "FIX WEATHER NODE"  (Level 2, door 2 -> Key + Health)
//
// Industrial control panel holding a grid maze. Press and HOLD on the lit
// start node (top-left), drag a continuous path through the open cells, and
// release on the destination node (bottom-right). Let go early, touch a
// wall, or double back into a dead end and the node resets to start.
//
// Two details that make the drag feel right instead of fighting the player:
//  * Mouse motion arrives in jumps, not cell by cell. pz4TraceTo() walks the
//    straight line between the last cell and the new one and enters each
//    cell along the way, so a fast drag can't skip over a wall.
//  * Backtracking is allowed: dragging back onto the previous cell pops the
//    path instead of failing, the way a pencil-on-paper maze would work.
// =====================================================================
#ifndef PUZZLE4_HPP
#define PUZZLE4_HPP

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "Variables.h"
#include "PuzzleCommon.hpp"

#define PZ4_COLS 15
#define PZ4_ROWS 9
#define PZ4_CELL 46
#define PZ4_ORIGIN_X 295   // left edge of the maze area
#define PZ4_ORIGIN_Y 190   // bottom edge of the maze area
#define PZ4_MAX_PATH (PZ4_COLS * PZ4_ROWS)

// '#' = wall / circuit block, '.' = open channel.
// Row 0 is the TOP row on screen. Start = (row 0, col 0),
// goal = (row PZ4_ROWS-1, col PZ4_COLS-1).
//
// RANDOMIZED: the maze used to be a fixed hand-drawn layout, so a player
// who memorised one route could clear it instantly every replay. It is now
// carved fresh on every entry by randomised DFS (recursive backtracker),
// which by construction produces a PERFECT maze - exactly one path between
// any two cells - so a start-to-goal route is always guaranteed to exist.
// No solvability check is needed; the algorithm cannot produce an
// unsolvable grid.
__declspec(selectany) char pz4MazeBuf[PZ4_ROWS][PZ4_COLS + 1];
__declspec(selectany) const char* pz4Maze[PZ4_ROWS];

__declspec(selectany) bool pz4Solved = false;
__declspec(selectany) bool pz4Dragging = false;
__declspec(selectany) int pz4PathR[PZ4_MAX_PATH];
__declspec(selectany) int pz4PathC[PZ4_MAX_PATH];
__declspec(selectany) int pz4PathLen = 0;
__declspec(selectany) int pz4FailFlashTicks = 0;
__declspec(selectany) int pz4Tick = 0;

inline bool pz4IsOpen(int r, int c) {
	if (r < 0 || r >= PZ4_ROWS || c < 0 || c >= PZ4_COLS) return false;
	return pz4Maze[r][c] == '.';
}

// Screen rect of a cell. Row 0 is drawn at the TOP, so the y axis flips.
inline void pz4CellRect(int r, int c, float& x, float& y) {
	x = (float)(PZ4_ORIGIN_X + c * PZ4_CELL);
	y = (float)(PZ4_ORIGIN_Y + (PZ4_ROWS - 1 - r) * PZ4_CELL);
}

inline bool pz4PointToCell(int mx, int my, int& r, int& c) {
	int cc = (mx - PZ4_ORIGIN_X) / PZ4_CELL;
	int rr = (PZ4_ROWS - 1) - (my - PZ4_ORIGIN_Y) / PZ4_CELL;
	if (mx < PZ4_ORIGIN_X || my < PZ4_ORIGIN_Y) return false;
	if (cc < 0 || cc >= PZ4_COLS || rr < 0 || rr >= PZ4_ROWS) return false;
	r = rr; c = cc;
	return true;
}

inline bool pz4InPath(int r, int c) {
	for (int i = 0; i < pz4PathLen; i++)
	if (pz4PathR[i] == r && pz4PathC[i] == c) return true;
	return false;
}

inline void pz4Reset() {
	pz4PathLen = 0;
	pz4Dragging = false;
}

inline void pz4Fail() {
	pz4Reset();
	pz4FailFlashTicks = 24; // red pulse on the panel so the reset is visible
}

// Randomised DFS carve. Works on the odd-indexed lattice (cells at even
// row/col, walls between them), which is what keeps corridors exactly one
// tile wide instead of opening into rooms.
inline void pz4GenerateMaze() {
	for (int r = 0; r < PZ4_ROWS; r++) {
		for (int c = 0; c < PZ4_COLS; c++) pz4MazeBuf[r][c] = '#';
		pz4MazeBuf[r][PZ4_COLS] = '\0';
		pz4Maze[r] = pz4MazeBuf[r];
	}

	int stackR[PZ4_MAX_PATH], stackC[PZ4_MAX_PATH], top = 0;
	stackR[top] = 0; stackC[top] = 0; top++;
	pz4MazeBuf[0][0] = '.';

	while (top > 0) {
		int r = stackR[top - 1], c = stackC[top - 1];

		// Candidate neighbours two cells away (the cell between them is the
		// wall that gets knocked out).
		int nr[4], nc[4], n = 0;
		if (r >= 2 && pz4MazeBuf[r - 2][c] == '#') { nr[n] = r - 2; nc[n] = c; n++; }
		if (r + 2 < PZ4_ROWS    && pz4MazeBuf[r + 2][c] == '#') { nr[n] = r + 2; nc[n] = c; n++; }
		if (c >= 2 && pz4MazeBuf[r][c - 2] == '#') { nr[n] = r; nc[n] = c - 2; n++; }
		if (c + 2 < PZ4_COLS    && pz4MazeBuf[r][c + 2] == '#') { nr[n] = r; nc[n] = c + 2; n++; }

		if (n == 0) { top--; continue; }   // dead end - backtrack

		int pick = rand() % n;
		int tr = nr[pick], tc = nc[pick];
		pz4MazeBuf[(r + tr) / 2][(c + tc) / 2] = '.';  // knock out the wall
		pz4MazeBuf[tr][tc] = '.';
		stackR[top] = tr; stackC[top] = tc; top++;
	}

	// PZ4_ROWS/COLS are odd (9 x 15), so the goal corner lands on the
	// lattice and is always carved. Forced open anyway as a belt-and-braces
	// guard in case those dimensions are ever changed to even numbers.
	pz4MazeBuf[0][0] = '.';
	pz4MazeBuf[PZ4_ROWS - 1][PZ4_COLS - 1] = '.';
	if (PZ4_ROWS % 2 == 0) pz4MazeBuf[PZ4_ROWS - 2][PZ4_COLS - 1] = '.';
	if (PZ4_COLS % 2 == 0) pz4MazeBuf[PZ4_ROWS - 1][PZ4_COLS - 2] = '.';
}

inline void resetPuzzle4() {
	pz4Solved = false;
	pz4FailFlashTicks = 0;
	pz4Tick = 0;
	pz4GenerateMaze();   // fresh layout every entry
	pz4Reset();
}

// Enter ONE cell that is 4-neighbour adjacent to the current head.
inline void pz4StepTo(int r, int c) {
	if (pz4PathLen == 0) return;

	int hr = pz4PathR[pz4PathLen - 1], hc = pz4PathC[pz4PathLen - 1];
	if (r == hr && c == hc) return;

	int d = abs(r - hr) + abs(c - hc);
	if (d != 1) return;              // not adjacent - ignore, never a fail

	// Dragging back onto the previous cell rubs the path out, like a pencil.
	if (pz4PathLen >= 2 && pz4PathR[pz4PathLen - 2] == r && pz4PathC[pz4PathLen - 2] == c) {
		pz4PathLen--;
		return;
	}

	if (!pz4IsOpen(r, c)) { pz4Fail(); return; }   // ran into a wall
	if (pz4InPath(r, c)) { pz4Fail(); return; }    // crossed our own trail

	if (pz4PathLen < PZ4_MAX_PATH) {
		pz4PathR[pz4PathLen] = r;
		pz4PathC[pz4PathLen] = c;
		pz4PathLen++;
	}

	// Reached the destination node - the drag only has to touch it.
	if (r == PZ4_ROWS - 1 && c == PZ4_COLS - 1) {
		pz4Solved = true;
		pz4Dragging = false;
		grantFlamethrowerAndKey();  // Level 2 / Door 2 -> Flamethrower [4] + Key
	}
}

// Walk cell-by-cell from the current head toward (r,c). Mouse events can jump
// several cells at once; stepping the difference one axis at a time means a
// fast drag still has to pass through - and can still be stopped by - every
// wall in between.
inline void pz4TraceTo(int r, int c) {
	int guard = 0;
	while (pz4PathLen > 0 && guard++ < 200) {
		int hr = pz4PathR[pz4PathLen - 1], hc = pz4PathC[pz4PathLen - 1];
		if (hr == r && hc == c) return;
		if (pz4Solved || pz4PathLen == 0) return;

		int nr = hr, nc = hc;
		if (hr != r)      nr += (r > hr) ? 1 : -1;
		else if (hc != c) nc += (c > hc) ? 1 : -1;

		int before = pz4PathLen;
		pz4StepTo(nr, nc);
		if (pz4PathLen == 0) return;         // pz4Fail() wiped the path
		if (pz4PathLen == before && !(pz4PathR[pz4PathLen - 1] == nr && pz4PathC[pz4PathLen - 1] == nc))
			return;                          // step refused, stop tracing
	}
}

// --- input hooks (wired up in iMain.cpp) -----------------------------------

inline void handlePuzzle4Press(int mx, int my) {
	if (pz4Solved) return;
	int r, c;
	if (!pz4PointToCell(mx, my, r, c)) return;

	// A drag is only valid if it BEGINS on the start node.
	if (r == 0 && c == 0) {
		pz4PathLen = 0;
		pz4PathR[pz4PathLen] = 0;
		pz4PathC[pz4PathLen] = 0;
		pz4PathLen = 1;
		pz4Dragging = true;
	}
	else {
		pz4Fail();
	}
}

inline void handlePuzzle4Drag(int mx, int my) {
	if (!pz4Dragging || pz4Solved) return;
	int r, c;
	if (!pz4PointToCell(mx, my, r, c)) return;
	pz4TraceTo(r, c);
}

inline void handlePuzzle4Release(int mx, int my) {
	if (pz4Solved || !pz4Dragging) return;
	// Released without reaching the destination - node snaps back to start.
	pz4Fail();
}

inline void updatePuzzle4() {
	pz4Tick++;
	if (pz4FailFlashTicks > 0) pz4FailFlashTicks--;
}

// --- drawing ---------------------------------------------------------------

inline void drawPuzzle4Page() {
	float mazeW = (float)(PZ4_COLS * PZ4_CELL);
	float mazeH = (float)(PZ4_ROWS * PZ4_CELL);

	iSetColor(9, 12, 20);
	iFilledRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	// Title bar, same orange plate as the other terminal screens.
	iSetColor(190, 60, 25);
	iFilledRectangle(PZ4_ORIGIN_X - 40, PZ4_ORIGIN_Y + mazeH + 70, mazeW + 80, 60);
	iSetColor(255, 255, 255);
	iText(PZ4_ORIGIN_X + mazeW / 2 - 105, PZ4_ORIGIN_Y + mazeH + 92,
		(char*)"FIX WEATHER NODE", GLUT_BITMAP_TIMES_ROMAN_24);

	// Console housing around the maze.
	iSetColor(34, 54, 92);
	iFilledRectangle(PZ4_ORIGIN_X - 40, PZ4_ORIGIN_Y - 40, mazeW + 80, mazeH + 80);
	iSetColor(20, 32, 58);
	iFilledRectangle(PZ4_ORIGIN_X - 22, PZ4_ORIGIN_Y - 22, mazeW + 44, mazeH + 44);
	iSetColor(70, 120, 190);
	iRectangle(PZ4_ORIGIN_X - 22, PZ4_ORIGIN_Y - 22, mazeW + 44, mazeH + 44);

	// Screen bed
	iSetColor(150, 142, 120);
	iFilledRectangle(PZ4_ORIGIN_X, PZ4_ORIGIN_Y, mazeW, mazeH);

	// Cells
	for (int r = 0; r < PZ4_ROWS; r++) {
		for (int c = 0; c < PZ4_COLS; c++) {
			float x, y;
			pz4CellRect(r, c, x, y);
			if (pz4IsOpen(r, c)) {
				iSetColor(178, 170, 148);
				iFilledRectangle(x + 1, y + 1, PZ4_CELL - 2, PZ4_CELL - 2);
			}
			else {
				iSetColor(96, 92, 80);
				iFilledRectangle(x + 3, y + 3, PZ4_CELL - 6, PZ4_CELL - 6);
				iSetColor(70, 66, 58);
				iRectangle(x + 3, y + 3, PZ4_CELL - 6, PZ4_CELL - 6);
			}
		}
	}

	// Traced path - a glowing cyan channel that breathes.
	float glow = 0.55f + 0.25f * fxPulse();
	for (int i = 0; i < pz4PathLen; i++) {
		float x, y;
		pz4CellRect(pz4PathR[i], pz4PathC[i], x, y);
		fxFilledRectA(x + 4, y + 4, (float)PZ4_CELL - 8, (float)PZ4_CELL - 8, 60, 220, 255, glow);
	}

	// Start node (top-left) and destination node (bottom-right).
	float sx, sy, gx, gy;
	pz4CellRect(0, 0, sx, sy);
	pz4CellRect(PZ4_ROWS - 1, PZ4_COLS - 1, gx, gy);

	iSetColor(255, 170, 60);
	iFilledCircle(sx + PZ4_CELL / 2, sy + PZ4_CELL / 2, 13, 24);
	fxFilledRectA(sx + 4, sy + 4, (float)PZ4_CELL - 8, (float)PZ4_CELL - 8, 255, 200, 90, 0.25f + 0.25f * fxPulse());

	iSetColor(pz4Solved ? 120 : 200, 255, 200);
	iFilledCircle(gx + PZ4_CELL / 2, gy + PZ4_CELL / 2, 13, 24);
	fxFilledRectA(gx + 4, gy + 4, (float)PZ4_CELL - 8, (float)PZ4_CELL - 8, 120, 255, 170, 0.25f + 0.25f * fxPulse());

	// Reset flash
	if (pz4FailFlashTicks > 0) {
		float a = 0.35f * (pz4FailFlashTicks / 24.0f);
		fxFilledRectA(PZ4_ORIGIN_X, PZ4_ORIGIN_Y, mazeW, mazeH, 255, 40, 40, a);
	}

	// Status line
	iSetColor(150, 190, 235);
	if (pz4Solved)
		iText(PZ4_ORIGIN_X, PZ4_ORIGIN_Y - 60, (char*)"NODE REPAIRED - WEATHER CONTROL ONLINE", GLUT_BITMAP_HELVETICA_18);
	else if (pz4Dragging)
		iText(PZ4_ORIGIN_X, PZ4_ORIGIN_Y - 60, (char*)"HOLD AND KEEP DRAGGING - DO NOT RELEASE", GLUT_BITMAP_HELVETICA_18);
	else if (pz4FailFlashTicks > 0)
		iText(PZ4_ORIGIN_X, PZ4_ORIGIN_Y - 60, (char*)"CIRCUIT BROKEN - NODE RESET TO START", GLUT_BITMAP_HELVETICA_18);
	else
		iText(PZ4_ORIGIN_X, PZ4_ORIGIN_Y - 60,
		(char*)"PRESS AND HOLD ON THE ORANGE NODE, DRAG TO THE GREEN NODE   -   [ESC] LEAVE", GLUT_BITMAP_HELVETICA_18);
}

#endif