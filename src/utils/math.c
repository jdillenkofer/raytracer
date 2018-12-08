#include "math.h"

double math_clamp(double in, double min, double max) {
    return in < min ? min : (in > max ? max : in);
}
