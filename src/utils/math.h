#ifndef RAYTRACER_MATH_H
#define RAYTRACER_MATH_H
#define _USE_MATH_DEFINES
#include <math.h>

#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

#define PI M_PI

double math_clamp(double in, double min, double max);
double math_deg2rad(double deg);
double math_rad2deg(double rad);
#endif //RAYTRACER_MATH_H
