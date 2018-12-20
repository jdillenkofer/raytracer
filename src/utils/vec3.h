#ifndef RAYTRACER_VEC3_H
#define RAYTRACER_VEC3_H

#include <assert.h>
#include <math.h>

#define ASSERT_VECTOR_VALUE_NOT_NAN(vec) \
    (assert(!isnan(vec.x) && !isnan(vec.y) && !isnan(vec.z)))

typedef union {
    struct {
        float x, y, z;
    };
    struct {
        float r, g, b;
    };
} Vec3;

Vec3 vec3_add(Vec3 a, Vec3 b);
Vec3 vec3_sub(Vec3 a, Vec3 b);
float vec3_dot(Vec3 a, Vec3 b);
Vec3 vec3_cross(Vec3 a, Vec3 b);
Vec3 vec3_clamp(Vec3 a, float min, float max);
Vec3 vec3_hadamard(Vec3 a, Vec3 b);
Vec3 vec3_norm(Vec3 a);
float vec3_length(Vec3 a);
Vec3 vec3_offset(Vec3 a, float offset);
Vec3 vec3_mul(Vec3 a, float b);
Vec3 vec3_div(Vec3 a, float b);
Vec3 vec3_reflect(Vec3 incomingVec, Vec3 normal);

#endif //RAYTRACER_VEC3_H
