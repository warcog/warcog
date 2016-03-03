#include "audio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <opus/opus.h>

//TODO: distance attenuation
//better "stop" (no cut-off)
//loading lock
//volume implementation?

static void lock(volatile bool *_lock)
{
    while (__sync_lock_test_and_set(_lock, 1))
        while (*_lock);
}

static void unlock(volatile bool *_lock)
{
    __sync_lock_release(_lock);
}

static void readsamples(float *dest, sound_t *sound, uint32_t i, uint32_t nsample)
{
    memcpy(dest, sound->data + i, nsample * 4);
}

static void mix(audio_t *a, ref_t ref, source_t *src, sample_t *buf, float *samples, uint32_t nsample)
{
    int i;
    vec3 dir, pos;
    float distance, ev, az, delta, gain, dirfact = 1.0;
    sound_t *sound;

    if (!src->playing)
        return;

    if (src->stop) {
        src->playing = 0;
        return;
    }

    if (src->done)
        return;

    lock(&src->lock);
    pos = src->pos;
    unlock(&src->lock);

    dir = ref_transform(sub(pos, ref.pos), ref);
    distance = mag(dir);
    if (distance > 0.0) {
        dir = scale(dir, 1.0 / distance);
        ev = asin(dir.z);
        az = atan2(dir.x, -dir.y);
    } else {
        dir.x = 0.0; dir.y = 0.0; dir.z = -1.0;
        ev = 0.0; az = 0.0;
    }

    gain = src->volume ? (float) src->volume / 256.0 : 1.0;

    if (src->moving) {
        delta = hrtf_calcfadetime(src->lastgain, gain, src->lastdir.v, dir.v);
        if (delta > 0.000015) {
            src->counter = hrtf_moving(&src->hrtf_params, ev, az, dirfact, gain, delta, src->counter);
            src->lastgain = gain;
            src->lastdir = dir;
        }
    } else {
        hrtf_lerped(&src->hrtf_params, ev, az, dirfact, gain);
        memset(&src->hrtf_state, 0, sizeof(src->hrtf_state));

        src->i = 0;
        src->offset = 0;
        src->counter = 0;
        src->lastgain = gain;
        src->lastdir = dir;

        src->moving = 1;
    }

    sound = a->sound[src->sound];
    i = sound->nsample - src->i;
    if (i < (int) nsample) {
        if (i > 0) {
            readsamples(samples, sound, src->i, i);
            memset(&samples[i], 0, (nsample - i) * sizeof(*samples));
        } else {
            memset(samples, 0, nsample * sizeof(*samples));
            if (src->i >= sound->nsample + HRTF_HISTORY_LENGTH)
                src->done = 1;
        }
    } else {
        readsamples(samples, sound, src->i, nsample);
    }
    src->i += nsample;

    hrtf_mix(buf, samples, src->counter, src->offset, 0, &src->hrtf_params, &src->hrtf_state, nsample);
    src->offset += nsample;
    if (src->counter < nsample)
        src->counter = 0;
    else
        src->counter -= nsample;
}

void audio_getsamples(audio_t *a, sample_t *res, float *buf, uint32_t samples)
{
    uint32_t i;
    ref_t ref;

    memset(res, 0, samples * sizeof(*res));

    lock(&a->lock);
    ref = a->ref;
    unlock(&a->lock);

    for (i = 0; i < 256; i++)
        mix(a, ref, &a->source[i], res, buf, samples);
}

void audio_listener(audio_t *a, ref_t ref)
{
    lock(&a->lock);
    a->ref = ref;
    unlock(&a->lock);
}

int audio_play(audio_t *a, uint32_t sound_id, vec3 pos, bool loop, uint8_t volume)
{
    uint32_t id;
    source_t *src;

    if (!a->sound[sound_id]) /* sound failed to load */
        return -1;

    if (a->nsource == 256)
        return -1;
    a->nsource++;

    while ((src = &a->source[id = a->i++])->playing); /* find a free source */

    src->loop = loop;
    src->volume = volume;

    src->sound = sound_id;
    src->pos = pos;
    src->stop = 0;
    src->done = 0;
    src->moving = 0;
    src->playing = 1;

    //printf("play %u (%u)\n", sound_id, a->sound[sound_id]->nsample);

    return id;
}

bool audio_move(audio_t *a, uint32_t id, vec3 pos)
{
    source_t *src;

    src = &a->source[id];
    if (src->done) {
        src->playing = 0;
        a->nsource--;
        return 1;
    }

    lock(&src->lock);
    src->pos = pos;
    unlock(&src->lock);
    return 0;
}

void audio_stop(audio_t *a, uint32_t id)
{
    a->source[id].stop = 1;
    a->nsource--;
}



void audio_init(audio_t *a)
{
    (void) a;
}

void audio_cleanup(audio_t *a)
{
    //TODO
    (void) a;
}
