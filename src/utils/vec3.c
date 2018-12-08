#include "vec3.h"
#include "utils/math.h"

Vec3 vec3_add(Vec3 a, Vec3 b) {
    ASSERT_VECTOR_VALUE_NOT_NAN(a);
    ASSERT_VECTOR_VALUE_NOT_NAN(b);
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    return a;
}

Vec3 vec3_sub(Vec3 a, Vec3 b) {
    ASSERT_VECTOR_VALUE_NOT_NAN(a);
    ASSERT_VECTOR_VALUE_NOT_NAN(b);
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    return a;
}

double vec3_dot(Vec3 a, Vec3 b) {
    ASSERT_VECTOR_VALUE_NOT_NAN(a);
    ASSERT_VECTOR_VALUE_NOT_NAN(b);
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 vec3_cross(Vec3 a, Vec3 b) {
    ASSERT_VECTOR_VALUE_NOT_NAN(a);
    ASSERT_VECTOR_VALUE_NOT_NAN(b);
    Vec3 result = {
        a.y * b.z - a.z * b.y ,
        a.z * b.x - a.x * b.z ,
        a.x * b.y - a.y * b.x
    };
    return result;
}

Vec3 vec3_clamp(Vec3 a, double min, double max) {
    ASSERT_VECTOR_VALUE_NOT_NAN(a);
    a.r = math_clamp(a.r, min, max);
    a.g = math_clamp(a.g, min, max);
    a.b = math_clamp(a.b, min, max);
    return a;
}

Vec3 vec3_hadamard(Vec3 a, Vec3 b) {
    ASSERT_VECTOR_VALUE_NOT_NAN(a);
    ASSERT_VECTOR_VALUE_NOT_NAN(b);
    Vec3 result = {
            a.x * b.x,
            a.y * b.y,
            a.z * b.z
    };
    return result;
}

Vec3 vec3_norm(Vec3 a) {
    ASSERT_VECTOR_VALUE_NOT_NAN(a);
    double length = vec3_length(a);
    if (length != 0) {
        a.x /= length;
        a.y /= length;
        a.z /= length;
    }
    return a;
}

double vec3_length(Vec3 a) {
    ASSERT_VECTOR_VALUE_NOT_NAN(a);
    return sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

Vec3 vec3_offset(Vec3 a, double offset) {
    ASSERT_VECTOR_VALUE_NOT_NAN(a);
    assert(!isnan(offset));
    a.x += offset;
    a.y += offset;
    a.z += offset;
    return a;
}

Vec3 vec3_mul(Vec3 a, double b) {
    ASSERT_VECTOR_VALUE_NOT_NAN(a);
    assert(!isnan(b));
    a.x *= b;
    a.y *= b;
    a.z *= b;
    return a;
}

Vec3 vec3_div(Vec3 a, double b) {
    ASSERT_VECTOR_VALUE_NOT_NAN(a);
    assert(!isnan(b));
    assert(b != 0);
    a.x /= b;
    a.y /= b;
    a.z /= b;
    return a;
}
