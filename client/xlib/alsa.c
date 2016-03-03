#include <stdbool.h>
#include <alsa/asoundlib.h>
#include "../audio.h"

volatile bool alsa_init, alsa_quit;

#define buffersize 1024

static snd_pcm_t* alsa_open(const char *dev_name)
{
    snd_pcm_t *handle;

    if (snd_pcm_open (&handle, dev_name, SND_PCM_STREAM_PLAYBACK, 0) < 0)
        return 0;

    return handle;
}

static int set_hw_params(snd_pcm_t *handle)
{
    snd_pcm_hw_params_t *params;
    int err;
    snd_pcm_uframes_t period_size, buffer_size;
    unsigned int periods, time;
    int dir;

    if ((err = snd_pcm_hw_params_malloc(&params)) < 0)
        return err;

    if ((err = snd_pcm_hw_params_any(handle, params)) < 0)
        goto ERR;

    if ((err = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
        goto ERR;

    if ((err = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_FLOAT_LE)) < 0)
        goto ERR;

    if ((err = snd_pcm_hw_params_set_rate(handle, params, audio_samplerate, 0)) < 0)
        goto ERR;

    if ((err = snd_pcm_hw_params_set_channels(handle, params, 2)) < 0)
        goto ERR;

    time = (4ull * 1000000 * buffersize - 1) / audio_samplerate + 1;
    if ((err = snd_pcm_hw_params_set_buffer_time_near(handle, params, &time, NULL)) < 0)
        goto ERR;

    time = (1000000ull * buffersize - 1) / audio_samplerate + 1;
    if ((err = snd_pcm_hw_params_set_period_time_near(handle, params, &time, NULL)) < 0)
        goto ERR;

    if ((err = snd_pcm_hw_params(handle, params)) < 0)
        goto ERR;

    snd_pcm_hw_params_get_period_size(params, &period_size, &dir);
    snd_pcm_hw_params_get_periods(params, &periods, &dir);
    snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
    printf("alsa period_size=%u periods=%u buffer_size=%u\n",
           (int) period_size, periods, (int) buffer_size);

ERR:
    snd_pcm_hw_params_free(params);
    return err;
}

static int set_sw_params(snd_pcm_t *handle)
{
    snd_pcm_sw_params_t *params;
    int err;

    if ((err = snd_pcm_sw_params_malloc(&params)) < 0)
        return err;

    if ((err = snd_pcm_sw_params_current(handle, params)) < 0)
        goto ERR;

    if ((err = snd_pcm_sw_params_set_avail_min(handle, params, buffersize)) < 0)
        goto ERR;

    if ((err = snd_pcm_sw_params_set_start_threshold(handle, params, 0)) < 0)
        goto ERR;

    if ((err = snd_pcm_sw_params(handle, params)) < 0)
        goto ERR;

ERR:
    snd_pcm_sw_params_free(params);
    return err;
}

#ifndef ALSA_DEVICE
#define ALSA_DEVICE "default"
#endif

enum {
    max_frames = (32768 / sizeof(sample_t)), /* half of stack size */
};

void alsa_thread(void *args)
{
    snd_pcm_t *handle;
    snd_pcm_sframes_t frames, avail;
    int ret;
    sample_t samples[max_frames], *p;
    float buf[max_frames];

    handle = alsa_open(ALSA_DEVICE);
    if (!handle)
        goto EXIT;

    if (set_hw_params(handle) < 0)
        goto EXIT_CLOSE;

    if (set_sw_params(handle) < 0)
        goto EXIT_CLOSE;

    if (snd_pcm_prepare(handle) < 0)
        goto EXIT_CLOSE;

    alsa_init = 1;
    while (alsa_init) {
        snd_pcm_wait(handle, 1000);
        frames = snd_pcm_avail_update(handle);
        if (frames < 0) {
            printf("frames < 0\n");
            break;
        }
        if (frames > max_frames)
            frames = max_frames;

        audio_getsamples(args, samples, buf, frames);

        avail = frames;
        p = samples;
        do {
            ret = snd_pcm_writei (handle, p, avail);
            if (ret < 0) {
                if (ret == -EAGAIN) {
                    continue;
                } else if (ret == -ESTRPIPE || ret == -EPIPE || ret == -EINTR) {
                    ret = snd_pcm_recover(handle, ret, 1);
                    printf("alsa recover %i\n", ret);
                }

                if (ret < 0) {
                    ret = snd_pcm_prepare(handle);
                    printf("alsa prepare %i\n", ret);
                    if (ret < 0)
                        goto EXIT_CLOSE;
                }
            } else {
                if (ret != avail)
                    printf("incomplete write\n");
                avail -= ret;
                p += ret;
            }
        } while (avail);
    }

EXIT_CLOSE:
    snd_pcm_close(handle);
EXIT:
    snd_config_update_free_global();
    alsa_quit = 1;
}
