#ifndef RAYTRACER_RANDOM_H
#define RAYTRACER_RANDOM_H

#include <stdint.h>

typedef struct {
    uint64_t x;
    uint64_t y;
} seed128bit;

float random_unilateral(void);
float random_bilateral(void);

#endif //RAYTRACER_RANDOM_H
