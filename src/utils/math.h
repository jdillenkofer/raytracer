#ifndef RAYTRACER_MATH_H
#define RAYTRACER_MATH_H
#define _USE_MATH_DEFINES
#include <math.h>

#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

#define PI ((float)(M_PI))

float math_clamp(float in, float min, float max);
float math_deg2rad(float deg);
float math_rad2deg(float rad);
#endif //RAYTRACER_MATH_H
