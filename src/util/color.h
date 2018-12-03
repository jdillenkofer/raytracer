#ifndef RAYTRACER_COLOR_H
#define RAYTRACER_COLOR_H

#include <math.h>
#include <assert.h>

#define ASSERT_COLOR_VALUE_NOT_NAN(color) \
    (assert(!isnan(color.r) && !isnan(color.g) && !isnan(color.b) && !isnan(color.a)))

typedef struct {
    double r, g, b, a;
} Color;

Color color_add(Color a, Color b);
Color color_mul(Color a, double b);

#endif //RAYTRACER_COLOR_H
