#include "raytracer.h"

#include <float.h>
#include <stdbool.h>

static bool raytracer_intersectPlane(Plane *plane, Ray *ray, double *hitDistance) {
        // We use the "Hesse normal form":
        //     normal * p - distanceFromOrigin = 0
        // to describe our planes
        double denominator = vec3_dot(plane->normal, ray->direction);
        if ((denominator < -EPSILON) || (denominator > EPSILON)) {
            double t = (-plane->distanceFromOrigin - vec3_dot(plane->normal, ray->origin)) / denominator;
            // only hit objects in front of us
            if (t > 0) {
                *hitDistance = t;
                return true;
            }
        }
        return false;
}

static bool raytracer_intersectSphere(Sphere *sphere, Ray *ray, double *hitDistance) {
        Vec3 sphereRelativeOrigin = vec3_sub(ray->origin, sphere->position);

        // Mitternachtsformel
        double a = vec3_dot(ray->direction, ray->direction);
        double b = 2.0f * vec3_dot(ray->direction, sphereRelativeOrigin);
        double c = vec3_dot(sphereRelativeOrigin, sphereRelativeOrigin) - sphere->radius * sphere->radius;

        double denominator = 2.0f * a;
        double squareRootTerm = sqrt(b*b - 4.0f * a * c);

        if (squareRootTerm > EPSILON) {
            double tpos = (-b + squareRootTerm) / denominator;
            double tneg = (-b - squareRootTerm) / denominator;

            double t = tpos;
            // only hit objects in front of us,
            if ((tneg > 0) && (tneg < tpos)) {
                t = tneg;
            }
            if (t > 0) {
                *hitDistance = t;
                return true;
            }
        }
		return false;
}

static void raytracer_calcClosestPlaneIntersect(Scene *scene, Ray *ray, double *minHitDistance,
                                                uint32_t *hitMaterialIndex) {
    for (uint32_t i = 0; i < scene->planeCount; i++) {
        Plane* plane = &scene->planes[i];
        double planeHitDistance = DBL_MAX;
        if (raytracer_intersectPlane(plane, ray, &planeHitDistance)) {
            if (planeHitDistance < *minHitDistance) {
                *minHitDistance = planeHitDistance;
                *hitMaterialIndex = plane->materialIndex;
            }
        }
    }
}

static void raytracer_calcClosestSphereIntersect(Scene *scene, Ray *ray, double *minHitDistance,
                                                 uint32_t *hitMaterialIndex) {
    for (uint32_t i = 0; i < scene->sphereCount; i++) {
        Sphere* sphere = &scene->spheres[i];
        double sphereHitDistance = DBL_MAX;
        if (raytracer_intersectSphere(sphere, ray, &sphereHitDistance)) {
            if (sphereHitDistance < *minHitDistance) {
                *minHitDistance = sphereHitDistance;
                *hitMaterialIndex = sphere->materialIndex;
            }
        }
    }
}

Vec3 raytracer_raycast(Scene* scene, Ray* primaryRay) {
    Vec3 outColor = (Vec3) { 0.0f, 0.0f, 0.0f };

    double minHitDistance = DBL_MAX;
    uint32_t hitMaterialIndex = 0;

    raytracer_calcClosestPlaneIntersect(scene, primaryRay, &minHitDistance, &hitMaterialIndex);
    raytracer_calcClosestSphereIntersect(scene, primaryRay, &minHitDistance, &hitMaterialIndex);

    // if we got a hit calculate the hitPoint and send a shadow rays to each lightsource
    if (hitMaterialIndex) {

        Material hitMaterial = scene->materials[hitMaterialIndex];

        Vec3 hitPoint = vec3_add(primaryRay->origin, vec3_mul(primaryRay->direction, minHitDistance));
        for (uint32_t i = 0; i < scene->pointLightCount; i++) {
            PointLight pointLight = scene->pointLights[i];
            Ray shadowRay = {0};
            shadowRay.direction = vec3_sub(pointLight.position, hitPoint);
            double distanceToLight = vec3_length(shadowRay.direction);
            shadowRay.direction = vec3_norm(shadowRay.direction);
            // move the ray origin slightly forward to prevent self intersection
            // "surface agne"
            shadowRay.origin = vec3_add(hitPoint, vec3_mul(shadowRay.direction,1.0f/1000.0f));

            uint32_t shadowRayHitMaterialIndex = 0;
            double closestHitDistance = DBL_MAX;
            raytracer_calcClosestPlaneIntersect(scene, &shadowRay, &closestHitDistance, &shadowRayHitMaterialIndex);
            raytracer_calcClosestSphereIntersect(scene, &shadowRay, &closestHitDistance, &shadowRayHitMaterialIndex);
            if (distanceToLight < closestHitDistance) {
                // we hit the light
                outColor = vec3_add(outColor, pointLight.emissionColor);
            }
        }
        outColor = vec3_hadamard(outColor, hitMaterial.color);
    }
    return outColor;
}
