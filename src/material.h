#ifndef RAYTRACER_MATERIAL_H
#define RAYTRACER_MATERIAL_H

#include "utils/vec3.h"

typedef struct {
	float reflectionIndex;
	float refractionIndex;
    Vec3 color;
} Material;

#endif //RAYTRACER_MATERIAL_H
