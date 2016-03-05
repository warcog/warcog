#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <stdbool.h>
#include "math.h"
#include "resource.h"
#include "audio/sample.h"
#include "audio/hrtf.h"

#define audio_samplerate 48000

typedef struct {
    volatile bool lock;
    volatile uint16_t sound;
    volatile bool playing, moving, done, stop;
    volatile vec3 pos;

    volatile bool loop;
    volatile uint8_t volume;

    hrtf_params_t hrtf_params;
    hrtf_state_t hrtf_state;

    uint32_t i, offset, counter;
    float lastgain;
    vec3 lastdir;
} source_t;

typedef struct {
    source_t source[256];
    uint32_t nsource;
    uint8_t i;

    volatile bool lock;
    volatile ref_t ref;
    volatile float volume;

    sound_t* sound[max_sounds];
} audio_t;

/* written for use with 2 threads,
    -1 calling only audio_getsamples()
    -1 calling the other functions
*/

/* render some samples */
void audio_getsamples(audio_t *a, sample_t *res, float *buf, uint32_t samples);

/* set global volume */
void audio_volume(audio_t *a, float volume);
/* set listener reference */
void audio_listener(audio_t *a, ref_t ref);
/* start playing a sound, return source's id, can return -1 */
int audio_play(audio_t *a, uint32_t sound_id, vec3 pos, bool loop, uint8_t volume);
/* update a source's position
 if returns true, sound has finished playing and id is no longer valid */
bool audio_move(audio_t *a, uint32_t id, vec3 pos);
/* stop playing a source */
void audio_stop(audio_t *a, uint32_t id);


/* init audio, assumes zeroed-out */
void audio_init(audio_t *a);
/* */
void audio_cleanup(audio_t *a);

#endif
