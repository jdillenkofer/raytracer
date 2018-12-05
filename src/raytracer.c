#include "raytracer.h"

#include <float.h>

static void raytracer_intersectPlanes(Scene *scene, Ray *ray, double *hitDistance, uint32_t* hitMaterialIndex) {
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
                *hitMaterialIndex = plane.materialIndex;
            }
        }
    }
}

static void raytracer_intersectSpheres(Scene *scene, Ray *ray, double *hitDistance, uint32_t* hitMaterialIndex) {
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
                *hitMaterialIndex = sphere.materialIndex;
            }
        }
    }
}

Vec3 raytracer_raycast(Scene* scene, Ray* ray) {
    Vec3 outColor = (Vec3) { 0.0f, 0.0f, 0.0f };

    double hitDistance = DBL_MAX;
    uint32_t hitMaterialIndex = 0;

    raytracer_intersectPlanes(scene, ray, &hitDistance, &hitMaterialIndex);
    raytracer_intersectSpheres(scene, ray, &hitDistance, &hitMaterialIndex);

    // if we got a hit calculate the hitPoint and send a shadow rays to each lightsource
    if (hitMaterialIndex) {

        Material hitMaterial = scene->materials[hitMaterialIndex];

        Vec3 hitPoint = vec3_add(ray->origin, vec3_mul(ray->direction, hitDistance));
        for (uint32_t i = 0; i < scene->pointLightCount; i++) {
            PointLight pointLight = scene->pointLights[i];
            Ray shadowRay = {0};
            shadowRay.direction = vec3_sub(pointLight.position, hitPoint);
            double distanceToLight = vec3_length(shadowRay.direction);
            shadowRay.direction = vec3_norm(shadowRay.direction);
            // move the ray origin slightly forward to prevent self intersection
            // "surface agne"
            shadowRay.origin = vec3_add(hitPoint, vec3_mul(shadowRay.direction,1.0f/1000.0f));
            double shadowRayHitDistance = DBL_MAX;
            uint32_t shadowRayMaterialIndex;
            raytracer_intersectPlanes(scene, &shadowRay, &shadowRayHitDistance, &shadowRayMaterialIndex);
            raytracer_intersectSpheres(scene, &shadowRay, &shadowRayHitDistance, &shadowRayMaterialIndex);
            if (distanceToLight < shadowRayHitDistance) {
                // we hit the light
                outColor = vec3_add(outColor, pointLight.emissionColor);
            }
        }
        outColor = vec3_hadamard(outColor, hitMaterial.color);
    }
    return outColor;
}
