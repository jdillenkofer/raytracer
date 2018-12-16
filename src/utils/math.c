#include "math.h"

double math_clamp(double in, double min, double max) {
    return in < min ? min : (in > max ? max : in);
}

double math_deg2rad(double deg) {
    return deg * (PI/180.f);
}

double math_rad2deg(double rad) {
    return rad * (180.f/PI);
}