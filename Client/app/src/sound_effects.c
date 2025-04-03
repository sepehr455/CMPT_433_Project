#include <stdio.h>
#include "../include/sound_effects.h"
#include "audioMixer.h"


// We will load the shoot wave file into this structure:
static wavedata_t shootSound;
// We will also load the lost wave file into this structure:
static wavedata_t lostSound;

static int isInitialized = 0;

void SoundEffects_init(void) {
    if (isInitialized) {
        return;
    }

    // 1) Initialize the audio mixer
    AudioMixer_init();

    AudioMixer_readWaveFileIntoMemory("wav-files/shoot.wav", &shootSound);
    AudioMixer_readWaveFileIntoMemory("wav-files/lost.wav", &lostSound);

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
