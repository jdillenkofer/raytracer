#include "utils/random.h"

#include <stdlib.h>

float random_unilateral() {
    return (float) rand() / (float) RAND_MAX;
}

float random_bilateral() {
    return -1.0f + 2.0f * random_unilateral();
}