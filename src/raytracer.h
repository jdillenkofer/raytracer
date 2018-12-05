#ifndef RAYTRACER_RAYTRACER_H
#define RAYTRACER_RAYTRACER_H

#include "utils/vec3.h"
#include "ray.h"
#include "scene.h"

#define EPSILON 0.0001f

Vec3 raytracer_raycast(Scene *scene, Ray *ray);

#endif //RAYTRACER_RAYTRACER_H
