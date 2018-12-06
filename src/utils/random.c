#include "utils/random.h"

#include <stdlib.h>

double random_unilateral() {
    return (double) rand() / (double) RAND_MAX;
}

double random_bilateral() {
    return -1.0f + 2.0f * random_unilateral();
}