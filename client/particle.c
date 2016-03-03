#include "particle.h"

void particle_new(psystem_t *s, vec3 pos, uint8_t d, uint16_t lifetime)
{
    particle_t *p;

    if (s->count == 256)
        return;

    p = &s->data[s->count++];
    p->pos = pos;
    p->def = d;
    p->lifetime = lifetime;
}

void particle_netframe(psystem_t *s, uint8_t delta)
{
    int i, j;
    particle_t *p;

    for (j = 0, i = 0; i < s->count; i++) {
        p = &s->data[i];

        if (p->lifetime <= delta)
            continue;
        p->lifetime -= delta;

        s->data[j++] = *p;
    }
    s->count = j;
}

void particle_clear(psystem_t *s)
{
    s->count = 0;
}
