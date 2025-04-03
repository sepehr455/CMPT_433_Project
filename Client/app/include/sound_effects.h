#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Module responsible for playing sound effects
 */

void SoundEffects_init(void);

void SoundEffects_cleanup(void);

void SoundEffects_playShoot(void);

void SoundEffects_playLost(void);

#ifdef __cplusplus
}
#endif
