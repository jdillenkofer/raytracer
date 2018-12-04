#include "angle.h"
#include <math.h>

double angle_deg2rad(double deg) {
    return deg * (M_PI/180.f);
}

double angle_rad2deg(double rad) {
    return rad * (180.f/M_PI);
}
