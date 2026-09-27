#ifndef UTILITY_HPP
#define UTILITY_HPP

#include <cstdio>

// Guarded: Variables.h already #defines these as macros, and it's usually
// included before this header (via Hero.hpp -> level.hpp -> iMain.cpp).
// Without the guard, the preprocessor would substitute SCREEN_WIDTH here
// and turn this into "const int 1280 = 1280;" - a compile error.
#ifndef SCREEN_WIDTH
const int SCREEN_WIDTH = 1280;
#endif
#ifndef SCREEN_HEIGHT
const int SCREEN_HEIGHT = 720;
#endif


// same as above, but for folders using "R_1.png", "L_1.png", etc.
inline void loadImage(int img[], int n, const char* path, const char* prefix) {
	for (int i = 0; i < n; i++) {
		char imgSource[100];
		sprintf_s(imgSource, sizeof(imgSource), "%s\\%s_%d.png", path, prefix, i + 1);
		img[i] = iLoadImage(imgSource);
	}
}

// true if the two boxes (bottom-left corner + width/height) overlap
inline bool rectsOverlap(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
	return (x1 < x2 + w2) && (x1 + w1 > x2) && (y1 < y2 + h2) && (y1 + h1 > y2);
}

// how many pixels two boxes overlap sideways (left-right), if we only look
// at their X range (ignoring height/Y completely). This is used together
// with rectsOverlap() above: rectsOverlap() answers "are they touching at
// all?", and this answers "by how much?" - so callers can decide to allow a
// little overlap before treating something as a solid wall.
//
// Example: box A is x=0..100, box B is x=80..150 -> they overlap from
// x=80 to x=100, which is 20 pixels, so this returns 20.
// If the boxes don't overlap at all, this returns 0 or a negative number.
inline int overlapWidth(int x1, int w1, int x2, int w2) {
	int rightEdgeOfA = x1 + w1;
	int rightEdgeOfB = x2 + w2;

	// the overlap starts at whichever left edge is further right...
	int overlapStart = (x1 > x2) ? x1 : x2;
	// ...and ends at whichever right edge is further left.
	int overlapEnd = (rightEdgeOfA < rightEdgeOfB) ? rightEdgeOfA : rightEdgeOfB;

	return overlapEnd - overlapStart;
}

#endif