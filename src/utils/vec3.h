#ifndef RAYTRACER_VEC3_H
#define RAYTRACER_VEC3_H

#include <assert.h>
#include <math.h>

#define ASSERT_VECTOR_VALUE_NOT_NAN(vec) \
    (assert(!isnan(vec.x) && !isnan(vec.y) && !isnan(vec.z)))

typedef union {
    struct {
        double x, y, z;
    };
    struct {
        double r, g, b;
    };
} Vec3;

Vec3 vec3_add(Vec3 a, Vec3 b);
Vec3 vec3_sub(Vec3 a, Vec3 b);
double vec3_dot(Vec3 a, Vec3 b);
Vec3 vec3_cross(Vec3 a, Vec3 b);
Vec3 vec3_hadamard(Vec3 a, Vec3 b);
Vec3 vec3_norm(Vec3 a);
double vec3_length(Vec3 a);
Vec3 vec3_offset(Vec3 a, double offset);
Vec3 vec3_mul(Vec3 a, double b);
Vec3 vec3_div(Vec3 a, double b);

#endif //RAYTRACER_VEC3_H
