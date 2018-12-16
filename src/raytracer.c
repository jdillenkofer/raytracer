#include "raytracer.h"

#include <float.h>
#include <stdbool.h>
#include <utils/random.h>

#include "utils/math.h"

static Vec3 raytracer_refract(Vec3 direction, Vec3 normal, double refractionIndex) {
    double cosi = math_clamp(-1, 1, vec3_dot(direction, normal));
    // refractionIndex of air is ~ 1
    double etai = 1; double etat = refractionIndex;
    Vec3 n = normal;
    if (cosi < 0) {
        cosi = -cosi;
    } else {
        double tmp = etai;
        etai = etat;
        etat = tmp;
        n = vec3_mul(n, -1);
    }
    double eta = etai / etat;
    double k = 1 - eta * eta * (1 - cosi * cosi);

    Vec3 refractedDir;
    if (k < 0) {
        refractedDir = (Vec3) { 0, 0, 0 };
    } else {
        refractedDir = vec3_norm(vec3_add(vec3_mul(direction, eta), vec3_mul(n, eta * cosi - sqrt(k))));
    }
    return refractedDir;
}

static double raytracer_fresnel(Vec3 dir, Vec3 normal, float refractionIndex) {
    double kr;
    double cosi = math_clamp(-1, 1, vec3_dot(dir, normal));
    double etai = 1;
    double etat = refractionIndex;
    if (cosi > 0) {
        double tmp = etat;
        etat = etai;
        etai = tmp;
    }
    // Snell's law
    float sint = etai / etat * sqrtf(MAX(0.f, 1 - cosi * cosi));
    if (sint >= 1) {
        kr = 1;
    } else {
        float cost = sqrtf(MAX(0.f, 1 - sint * sint));
        cosi = fabsf(cosi);
        float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
        float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
        kr = (Rs * Rs + Rp * Rp) / 2;
    }
    return kr;
}

static Vec3 raytracer_calculateHitpoint(Ray* ray, double hitDistance) {
    return vec3_add(ray->origin, vec3_mul(ray->direction, hitDistance));
}

/*
 * Moves the ray origin slightly forward to prevent self intersection
 * also called "surface agne"
 */
static void raytracer_moveRayOutOfObject(Ray* ray) {
    ray->origin = vec3_add(ray->origin, vec3_mul(ray->direction, 1.0f/1000.0f));
}

static bool raytracer_intersectPlane(Plane* plane, Ray* ray, double* hitDistance, Vec3* intersectionNormal) {
        // We use the "Hesse normal form":
        //     normal * p - distanceFromOrigin = 0
        // to describe our planes
        double denominator = vec3_dot(plane->normal, ray->direction);
        if ((denominator < -EPSILON) || (denominator > EPSILON)) {
            double cosAngle = vec3_dot(plane->normal, ray->origin);
            double t = (-plane->distanceFromOrigin - cosAngle) / denominator;
            // only hit objects in front of us
            if (t > 0) {
                *intersectionNormal = plane->normal;
                *hitDistance = t;
                return true;
            }
        }
        return false;
}

static bool raytracer_intersectSphere(Sphere* sphere, Ray* ray, double* hitDistance, Vec3* intersectionNormal) {
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
                Vec3 hitPoint = raytracer_calculateHitpoint(ray, t);
                *intersectionNormal = vec3_norm(vec3_sub(hitPoint, sphere->position));
                *hitDistance = t;
                return true;
            }
        }
		return false;
}

static bool raytracer_intersectTriangle(Triangle* triangle, Ray* ray, double* hitDistance, Vec3* intersectionNormal) {
    Vec3 v0v1 = vec3_sub(triangle->v1, triangle->v0);
    Vec3 v0v2 = vec3_sub(triangle->v2, triangle->v0);
    Vec3 normal = vec3_norm(vec3_cross(v0v1, v0v2));
    double normalDotRayDir = vec3_dot(normal, ray->direction);
    if (fabs(normalDotRayDir) < EPSILON) {
        return false;
    }

    double d = vec3_dot(normal, triangle->v0);
	double t = -(vec3_dot(normal, ray->origin) - d) / normalDotRayDir;
    if (t > 0) {
        Vec3 hitPoint = raytracer_calculateHitpoint(ray, t);

        // difference to the plane test
        Vec3 c;

        Vec3 edge0 = vec3_sub(triangle->v1, triangle->v0);
        Vec3 vp0 = vec3_sub(hitPoint, triangle->v0);
        c = vec3_cross(edge0, vp0);
        if (vec3_dot(normal, c) < 0) {
            return false;
        }

        Vec3 edge1 = vec3_sub(triangle->v2, triangle->v1);
        Vec3 vp1 = vec3_sub(hitPoint, triangle->v1);
        c = vec3_cross(edge1, vp1);
        if (vec3_dot(normal, c) < 0) {
            return false;
        }

        Vec3 edge2 = vec3_sub(triangle->v0, triangle->v2);
        Vec3 vp2 = vec3_sub(hitPoint, triangle->v2);
        c = vec3_cross(edge2, vp2);
        if (vec3_dot(normal, c) < 0) {
            return false;
        }

        *intersectionNormal = normal;
        *hitDistance = t;
        return true;
    }
    return false;
}

static void raytracer_calcClosestPlaneIntersect(Scene* scene, Ray* ray, double* minHitDistance, Vec3* intersectionNormal,
                                                uint32_t* hitMaterialIndex) {
    for (uint32_t i = 0; i < scene->planeCount; i++) {
        Plane* plane = &scene->planes[i];
        double planeHitDistance = DBL_MAX;
        Vec3 planeIntersectionNormal = {0, 0, 0};
        if (raytracer_intersectPlane(plane, ray, &planeHitDistance, &planeIntersectionNormal)) {
            if (planeHitDistance < *minHitDistance) {
                *intersectionNormal = planeIntersectionNormal;
                *minHitDistance = planeHitDistance;
                *hitMaterialIndex = plane->materialIndex;
            }
        }
    }
}

static void raytracer_calcClosestSphereIntersect(Scene* scene, Ray *ray, double* minHitDistance, Vec3* intersectionNormal,
                                                 uint32_t* hitMaterialIndex) {
    for (uint32_t i = 0; i < scene->sphereCount; i++) {
        Sphere* sphere = &scene->spheres[i];
        double sphereHitDistance = DBL_MAX;
        Vec3 sphereIntersectionNormal = {0.0f, 0.0f, 0.0f};
        if (raytracer_intersectSphere(sphere, ray, &sphereHitDistance, &sphereIntersectionNormal)) {
            if (sphereHitDistance < *minHitDistance) {
                *intersectionNormal = sphereIntersectionNormal;
                *minHitDistance = sphereHitDistance;
                *hitMaterialIndex = sphere->materialIndex;
            }
        }
    }
}

static void raytracer_calcClosestTriangleIntersect(Scene* scene, Ray* ray, double* minHitDistance, Vec3* intersectionNormal,
                                                   uint32_t* hitMaterialIndex) {
    for (uint32_t i = 0; i < scene->triangleCount; i++) {
        Triangle* triangle = &scene->triangles[i];
        double triangleHitDistance = DBL_MAX;
        Vec3 triangleIntersectionNormal = {0.0f, 0.0f, 0.0f};
        if (raytracer_intersectTriangle(triangle, ray, &triangleHitDistance, &triangleIntersectionNormal)) {
            if (triangleHitDistance < *minHitDistance) {
                *intersectionNormal = triangleIntersectionNormal;
                *minHitDistance = triangleHitDistance;
                *hitMaterialIndex = triangle->materialIndex;
            }
        }
    }
}

Vec3 raytracer_raycast(Scene* scene, Ray* primaryRay, uint32_t recursionDepth, uint32_t maxRecursionDepth) {
    Vec3 outColor = (Vec3) { 0.0f, 0.0f, 0.0f };

    if (recursionDepth >= maxRecursionDepth) {
        return outColor;
    }

    double minHitDistance = DBL_MAX;
    uint32_t hitMaterialIndex = 0;

    Vec3 intersectionNormal = {0, 0, 0};

    raytracer_calcClosestPlaneIntersect(scene, primaryRay, &minHitDistance, &intersectionNormal, &hitMaterialIndex);
    raytracer_calcClosestSphereIntersect(scene, primaryRay, &minHitDistance, &intersectionNormal, &hitMaterialIndex);
    raytracer_calcClosestTriangleIntersect(scene, primaryRay, &minHitDistance, &intersectionNormal, &hitMaterialIndex);

    if (hitMaterialIndex) {

        Material* hitMaterial = &scene->materials[hitMaterialIndex];

		// if we got a hit, calculate the hitPoint and send a shadow rays to each lightsource
		Vec3 hitPoint = raytracer_calculateHitpoint(primaryRay, minHitDistance);

		// REFLECTION AND REFRACTION
		if (hitMaterial->refractionIndex > 0) {
		    double kr = raytracer_fresnel(primaryRay->direction, intersectionNormal, hitMaterial->refractionIndex);

            Vec3 refractionColor = { 0.0f, 0.0f, 0.0f };

            // compute refraction if it is not a case of total internal reflection
            if (kr < 1) {
                Ray refractedRay;
                refractedRay.origin = hitPoint;
                refractedRay.direction = raytracer_refract(primaryRay->direction, intersectionNormal, hitMaterial->refractionIndex);
                raytracer_moveRayOutOfObject(&refractedRay);

                refractionColor = raytracer_raycast(scene, &refractedRay, recursionDepth + 1, maxRecursionDepth);
            }

            Ray reflectedRay;
            reflectedRay.origin = hitPoint;
            reflectedRay.direction = vec3_reflect(primaryRay->direction, intersectionNormal);
            raytracer_moveRayOutOfObject(&reflectedRay);

            Vec3 reflectionColor = raytracer_raycast(scene, &reflectedRay, recursionDepth + 1, maxRecursionDepth);

            // mix the two
            outColor = vec3_add(outColor, vec3_add(vec3_mul(reflectionColor, kr), vec3_mul(refractionColor, (1 - kr))));
        } else
		// REFLECTION:
		if (hitMaterial->reflectionIndex > 0) {

			Ray reflectedRay;
            reflectedRay.origin = hitPoint;
            reflectedRay.direction = vec3_reflect(primaryRay->direction, intersectionNormal);
            raytracer_moveRayOutOfObject(&reflectedRay);

			Vec3 reflectionColor = raytracer_raycast(scene, &reflectedRay, recursionDepth + 1, maxRecursionDepth);

			outColor = vec3_add(outColor, vec3_mul(reflectionColor, hitMaterial->reflectionIndex));
		}

		// SHADOWS
        for (uint32_t i = 0; i < scene->pointLightCount; i++) {
            PointLight* pointLight = &scene->pointLights[i];
            Ray shadowRay = {0};
            Vec3 hitToLight = vec3_sub(pointLight->position, hitPoint);
            Vec3 randomOffset = vec3_norm((Vec3) { random_bilateral(), random_bilateral(), random_bilateral()});
            hitToLight = vec3_add(hitToLight, randomOffset);
            double distanceToLight = vec3_length(hitToLight);

            shadowRay.origin = hitPoint;
            shadowRay.direction = vec3_norm(hitToLight);
            raytracer_moveRayOutOfObject(&shadowRay);

            uint32_t shadowRayHitMaterialIndex = 0;
            double closestHitDistance = DBL_MAX;
            Vec3 shadowRayIntersectionNormal = {0, 0, 0};
            raytracer_calcClosestPlaneIntersect(scene, &shadowRay, &closestHitDistance, &shadowRayIntersectionNormal, &shadowRayHitMaterialIndex);
            raytracer_calcClosestSphereIntersect(scene, &shadowRay, &closestHitDistance, &shadowRayIntersectionNormal, &shadowRayHitMaterialIndex);
            raytracer_calcClosestTriangleIntersect(scene, &shadowRay, &closestHitDistance, &shadowRayIntersectionNormal, &shadowRayHitMaterialIndex);
            if (distanceToLight < closestHitDistance) {
                // we hit the light
                double cosAngle = vec3_dot(shadowRay.direction, intersectionNormal);
                cosAngle = math_clamp(cosAngle, 0.0f, 1.0f);

                double lightStrength = (pointLight->strength/(4 * PI * distanceToLight * distanceToLight));
                Vec3 diffuseLighting = vec3_mul(pointLight->emissionColor, cosAngle * lightStrength);

                Vec3 toView = vec3_norm(vec3_sub(scene->camera->position, hitPoint));
                Vec3 toLight = vec3_mul(shadowRay.direction, -1);
                Vec3 reflectionVector = vec3_reflect(toLight, intersectionNormal);
                cosAngle = vec3_dot(toView, reflectionVector);
                cosAngle = pow(cosAngle, 64);

                Vec3 specularLighting = vec3_mul(pointLight->emissionColor, cosAngle * lightStrength);

                outColor = vec3_add(outColor, vec3_mul(vec3_add(diffuseLighting, specularLighting), (1-hitMaterial->reflectionIndex)));
            }
        }
        outColor = vec3_hadamard(outColor, hitMaterial->color);
    }
    return outColor;
}
