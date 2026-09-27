#pragma once

// Everything about actually SHOWING an enemy's animation: loading the
// sprite images, advancing which frame is current, and drawing the right
// frame to the screen. This file doesn't decide WHY an enemy is walking,
// chasing, or attacking - enemy.h's updateEnemies() owns that state
// machine. Here we just react to whatever type/state/frame/direction an
// enemy currently has and show the right picture for it.
//
// NOT includable on its own: it uses struct Enemy, enum EnemyState,
// enemyList, and MAX_ENEMIES without declaring any of them itself, on the
// assumption enemy.h has already defined all four before pulling this file
// in. That's the same "include order carries shared context" convention
// enemy.h itself relies on for level.hpp's TILE_SIZE/cameraX/cameraY (see
// enemy.h's own top-of-file comment) - so enemy.h is the only file that
// should ever #include this one, and only after those four are declared.
//
// BEGINNER NOTE: this file avoids the "a ? b : c" (ternary) shorthand on
// purpose and spells everything out as plain if/else instead. It reads a
// bit longer, but every line does exactly one thing and there's nothing
// hidden inside a single expression.

#include "iGraphics.h"
#include "enemy_properties.h"   // per-type frame counts, delays, sprite paths/sizes
#include <ctime>                // clock(), CLOCKS_PER_SEC - for nowMs() below

// Every loaded sprite picture lives in one of these 4 arrays, indexed by
// [enemy type][frame number]:
//   leftPic / rightPic             - the "just standing/idle" picture(s)
//   attackLeftPic / attackRightPic - the attack-swing picture(s)
// "left"/"right" just means which way the enemy is facing.
//
// attackLeftPic/attackRightPic rows for a type with no attack art yet
// (ENEMY_ATTACK_FRAME_COUNT[type] == 0, e.g. enemy_4) are simply never
// written to by the loading loop below and stay at 0.
static unsigned int leftPic[ENEMY_TYPE_COUNT][MAX_FRAMES];
static unsigned int rightPic[ENEMY_TYPE_COUNT][MAX_FRAMES];
static unsigned int attackLeftPic[ENEMY_TYPE_COUNT][MAX_FRAMES];
static unsigned int attackRightPic[ENEMY_TYPE_COUNT][MAX_FRAMES];
static bool picturesLoaded = false;

// Loads every idle/walk/fly frame and every attack frame, for every enemy
// type. Only needs to run once at startup - if something calls this again
// by mistake, picturesLoaded stops it from re-loading everything.
void loadEnemyPictures()
{
    if (picturesLoaded == true)
    {
        return;   // already loaded - nothing to do
    }

    for (int type = 0; type < ENEMY_TYPE_COUNT; type++)
    {
        // idle/walk/fly frames for this type
        int howManyIdleFrames = ENEMY_FRAME_COUNT[type];
        for (int frame = 0; frame < howManyIdleFrames; frame++)
        {
            leftPic[type][frame]  = iLoadImage((char*)LEFT_PIC_PATH[type][frame]);
            rightPic[type][frame] = iLoadImage((char*)RIGHT_PIC_PATH[type][frame]);
        }

        // attack frames for this type (0 frames = loop just doesn't run)
        int howManyAttackFrames = ENEMY_ATTACK_FRAME_COUNT[type];
        for (int frame = 0; frame < howManyAttackFrames; frame++)
        {
            attackLeftPic[type][frame]  = iLoadImage((char*)ATTACK_LEFT_PIC_PATH[type][frame]);
            attackRightPic[type][frame] = iLoadImage((char*)ATTACK_RIGHT_PIC_PATH[type][frame]);
        }
    }

    picturesLoaded = true;
}

// Current time in milliseconds, for frame-rate-independent animation. Lives
// here rather than in enemy.h because it was originally added to pace
// animation frames - but enemy.h's own real-time movement timers
// (walkStartMs, hitFlashUntilMs, etc.) are stamped from this same clock, so
// both files stay in sync by construction.
static long nowMs()
{
    return (long)(clock() * 1000L / CLOCKS_PER_SEC);
}

// Moves one enemy's animation forward by one frame, but only once enough
// real time has passed - this is what makes animation speed the same on a
// fast PC and a slow PC, instead of depending on how many times per second
// this function happens to get called.
//
// Does NOT reset frame/lastFrameChangeMs when the enemy's state changes -
// enemy.h's updateEnemies() already does that itself the moment the state
// actually changes (see the BUGFIX comment there). This function only ever
// moves forward whatever state is currently playing.
void advanceEnemyAnimation(Enemy& e, long now)
{
    // Step 1: are we currently attacking, or just idling/walking? The two
    // use different frame delays and different frame counts (see step 2/3),
    // so we need to know which set of numbers to use before doing anything else.
    bool isAttacking = false;
    if (e.state == ENEMY_STATE_ATTACK)
    {
        isAttacking = true;
    }

    // Step 2: how long should the CURRENT frame stay on screen before we
    // move to the next one? Attack clips are short, frame-dense swings, so
    // they get their own (usually faster) delay instead of inheriting the
    // walk cycle's slower one - using the walk delay for an attack made the
    // swing look like a slow slideshow instead of one smooth motion.
    int frameDelay;
    if (isAttacking == true)
    {
        frameDelay = ENEMY_ATTACK_ANIM_DELAY_MS[e.type];
    }
    else
    {
        frameDelay = ENEMY_ANIM_DELAY_MS[e.type];
    }

    // Step 3: has enough time passed since the frame last changed? If not,
    // do nothing this call - just keep showing the same frame a bit longer.
    long timeSinceLastFrame = now - e.lastFrameChangeMs;
    if (timeSinceLastFrame < frameDelay)
    {
        return;
    }

    // Enough time has passed - advance to the next frame.
    e.lastFrameChangeMs = now;

    int totalFrames;
    if (isAttacking == true)
    {
        totalFrames = ENEMY_ATTACK_FRAME_COUNT[e.type];
    }
    else
    {
        totalFrames = ENEMY_FRAME_COUNT[e.type];
    }

    e.frame = e.frame + 1;
    if (e.frame >= totalFrames)
    {
        e.frame = 0;   // reached the last frame - loop back to the first one
    }

    // BUGFIX: an enemy that stays in ENEMY_STATE_ATTACK across multiple
    // swings (e.g. the hero just stands there taking hits) never gets a
    // state CHANGE to reset hasHitHero on - updateEnemies() only clears it
    // there. So once frame 0 wrapped back around after the first swing's
    // hit landed, hasHitHero stayed true forever and every swing after the
    // first silently dealt 0 damage. Wrapping back to frame 0 is exactly "a
    // new swing just started", so clear it here too.
    if (isAttacking == true && e.frame == 0)
    {
        e.hasHitHero = false;
    }
}

// Draws each active enemy in enemyList at (world x - cameraX, world y -
// cameraY) so enemies scroll with the level the same way the tile map and
// hero do (see level.hpp's drawGameLevel()). cameraX/cameraY are declared
// in level.hpp and enemyList/MAX_ENEMIES in enemy.h - all three are already
// in scope by the time enemy.h includes this file (see the top-of-file
// note above).
void drawEnemies()
{
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (enemyList[i].active == false)
        {
            continue;   // empty slot - nothing to draw
        }

        // hit-flash: the enemy just took a hit (see damageEnemiesInBox() in
        // enemy.h), so skip drawing it for a split second as a visible
        // "that landed" flash. It's still alive and updating underneath,
        // just invisible for a moment.
        if (nowMs() < enemyList[i].hitFlashUntilMs)
        {
            continue;
        }

        int  type  = enemyList[i].type;
        int  frame = enemyList[i].frame;

        bool isAttacking = false;
        if (enemyList[i].state == ENEMY_STATE_ATTACK)
        {
            isAttacking = true;
        }

        bool facingLeft = false;
        if (enemyList[i].direction == -1)
        {
            facingLeft = true;
        }

        // Step 1: pick which picture to show - attack art or idle/walk art,
        // and left-facing or right-facing.
        unsigned int pictureToShow;
        if (isAttacking == true)
        {
            if (facingLeft == true)
            {
                pictureToShow = attackLeftPic[type][frame];
            }
            else
            {
                pictureToShow = attackRightPic[type][frame];
            }
        }
        else
        {
            if (facingLeft == true)
            {
                pictureToShow = leftPic[type][frame];
            }
            else
            {
                pictureToShow = rightPic[type][frame];
            }
        }

        // Step 2: pick how big to draw it. Attack frames can use a bigger
        // box than the normal walk box (see enemy_properties.h -
        // ENEMY_ATTACK_WIDTH/HEIGHT), for types whose attack art needs more
        // canvas than their small standing pose - e.g. a weapon swing
        // reaching outward.
        int drawWidth;
        int drawHeight;
        if (isAttacking == true)
        {
            drawWidth  = ENEMY_ATTACK_WIDTH[type];
            drawHeight = ENEMY_ATTACK_HEIGHT[type];
        }
        else
        {
            drawWidth  = ENEMY_TYPE_WIDTH[type];
            drawHeight = ENEMY_TYPE_HEIGHT[type];
        }

        // Step 3: work out where to draw it. iShowImage draws from the
        // BOTTOM-LEFT corner upward, and the enemy's feet are anchored at
        // enemyList[i].y - that doesn't change here. But if the attack box
        // is wider than the normal walk box, drawing it from the same left
        // edge would make the sprite grow only to the right. Shifting drawX
        // left by half of the extra width instead makes it grow outward
        // from its own center, which looks more natural.
        int extraWidth = drawWidth - ENEMY_TYPE_WIDTH[type];
        int drawX = enemyList[i].x - (extraWidth / 2);

        iShowImage(drawX - (int)cameraX, enemyList[i].y - (int)cameraY,
                   drawWidth, drawHeight,
                   pictureToShow);
    }
}
