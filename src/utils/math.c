#include "math.h"

float math_clamp(float in, float min, float max) {
    return in < min ? min : (in > max ? max : in);
}

float math_deg2rad(float deg) {
    return deg * (PI/180.f);
}

float math_rad2deg(float rad) {
    return rad * (180.f/PI);
}