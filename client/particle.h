#ifndef PARTICLE_H
#define PARTICLE_H

#include <stdint.h>
#include "math.h"

typedef struct {
    vec3 pos;
    uint8_t def, unused;
    uint16_t lifetime;
} particle_t;

typedef struct {
    int count;
    particle_t data[4096];
} psystem_t;

void particle_new(psystem_t *s, vec3 pos, uint8_t d, uint16_t lifetime);
void particle_netframe(psystem_t *s, uint8_t delta);

void particle_clear(psystem_t *s);

#endif
