#include <stdio.h>
#include "../include/sound_effects.h"
#include "audioMixer.h"


static wavedata_t shootSound;
static wavedata_t lostSound;
static wavedata_t hitSound;

static int isInitialized = 0;

void SoundEffects_init(void) {
    if (isInitialized) {
        return;
    }

    // 1) Initialize the audio mixer
    AudioMixer_init();

    AudioMixer_readWaveFileIntoMemory("wav-files/shoot.wav", &shootSound);
    AudioMixer_readWaveFileIntoMemory("wav-files/lost.wav", &lostSound);
    AudioMixer_readWaveFileIntoMemory("wav-files/hit.wav", &hitSound);

    isInitialized = 1;
}

void SoundEffects_cleanup(void) {
    printf("SoundEffects: Cleaning up sound effects\n");
    if (!isInitialized) {
        return;
    }

    AudioMixer_freeWaveFileData(&shootSound);

    // free the lost sound as well
    AudioMixer_freeWaveFileData(&lostSound);

    AudioMixer_cleanup();

    isInitialized = 0;
}

// Public function to play the shoot effect:
void SoundEffects_playShoot(void) {
    if (!isInitialized) {
        return;
    }

    // Just queue it to the AudioMixer
    AudioMixer_queueSound(&shootSound);
}

// Public function to play the lost effect:
void SoundEffects_playLost(void) {
    if (!isInitialized) {
        return;
    }

    AudioMixer_queueSound(&lostSound);
}

void SoundEffects_playHit(void) {
    if (!isInitialized) {
        return;
    }
    AudioMixer_queueSound(&hitSound);
}
