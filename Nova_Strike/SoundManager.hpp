#ifndef SOUND_MANAGER_HPP
#define SOUND_MANAGER_HPP

// Owner: [Audio / Assets teammate]

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <string.h>
#include "Variables.h"
#pragma comment(lib, "winmm.lib")

// =====================================================================
// LEVEL 3 BGM - level3.mp3 played through MCI
//
// PlaySound() (which every other track in this file uses) is WAV-ONLY. It
// silently does nothing with an .mp3, which is why simply pointing it at
// Audios\level3.mp3 would leave the Titan arena dead quiet. So the Level 3
// theme goes through the SAME MCI pipe the weapon SFX already use: the file
// is opened once under an alias, then played with the "repeat" flag so it
// loops for the whole boss fight.
//
// A watchdog (tickLevel3Track) re-arms the track if the driver ever drops
// the repeat flag, so the arena can never fall silent mid-fight.
// =====================================================================
#define BGM_L3_ALIAS "bgmTitan"
#define BGM_L3_PATH  "Audios\\level3.mp3"
#define BGM_L3_WATCHDOG_TICKS 40      // ~1.2s between "is it still playing?" checks

__declspec(selectany) bool bgmL3Opened = false;   // MCI handle created?
__declspec(selectany) bool bgmL3Available = false;// file exists AND opened cleanly
__declspec(selectany) bool bgmL3Playing = false;  // should it be looping right now?
__declspec(selectany) int  bgmL3WatchTicks = 0;

// MCI volume is a 0..1000 scale; masterVolume is 0..100.
inline void bgmL3ApplyVolume() {
	if (!bgmL3Available) return;
	int v = isMusicMuted ? 0 : masterVolume * 10;
	if (v > 1000) v = 1000;
	if (v < 0)    v = 0;
	char cmd[160];
	sprintf_s(cmd, sizeof(cmd), "setaudio " BGM_L3_ALIAS " volume to %d", v);
	mciSendStringA(cmd, NULL, 0, NULL);
}

// Opened once for the whole process. Probing the file first is the same
// guard used for every optional asset in this project - MCI reports success
// for some missing files, so the probe is what makes the fallback real.
inline void openLevel3Track() {
	if (bgmL3Opened) return;
	bgmL3Opened = true;

	FILE* probe = NULL;
	fopen_s(&probe, BGM_L3_PATH, "rb");
	if (!probe) return;                 // file missing - stay unavailable
	fclose(probe);

	mciSendStringA("open \"" BGM_L3_PATH "\" type mpegvideo alias " BGM_L3_ALIAS,
		NULL, 0, NULL);

	// Confirm the device actually exists before trusting it.
	char buf[64] = "";
	mciSendStringA("status " BGM_L3_ALIAS " mode", buf, sizeof(buf), NULL);
	bgmL3Available = (buf[0] != '\0');

	bgmL3ApplyVolume();
}

inline void stopLevel3Track() {
	bgmL3Playing = false;
	if (!bgmL3Available) return;
	mciSendStringA("stop " BGM_L3_ALIAS, NULL, 0, NULL);
	mciSendStringA("seek " BGM_L3_ALIAS " to start", NULL, 0, NULL);
}

// Returns false if level3.mp3 could not be used, so the caller can fall
// back to a WAV track instead of leaving the arena silent.
inline bool startLevel3Track() {
	openLevel3Track();
	if (!bgmL3Available) return false;

	bgmL3ApplyVolume();
	mciSendStringA("seek " BGM_L3_ALIAS " to start", NULL, 0, NULL);
	mciSendStringA("play " BGM_L3_ALIAS " repeat", NULL, 0, NULL);
	bgmL3Playing = true;
	bgmL3WatchTicks = BGM_L3_WATCHDOG_TICKS;
	return true;
}

// Cheap safety net, called once per fixed tick from iMain::fixedUpdate().
// Some MCI drivers honour "repeat" and some quietly stop at the end of the
// file; this restarts the track in that second case.
inline void tickLevel3Track() {
	if (!bgmL3Playing || !bgmL3Available || isMusicMuted) return;
	if (--bgmL3WatchTicks > 0) return;
	bgmL3WatchTicks = BGM_L3_WATCHDOG_TICKS;

	char buf[64] = "";
	mciSendStringA("status " BGM_L3_ALIAS " mode", buf, sizeof(buf), NULL);
	if (_stricmp(buf, "playing") != 0) {
		mciSendStringA("seek " BGM_L3_ALIAS " to start", NULL, 0, NULL);
		mciSendStringA("play " BGM_L3_ALIAS " repeat", NULL, 0, NULL);
	}
}

inline void playMenuBGM() {
	stopLevel3Track();
	if (isMusicMuted || isBGMPlaying) return;
	PlaySound(TEXT("Audios\\bgm1.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
	isBGMPlaying = true;
}

inline void stopBGM() {
	stopLevel3Track();          // the Titan mp3 lives on MCI, not PlaySound
	PlaySound(NULL, 0, 0);
	isBGMPlaying = false;
}

// FIX: this was an empty stub before, which is why gameplay had no music
// and left isBGMPlaying out of sync with what was actually playing.
inline void playGameBGM() {
	stopLevel3Track();
	if (isMusicMuted) return;
	PlaySound(TEXT("Audios\\bgm.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
	isBGMPlaying = true;
}

// Level 2 gets its own track. PlaySound() can only ever have ONE looping
// sound at a time, which is exactly what we want for BGM.
inline void playLevel2BGM() {
	stopLevel3Track();
	if (isMusicMuted) return;
	PlaySound(TEXT("Audios\\bgm2.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
	isBGMPlaying = true;
}

// Level 3 (Titan boss arena) - Audios\\level3.mp3, looped through MCI.
// Falls back to bgm2.wav only if that mp3 is missing, so the arena is never
// silent even on a fresh checkout without the audio pack.
inline void playLevel3BGM() {
	PlaySound(NULL, 0, 0);          // drop whatever WAV loop was running
	isBGMPlaying = false;
	if (isMusicMuted) return;

	if (startLevel3Track()) {       // the intended path: level3.mp3
		isBGMPlaying = true;
		return;
	}

	PlaySound(TEXT("Audios\\bgm2.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
	isBGMPlaying = true;
}

// Re-arms whichever track belongs to the level being resumed.
inline void playCurrentLevelBGM() {
	if (currentLevel == 3)      playLevel3BGM();
	else if (currentLevel == 2) playLevel2BGM();
	else                        playGameBGM();
}

// =====================================================================
// WEAPON SFX (ak47.mp3 / pistol.mp3 / flame.mp3)
//
// PlaySound() is WAV-only - it silently does nothing with an .mp3 - and it
// can only hold one sound at a time, so it would also kill the looping BGM
// on every shot. So weapon SFX go through MCI instead: each file is opened
// ONCE at startup under an alias, and firing just seeks and plays a slice of
// it. MCI happily runs alongside PlaySound's looping BGM.
//
// The files are ~5-10s long with silence around the actual hit, so each one
// plays only a window of itself - tune the four numbers below (milliseconds)
// until each shot sounds tight. "from X to Y" also restarts a sound that is
// already playing, which is what makes rapid AK fire re-trigger cleanly
// instead of being swallowed.
// =====================================================================
#define SFX_PISTOL_FROM_MS 0
#define SFX_PISTOL_TO_MS   320
#define SFX_AK47_FROM_MS   0
#define SFX_AK47_TO_MS     220
#define SFX_FLAME_FROM_MS  0
#define SFX_FLAME_TO_MS    1400

// Plasma Rifle (Audios\PlasmaRifle.mp3) - the Level 3 weapon. The window is
// the longest of the four because the rifle also has the slowest firing
// animation in the roster: PLASMA_RIFLE_ATTACK_TICKS is 24 ticks at 30ms,
// i.e. ~720ms per bolt (see Header/PlasmaRifle.hpp), so a 700ms slice plays
// out almost exactly one shot's worth of sound and is retriggered by the
// next bolt rather than being cut off by it.
#define SFX_PLASMA_FROM_MS 0
#define SFX_PLASMA_TO_MS   700

__declspec(selectany) bool sfxReady = false;

// MCI's volume scale is 0..1000. Weapons are pinned to the ceiling so they
// cut cleanly through the looping BGM underneath.
//
// IMPORTANT: this raises PLAYBACK gain, it cannot add loudness the source
// file doesn't have. If pistol/ak47/flame still sound thin at 1000, the mp3s
// themselves are quietly mastered and need normalizing (Audacity ->
// Effect -> Normalize to about -1.0 dB) - no amount of MCI volume fixes a
// quiet source.
#define SFX_VOLUME_MAX 1000

// Weapons sit ABOVE the master setting deliberately - they were still thin
// against the BGM at parity, so pistol/AK/flame get a 1.35x bias and clamp
// at the MCI ceiling. Music uses masterVolume straight.
#define SFX_WEAPON_BIAS 1.35f

inline void sfxSetVolume(const char* alias, int vol) {
	char cmd[160];
	sprintf_s(cmd, sizeof(cmd), "setaudio %s volume to %d", alias, vol);
	mciSendStringA(cmd, NULL, 0, NULL);
}

inline void sfxOpen(const char* path, const char* alias) {
	char cmd[320];
	// "mpegvideo" is the MCI device that handles mp3 playback on Windows.
	sprintf_s(cmd, sizeof(cmd), "open \"%s\" type mpegvideo alias %s", path, alias);
	mciSendStringA(cmd, NULL, 0, NULL);
	int v = (int)(masterVolume * 10 * SFX_WEAPON_BIAS);
	if (v > SFX_VOLUME_MAX) v = SFX_VOLUME_MAX;
	if (v < 0) v = 0;
	sfxSetVolume(alias, v);
}

// Call once from main(), after iInitialize().
inline void initWeaponAudio() {
	if (sfxReady) return;
	sfxOpen("Audios\\pistol.mp3", "sfxPistol");
	sfxOpen("Audios\\ak47.mp3", "sfxAk47");
	sfxOpen("Audios\\flame.mp3", "sfxFlame");

	// Plasma Rifle - opened here alongside the other three so it is a
	// one-time MCI device creation for the whole process, exactly like
	// them. If the mp3 is missing, MCI simply never creates the device and
	// every later "play sfxPlasma ..." is a harmless no-op - the rifle goes
	// back to firing silently rather than erroring out, same graceful
	// fallback every optional asset in this project has.
	sfxOpen("Audios\\PlasmaRifle.mp3", "sfxPlasma");

	sfxReady = true;
}

// Re-applies masterVolume to the three weapon aliases. Called whenever the
// settings slider moves or mute is toggled. Music volume is handled by
// PlaySound, which has no volume API - muting stops it instead.
inline void applyMasterVolume() {
	int v = (int)(masterVolume * 10 * SFX_WEAPON_BIAS);
	if (v > SFX_VOLUME_MAX) v = SFX_VOLUME_MAX;
	if (v < 0) v = 0;
	if (isMusicMuted) v = 0;
	sfxSetVolume("sfxPistol", v);
	sfxSetVolume("sfxAk47", v);
	sfxSetVolume("sfxFlame", v);
	sfxSetVolume("sfxPlasma", v);   // Level 3's plasma rifle rides the slider too
	bgmL3ApplyVolume();   // Level 3's mp3 rides the same slider
}

inline void sfxPlaySlice(const char* alias, int fromMs, int toMs) {
	if (!sfxReady || isMusicMuted) return;
	char cmd[160];
	sprintf_s(cmd, sizeof(cmd), "play %s from %d to %d", alias, fromMs, toMs);
	mciSendStringA(cmd, NULL, 0, NULL);
}

// Small floor between repeats so a held trigger can't stack dozens of
// overlapping starts (which is what makes MCI audio crackle).
#define SFX_MIN_GAP_TICKS 3
__declspec(selectany) int sfxPistolCooldown = 0;
__declspec(selectany) int sfxAk47Cooldown = 0;
__declspec(selectany) int sfxFlameCooldown = 0;
__declspec(selectany) int sfxPlasmaCooldown = 0;

inline void tickWeaponSfx() {
	if (sfxPistolCooldown > 0) sfxPistolCooldown--;
	if (sfxAk47Cooldown > 0) sfxAk47Cooldown--;
	if (sfxFlameCooldown > 0) sfxFlameCooldown--;
	if (sfxPlasmaCooldown > 0) sfxPlasmaCooldown--;
}

inline void playPistolSfx() {
	if (sfxPistolCooldown > 0) return;
	sfxPistolCooldown = SFX_MIN_GAP_TICKS;
	sfxPlaySlice("sfxPistol", SFX_PISTOL_FROM_MS, SFX_PISTOL_TO_MS);
}

inline void playAk47Sfx() {
	if (sfxAk47Cooldown > 0) return;
	sfxAk47Cooldown = SFX_MIN_GAP_TICKS;
	sfxPlaySlice("sfxAk47", SFX_AK47_FROM_MS, SFX_AK47_TO_MS);
}

// Plasma Rifle - one slice per bolt, same shape as the pistol/AK above.
// Called from Playercontroller.hpp on the tick a bolt actually spawns.
inline void playPlasmaSfx() {
	if (sfxPlasmaCooldown > 0) return;
	sfxPlasmaCooldown = SFX_MIN_GAP_TICKS;
	sfxPlaySlice("sfxPlasma", SFX_PLASMA_FROM_MS, SFX_PLASMA_TO_MS);
}

// The flamethrower is a sustained stream, so its clip is re-armed only when
// the stream actually (re)starts, and left alone while it runs.
inline void playFlameSfx() {
	if (sfxFlameCooldown > 0) return;
	sfxFlameCooldown = (SFX_FLAME_TO_MS - SFX_FLAME_FROM_MS) / 30; // clip length in ticks
	sfxPlaySlice("sfxFlame", SFX_FLAME_FROM_MS, SFX_FLAME_TO_MS);
}

inline void stopFlameSfx() {
	if (!sfxReady) return;
	mciSendStringA("stop sfxFlame", NULL, 0, NULL);
	sfxFlameCooldown = 0;
}

// NOTE: do NOT put "#define STB_IMAGE_IMPLEMENTATION / #include stb_image.h"
// anywhere in this file or any other header. That macro must be defined
// exactly ONCE in the whole project (already done at the top of iMain.cpp,
// right before iGraphics.h). A second definition anywhere else causes
// LNK2005 duplicate-symbol errors for every stbi_* function at link time.

#endif