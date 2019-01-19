#ifndef RAYTRACER_MATERIAL_H
#define RAYTRACER_MATERIAL_H

#include "utils/vec3.h"

typedef struct {
	float reflectionIndex;
	float refractionIndex;
    float ambientWeight;
    float diffuseWeight;
    float specularWeight;
    float specularExponent;
    Vec3 color;
} Material;

#endif //RAYTRACER_MATERIAL_H
