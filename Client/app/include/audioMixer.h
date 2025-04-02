#ifndef AUDIO_MIXER_H
#define AUDIO_MIXER_H

#include <stdint.h>

// Data structure to hold wave file data
typedef struct {
    int numSamples;
    short *pData;
} wavedata_t;

#define AUDIOMIXER_MAX_VOLUME 100

// Must be called before any other functions.
// Must be cleaned up last to stop playback threads and free memory.
void AudioMixer_init(void);

void AudioMixer_cleanup(void);

// Read the contents of a wave file into the pSound structure.
void AudioMixer_readWaveFileIntoMemory(char *fileName, wavedata_t *pSound);

void AudioMixer_freeWaveFileData(wavedata_t *pSound);

// Queue up another sound to play.
void AudioMixer_queueSound(wavedata_t *pSound);

// Get/set volume
int AudioMixer_getVolume(void);

void AudioMixer_setVolume(int newVolume);

// For debugging or stats if you like
int AudioMixer_getMinLatency(void);

int AudioMixer_getMaxLatency(void);

double AudioMixer_getAvgLatency(void);

#endif
