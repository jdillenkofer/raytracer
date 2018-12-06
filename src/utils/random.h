#ifndef RAYTRACER_RANDOM_H
#define RAYTRACER_RANDOM_H

#include <stdlib.h>

int seed = 0x1AEFCB36AL;

int random_next(int bits) {
	seed = (seed * 0x5DEECE66DL + 0xBL) & ((1L << 48) - 1);
	return (int)(seed >> (48 - bits));
}

double random_unilateral() {
	return (double) random_next(16) / (double) 65536;
}

double random_bilateral() {
	return -1.0f + 2.0f * random_unilateral();
}

#endif //RAYTRACER_RANDOM_H
