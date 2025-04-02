#include "../include/sound_effects.h"
#include "audioMixer.h"


// We will load the shoot wave file into this structure:
static wavedata_t shootSound;
static int isInitialized = 0;

void SoundEffects_init(void) {
    if (isInitialized) {
        return;
    }

    // 1) Initialize the audio mixer
    AudioMixer_init();

    // 2) Read the "shoot.wav" wave file into memory
    AudioMixer_readWaveFileIntoMemory("wav-files/shoot.wav", &shootSound);

    isInitialized = 1;
}

void SoundEffects_cleanup(void) {
    if (!isInitialized) {
        return;
    }

    AudioMixer_freeWaveFileData(&shootSound);
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
