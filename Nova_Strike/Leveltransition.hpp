// =====================================================
// LevelTransition.hpp
// Owner: Mahdin (Map Design)
//
// Simple fade-to-black-and-back transition, used when moving from Level 1
// into Level 2 (or any future level swap) so the map/asset load doesn't
// happen as a jarring instant cut.
//
// Flow: startLevel2Transition() -> TRANS_FADE_OUT (alpha climbs 0->1) ->
// at full black, the actual initLevel2() swap happens once -> TRANS_FADE_IN
// (alpha falls 1->0) -> TRANS_NONE (normal gameplay, input re-enabled).
// =====================================================
#ifndef LEVELTRANSITION_HPP
#define LEVELTRANSITION_HPP

#include "Variables.h"
#include "SoundManager.hpp"
#include "level.hpp"    // needs initLevel2()
#include "GameFlow.hpp" // needs showLevelTitle()
// (original note) needs initLevel2() (Level2.hpp is included at the bottom of level.hpp)

enum TransitionState {
	TRANS_NONE,
	TRANS_FADE_OUT,
	TRANS_FADE_IN
};

__declspec(selectany) TransitionState levelTransitionState = TRANS_NONE;
__declspec(selectany) float transitionAlpha = 0.0f;
__declspec(selectany) bool transitionSwapDone = false;

#define TRANSITION_SPEED 0.04f // alpha change per tick - ~25 ticks (~0.75s) each way at 30ms/tick

// true while a fade is in progress - PlayerController.hpp uses this to
// freeze movement/input so the hero doesn't act mid-transition.
inline bool isLevelTransitioning() {
	return levelTransitionState != TRANS_NONE;
}

// Call this instead of initLevel2() directly (e.g. from the "Q near the
// door" handler in PlayerController.hpp) to fade out first.
inline void startLevel2Transition() {
	if (levelTransitionState != TRANS_NONE) return; // already transitioning, ignore
	levelTransitionState = TRANS_FADE_OUT;
	transitionAlpha = 0.0f;
	transitionSwapDone = false;
}

// Call once per tick from fixedUpdate(), regardless of currentPage.
inline void updateLevelTransition() {
	if (levelTransitionState == TRANS_NONE) return;

	if (levelTransitionState == TRANS_FADE_OUT) {
		transitionAlpha += TRANSITION_SPEED;
		if (transitionAlpha >= 1.0f) {
			transitionAlpha = 1.0f;

			// Screen is fully black now - safe to swap the level underneath
			// without the player ever seeing the load happen.
			if (!transitionSwapDone) {
				initLevel2();
				showLevelTitle(2);   // "VOID INCURSION" card + bio-hazard banner
				// Level 2 has its own track - swapped here, under full black,
				// so the music change lands with the level change.
				stopBGM();
				playLevel2BGM();
				transitionSwapDone = true;
			}

			levelTransitionState = TRANS_FADE_IN;
		}
	}
	else if (levelTransitionState == TRANS_FADE_IN) {
		transitionAlpha -= TRANSITION_SPEED;
		if (transitionAlpha <= 0.0f) {
			transitionAlpha = 0.0f;
			levelTransitionState = TRANS_NONE;
		}
	}
}

// Call from iDraw(), AFTER the normal page/gameplay drawing, so the fade
// overlay sits on top of everything else.
//
// NOTE: iGraphics (v4.0) has no built-in alpha/transparency helper and
// never calls glEnable(GL_BLEND) itself, so a plain iSetColor+
// iFilledRectangle would just paint solid black regardless of alpha. We
// drop to raw OpenGL here (glut.h already pulls it in via iGraphics.h)
// to actually blend, then restore the default blend state afterward.
inline void drawLevelTransitionOverlay() {
	if (levelTransitionState == TRANS_NONE || transitionAlpha <= 0.0f) return;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glColor4f(0.0f, 0.0f, 0.0f, transitionAlpha);
	glBegin(GL_QUADS);
	glVertex2f(0, 0);
	glVertex2f(SCREEN_WIDTH, 0);
	glVertex2f(SCREEN_WIDTH, SCREEN_HEIGHT);
	glVertex2f(0, SCREEN_HEIGHT);
	glEnd();

	glDisable(GL_BLEND);
}

#endif