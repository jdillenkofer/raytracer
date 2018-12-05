#include "raytracer.h"

#include <float.h>

static void raytracer_intersectPlanes(Scene *scene, Ray *ray, double *hitDistance, Material **hitMaterial) {
    for (uint32_t i = 0; i < scene->planeCount; i++) {
        Plane plane = scene->planes[i];

        // We use the "Hesse normal form":
        //     normal * p - distanceFromOrigin = 0
        // to describe our planes
        double denominator = vec3_dot(plane.normal, ray->direction);
        if ((denominator < -EPSILON) || (denominator > EPSILON)) {
            double t = (-plane.distanceFromOrigin - vec3_dot(plane.normal, ray->origin)) / denominator;
            // only hit objects in front of us,
            // and if there are closer than the currently closest hit
            if ((t > 0) && (t < *hitDistance)) {
                *hitDistance = t;
                *hitMaterial = &scene->materials[plane.materialIndex];
            }
        }
    }
}

static void raytracer_intersectSpheres(Scene *scene, Ray *ray, double *hitDistance, Material **hitMaterial) {
    for (uint32_t i = 0; i < scene->sphereCount; i++) {
        Sphere sphere = scene->spheres[i];

        Vec3 sphereRelativeOrigin = vec3_sub(ray->origin, sphere.position);

        // Mitternachtsformel
        double a = vec3_dot(ray->direction, ray->direction);
        double b = 2.0f * vec3_dot(ray->direction, sphereRelativeOrigin);
        double c = vec3_dot(sphereRelativeOrigin, sphereRelativeOrigin) - sphere.radius * sphere.radius;

        double denominator = 2.0f * a;
        double squareRootTerm = sqrt(b*b - 4.0f * a * c);

        if (squareRootTerm > EPSILON) {
            double tpos = (-b + squareRootTerm) / denominator;
            double tneg = (-b - squareRootTerm) / denominator;

            double t = tpos;
            // only hit objects in front of us,
            // and if there are closer than the currently closest hit
            if ((tneg > 0) && (tneg < tpos)) {
                t = tneg;
            }
            if ((t > 0) && (t < *hitDistance)) {
                *hitDistance = t;
                *hitMaterial = &scene->materials[sphere.materialIndex];
            }
        }
    }
}

Vec3 raytracer_raycast(Scene* scene, Ray* ray) {
    double hitDistance = DBL_MAX;
    Material* hitMaterial = &scene->materials[0];

    raytracer_intersectPlanes(scene, ray, &hitDistance, &hitMaterial);

    raytracer_intersectSpheres(scene, ray, &hitDistance, &hitMaterial);

    Vec3 outColor = hitMaterial->color;

    return outColor;
}
