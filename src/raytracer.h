#ifndef RAYTRACER_RAYTRACER_H
#define RAYTRACER_RAYTRACER_H

#include "utils/vec3.h"
#include "ray.h"
#include "scene.h"

#define EPSILON 0.00001f

Vec3 raytracer_raycast(Scene *scene, Ray *primaryRay, uint32_t maxRecursionDepth);

#endif //RAYTRACER_RAYTRACER_H
