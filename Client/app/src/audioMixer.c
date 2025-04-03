#include "audioMixer.h"
#include <alsa/asoundlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <limits.h>
#include <time.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static snd_pcm_t *handle;

#define DEFAULT_VOLUME 40

#define SAMPLE_RATE 44100
#define NUM_CHANNELS 1
#define SAMPLE_SIZE (sizeof(short))  // bytes per sample

static unsigned long playbackBufferSize = 0;
static short *playbackBuffer = NULL;

// Sound bites to be played
#define MAX_SOUND_BITES 30
typedef struct {
    wavedata_t *pSound;
    int location;
} playbackSound_t;
static playbackSound_t soundBites[MAX_SOUND_BITES];

// Playback threading
static pthread_t playbackThreadId;
static pthread_mutex_t audioMutex = PTHREAD_MUTEX_INITIALIZER;
static bool stopping = false;
static int volume = DEFAULT_VOLUME;

// Audio timing stats
static int audioMinLatency = INT_MAX;
static int audioMaxLatency = 0;
static double audioAvgLatency = 0.0;
static int audioSampleCount = 0;

static void *playbackThread(void *arg);

static void fillPlaybackBuffer(short *buff, int size);

void AudioMixer_init(void) {
    // Initialize volume
    AudioMixer_setVolume(DEFAULT_VOLUME);

    // Clear out soundBites
    for (int i = 0; i < MAX_SOUND_BITES; i++) {
        soundBites[i].pSound = NULL;
        soundBites[i].location = 0;
    }

    // Open the PCM output
    int err = snd_pcm_open(&handle, "plughw:0", SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        fprintf(stderr, "Playback open error: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    // Configure ALSA
    err = snd_pcm_set_params(handle,
                             SND_PCM_FORMAT_S16_LE,
                             SND_PCM_ACCESS_RW_INTERLEAVED,
                             NUM_CHANNELS,
                             SAMPLE_RATE,
                             1,           // allow software resampling
                             50000);      // 0.05 seconds per buffer
    if (err < 0) {
        fprintf(stderr, "Playback parameter error: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    unsigned long unusedBufferSize = 0;
    snd_pcm_get_params(handle, &unusedBufferSize, &playbackBufferSize);

    // Allocate buffer
    playbackBuffer = malloc(playbackBufferSize * sizeof(*playbackBuffer));

    // Start background thread
    stopping = false;
    pthread_create(&playbackThreadId, NULL, playbackThread, NULL);
}

void AudioMixer_cleanup(void) {
    printf("Stopping audio...\n");

    // Signal thread to stop
    stopping = true;
    pthread_join(playbackThreadId, NULL);

    // Drain any leftover audio
    snd_pcm_drain(handle);
    snd_pcm_hw_free(handle);
    snd_pcm_close(handle);

    // Free buffer
    free(playbackBuffer);
    playbackBuffer = NULL;

    printf("Audio stopped.\n");
}

// Read wave file data into pSound
void AudioMixer_readWaveFileIntoMemory(char *fileName, wavedata_t *pSound) {
    assert(pSound);

    const int PCM_DATA_OFFSET = 44; // typically 44‐byte header

    FILE *file = fopen(fileName, "rb");
    if (!file) {
        fprintf(stderr, "ERROR: Unable to open file %s.\n", fileName);
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    int sizeInBytes = ftell(file) - PCM_DATA_OFFSET;
    pSound->numSamples = sizeInBytes / SAMPLE_SIZE;

    fseek(file, PCM_DATA_OFFSET, SEEK_SET);

    pSound->pData = malloc(sizeInBytes);
    if (!pSound->pData) {
        fprintf(stderr, "ERROR: malloc for %d bytes (%s) failed.\n", sizeInBytes, fileName);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    int samplesRead = fread(pSound->pData, SAMPLE_SIZE, pSound->numSamples, file);
    if (samplesRead != pSound->numSamples) {
        fprintf(stderr, "ERROR: Could not read %d samples (%s), got %d.\n",
                pSound->numSamples, fileName, samplesRead);
        fclose(file);
        exit(EXIT_FAILURE);
    }
    fclose(file);
}

void AudioMixer_freeWaveFileData(wavedata_t *pSound) {
    free(pSound->pData);
    pSound->pData = NULL;
    pSound->numSamples = 0;
}

void AudioMixer_queueSound(wavedata_t *pSound) {
    assert(pSound && pSound->pData && pSound->numSamples > 0);

    pthread_mutex_lock(&audioMutex);
    {
        bool added = false;
        for (int i = 0; i < MAX_SOUND_BITES; i++) {
            if (soundBites[i].pSound == NULL) {
                soundBites[i].pSound = pSound;
                soundBites[i].location = 0;
                added = true;
                break;
            }
        }
        if (!added) {
            fprintf(stderr, "AudioMixer: No free slot to queue sound!\n");
        }
    }
    pthread_mutex_unlock(&audioMutex);
}

int AudioMixer_getVolume(void) {
    return volume;
}

// From StackOverflow user "trenki".
void AudioMixer_setVolume(int newVolume) {
    if (newVolume < 0 || newVolume > AUDIOMIXER_MAX_VOLUME) {
        fprintf(stderr, "Volume must be 0–100.\n");
        return;
    }
    volume = newVolume;

    long min, max;
    snd_mixer_t *mixerHandle;
    snd_mixer_selem_id_t *sid;
    const char *card = "hw:0";
    const char *selem_name = "PCM";

    snd_mixer_open(&mixerHandle, 0);
    snd_mixer_attach(mixerHandle, card);
    snd_mixer_selem_register(mixerHandle, NULL, NULL);
    snd_mixer_load(mixerHandle);

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, selem_name);
    snd_mixer_elem_t *elem = snd_mixer_find_selem(mixerHandle, sid);

    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    snd_mixer_selem_set_playback_volume_all(elem, volume * max / 100);

    snd_mixer_close(mixerHandle);
}

// Playback loop
static void *playbackThread(void *_arg) {
    (void) _arg;
    struct timespec startTime, endTime;

    while (!stopping) {
        clock_gettime(CLOCK_MONOTONIC, &startTime);

        // fill the output buffer
        fillPlaybackBuffer(playbackBuffer, playbackBufferSize);

        // write to ALSA
        snd_pcm_sframes_t frames = snd_pcm_writei(handle, playbackBuffer, playbackBufferSize);

        clock_gettime(CLOCK_MONOTONIC, &endTime);
        long latencyMs = (endTime.tv_sec - startTime.tv_sec) * 1000 +
                         (endTime.tv_nsec - startTime.tv_nsec) / 1000000;

        if (latencyMs < audioMinLatency) audioMinLatency = latencyMs;
        if (latencyMs > audioMaxLatency) audioMaxLatency = latencyMs;
        audioAvgLatency = (audioAvgLatency * audioSampleCount + latencyMs) / (audioSampleCount + 1);
        audioSampleCount++;

        // If error, attempt recovery
        if (frames < 0) {
            frames = snd_pcm_recover(handle, frames, 1);
            if (frames < 0) {
                fprintf(stderr, "Failed writing to PCM: %li\n", frames);
                exit(EXIT_FAILURE);
            }
        }
        usleep(1000); // small pause to avoid busy-loop if buffer is small
    }

    return NULL;
}

static void fillPlaybackBuffer(short *buff, int size) {
    memset(buff, 0, size * SAMPLE_SIZE); // zero out

    pthread_mutex_lock(&audioMutex);
    {
        for (int i = 0; i < MAX_SOUND_BITES; i++) {
            if (soundBites[i].pSound != NULL) {
                wavedata_t *sound = soundBites[i].pSound;
                int sampleCount = sound->numSamples;
                int offset = soundBites[i].location;

                int j = 0;
                while (j < size && offset < sampleCount) {
                    int32_t sampleOut = buff[j] + sound->pData[offset];
                    if (sampleOut > SHRT_MAX) {
                        sampleOut = SHRT_MAX;
                    } else if (sampleOut < SHRT_MIN) {
                        sampleOut = SHRT_MIN;
                    }
                    buff[j] = (short) sampleOut;

                    j++;
                    offset++;
                }
                soundBites[i].location = offset;

                // If we’ve played the entire sound, free this slot
                if (offset >= sampleCount) {
                    soundBites[i].pSound = NULL;
                    soundBites[i].location = 0;
                }
            }
        }
    }
    pthread_mutex_unlock(&audioMutex);
}
