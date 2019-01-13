#ifdef USE_SHARED_MEMORY_CAMERA
	#define CAMERA_QUALIFIER __local
#else
	#define CAMERA_QUALIFIER __global
#endif

#ifdef USE_SHARED_MEMORY_MATERIALS
#define MATERIALS_QUALIFIER __local
#else
#define MATERIALS_QUALIFIER __global
#endif

#ifdef USE_SHARED_MEMORY_PLANES
#define PLANES_QUALIFIER __local
#else
#define PLANES_QUALIFIER __global
#endif

#ifdef USE_SHARED_MEMORY_SPHERES
#define SPHERES_QUALIFIER __local
#else
#define SPHERES_QUALIFIER __global
#endif

#ifdef USE_SHARED_MEMORY_TRIANGLES
#define TRIANGLES_QUALIFIER __local
#else
#define TRIANGLES_QUALIFIER __global
#endif

#ifdef USE_SHARED_MEMORY_POINTLIGHTS
#define POINTLIGHTS_QUALIFIER __local
#else
#define POINTLIGHTS_QUALIFIER __global
#endif

#ifdef USE_SHARED_MEMORY_OCTREENODES
#define OCTREENODES_QUALIFIER __local
#else
#define OCTREENODES_QUALIFIER __global
#endif

#ifdef USE_SHARED_MEMORY_OCTREEINDEXES
#define OCTREEINDEX_QUALIFIER __local
#else
#define OCTREEINDEX_QUALIFIER __global
#endif

#define uint32_t uint
#define int32_t int
#define PI 3.14159265358979323846f

#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
float math_clamp(float in, float min, float max) {
    return in < min ? min : (in > max ? max : in);
}

float math_deg2rad(float deg) {
    return deg * (PI/180.f);
}

float math_rad2deg(float rad) {
    return rad * (180.f/PI);
}

float random_unilateral() {
    return 0;
}

float random_bilateral() {
    return -1.0f + 2.0f * random_unilateral();
}

typedef union {
    struct {
        float x, y, z;
    };
    struct {
        float r, g, b;
    };
} Vec3;

Vec3 vec3_add(Vec3 a, Vec3 b);
Vec3 vec3_sub(Vec3 a, Vec3 b);
float vec3_dot(Vec3 a, Vec3 b);
Vec3 vec3_cross(Vec3 a, Vec3 b);
Vec3 vec3_clamp(Vec3 a, float min, float max);
Vec3 vec3_hadamard(Vec3 a, Vec3 b);
Vec3 vec3_norm(Vec3 a);
float vec3_length(Vec3 a);
Vec3 vec3_offset(Vec3 a, float offset);
Vec3 vec3_mul(Vec3 a, float b);
Vec3 vec3_div(Vec3 a, float b);
Vec3 vec3_reflect(Vec3 incomingVec, Vec3 normal);

Vec3 vec3_add(Vec3 a, Vec3 b) {
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    return a;
}

Vec3 vec3_sub(Vec3 a, Vec3 b) {
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    return a;
}

float vec3_dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 vec3_cross(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    return result;
}

Vec3 vec3_clamp(Vec3 a, float min, float max) {
    a.r = math_clamp(a.r, min, max);
    a.g = math_clamp(a.g, min, max);
    a.b = math_clamp(a.b, min, max);
    return a;
}

Vec3 vec3_hadamard(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
    return result;
}

Vec3 vec3_norm(Vec3 a) {
    float length = vec3_length(a);
    if (length != 0) {
        a.x /= length;
        a.y /= length;
        a.z /= length;
    }
    return a;
}

float vec3_length(Vec3 a) {
    return sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

Vec3 vec3_offset(Vec3 a, float offset) {
    a.x += offset;
    a.y += offset;
    a.z += offset;
    return a;
}

Vec3 vec3_mul(Vec3 a, float b) {
    a.x *= b;
    a.y *= b;
    a.z *= b;
    return a;
}

Vec3 vec3_div(Vec3 a, float b) {
    a.x /= b;
    a.y /= b;
    a.z /= b;
    return a;
}

Vec3 vec3_reflect(Vec3 incomingVec, Vec3 normal) {
    Vec3 reversedVec = vec3_mul(incomingVec, -1);
    Vec3 reflectedVec = vec3_norm(vec3_sub(vec3_mul(normal, 2.0f * vec3_dot(normal, reversedVec)), reversedVec));
    return reflectedVec;
}


typedef struct {
    Vec3 position;
    Vec3 x, y, z;
    Vec3 lookAt;
    uint32_t width, height;
    float FOV, aspectRatio;
    Vec3 renderTargetCenter;
    float renderTargetWidth, renderTargetHeight, renderTargetDistance;
} Camera;

typedef struct {
    float reflectionIndex;
    float refractionIndex;
    Vec3 color;
} Material;

typedef struct {
    uint32_t materialIndex;
    Vec3 normal;
    float distanceFromOrigin;
} Plane;

typedef struct {
    uint32_t materialIndex;
    Vec3 position;
    float radius;
} Sphere;

typedef struct {
    uint32_t materialIndex;
    Vec3 v0, v1, v2;
} Triangle;

typedef struct {
    Vec3 position;
    Vec3 emissionColor;
    float strength;
} PointLight;

typedef struct {
    uint32_t width, height;
    uint32_t* buffer; // Stores the data as rgba top to bottom, left to right
    uint32_t bufferSize;
} Image;

typedef struct {
    Vec3 origin;
    Vec3 direction;
} Ray;

#define NODE_INDEX_UNDEF -1

typedef struct {
	Vec3 bottomLeftFrontCorner;
	Vec3 topRightBackCorner;
} BoundingBox;

typedef struct {
	BoundingBox boundingBox;

	uint32_t sphereIndexOffset;
	uint32_t sphereIndexCount;

	uint32_t triangleIndexOffset;
	uint32_t triangleIndexCount;

	int32_t childNodeIndexes[8];
} OctreeNode;

#define EPSILON 0.00001f
static Vec3 raytracer_refract(Vec3 direction, Vec3 normal, float refractionIndex) {
    float cosi = math_clamp(-1, 1, vec3_dot(direction, normal));
    // refractionIndex of air is ~ 1
    float etai = 1; float etat = refractionIndex;
    Vec3 n = normal;
    if (cosi < 0) {
        cosi = -cosi;
    } else {
        float tmp = etai;
        etai = etat;
        etat = tmp;
        n = vec3_mul(n, -1);
    }
    float eta = etai / etat;
    float k = 1 - eta * eta * (1 - cosi * cosi);

    Vec3 refractedDir;
    if (k < 0) {
        refractedDir;
        refractedDir.x = 0.0f;
        refractedDir.y = 0.0f;
        refractedDir.z = 0.0f;
    } else {
        refractedDir = vec3_norm(vec3_add(vec3_mul(direction, eta), vec3_mul(n, eta * cosi - sqrt(k))));
    }
    return refractedDir;
}

static float raytracer_fresnel(Vec3 dir, Vec3 normal, float refractionIndex) {
    float kr;
    float cosi = math_clamp(-1, 1, vec3_dot(dir, normal));
    float etai = 1;
    float etat = refractionIndex;
    if (cosi > 0) {
        float tmp = etat;
        etat = etai;
        etai = tmp;
    }
    // Snell's law
    float sint = etai / etat * sqrt(MAX(0.f, 1.0f - cosi * cosi));
    if (sint >= 1) {
        kr = 1;
    } else {
        float cost = sqrt(MAX(0.f, 1.0f - sint * sint));
        cosi = fabs(cosi);
        float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
        float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
        kr = (Rs * Rs + Rp * Rp) / 2;
    }
    return kr;
}

static Vec3 raytracer_calculateHitpoint(Ray* ray, float hitDistance) {
    return vec3_add(ray->origin, vec3_mul(ray->direction, hitDistance));
}

/*
 * Moves the ray origin slightly forward to prevent self intersection
 * also called \surface agne\
 */
static void raytracer_moveRayOutOfObject(Ray* ray) {
    ray->origin = vec3_add(ray->origin, vec3_mul(ray->direction, 1.0f/1000.0f));
}

static bool raytracer_intersectPlane(PLANES_QUALIFIER Plane* plane, Ray* ray, float* hitDistance, Vec3* intersectionNormal) {
        // We use the \Hesse normal form\:
        //     normal * p - distanceFromOrigin = 0
        // to describe our planes
        float denominator = vec3_dot(plane->normal, ray->direction);
        if ((denominator < -EPSILON) || (denominator > EPSILON)) {
            float cosAngle = vec3_dot(plane->normal, ray->origin);
            float t = (-plane->distanceFromOrigin - cosAngle) / denominator;
            // only hit objects in front of us
            if (t > 0) {
                *intersectionNormal = plane->normal;
                *hitDistance = t;
                return true;
            }
        }
        return false;
}

static bool raytracer_intersectSphere(SPHERES_QUALIFIER Sphere* sphere, Ray* ray, float* hitDistance, Vec3* intersectionNormal) {
        Vec3 sphereRelativeOrigin = vec3_sub(ray->origin, sphere->position);

        // Mitternachtsformel
        float a = vec3_dot(ray->direction, ray->direction);
        float b = 2.0f * vec3_dot(ray->direction, sphereRelativeOrigin);
        float c = vec3_dot(sphereRelativeOrigin, sphereRelativeOrigin) - sphere->radius * sphere->radius;

        float denominator = 2.0f * a;
        float squareRootTerm = sqrt(b*b - 4.0f * a * c);

        if (squareRootTerm > EPSILON) {
            float tpos = (-b + squareRootTerm) / denominator;
            float tneg = (-b - squareRootTerm) / denominator;

            float t = tpos;
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

static bool raytracer_intersectTriangle(TRIANGLES_QUALIFIER Triangle* triangle, Ray* ray, float* hitDistance, Vec3* intersectionNormal) {
    Vec3 v0v1 = vec3_sub(triangle->v1, triangle->v0);
    Vec3 v0v2 = vec3_sub(triangle->v2, triangle->v0);
    Vec3 normal = vec3_norm(vec3_cross(v0v1, v0v2));
    float normalDotRayDir = vec3_dot(normal, ray->direction);
    if (fabs(normalDotRayDir) < EPSILON) {
        return false;
    }

    float d = vec3_dot(normal, triangle->v0);
    float t = -(vec3_dot(normal, ray->origin) - d) / normalDotRayDir;
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

static bool raytracer_intersectBoundingBox(Ray* ray, BoundingBox boundingBox) {
	float txmin = (boundingBox.bottomLeftFrontCorner.x - ray->origin.x) / ray->direction.x;
	float txmax = (boundingBox.topRightBackCorner.x - ray->origin.x) / ray->direction.x;

	if (txmin > txmax) {
		float tmp = txmin;
		txmin = txmax;
		txmax = tmp;
	}

	float tymin = (boundingBox.bottomLeftFrontCorner.y - ray->origin.y) / ray->direction.y;
	float tymax = (boundingBox.topRightBackCorner.y - ray->origin.y) / ray->direction.y;

	if (tymin > tymax) {
		float tmp = tymin;
		tymin = tymax;
		tymax = tmp;
	}

	if ((txmin > tymax) || (tymin > txmax)) {
		return false;
	}

	if (tymin > txmin) {
		txmin = tymin;
	}

	if (tymax < txmax) {
		txmax = tymax;
	}

	float tzmin = (boundingBox.bottomLeftFrontCorner.z - ray->origin.z) / ray->direction.z;
	float tzmax = (boundingBox.topRightBackCorner.z - ray->origin.z) / ray->direction.z;

	if (tzmin > tzmax) {
		float tmp = tzmin;
		tzmin = tzmax;
		tzmax = tmp;
	}

	if ((txmin > tzmax) || (tzmin > txmax)) {
		return false;
	}
	return true;
}

static void raytracer_calcClosestPlaneIntersect(PLANES_QUALIFIER Plane* planes, uint32_t planeCount, Ray* ray, float* minHitDistance, Vec3* intersectionNormal,
                                                uint32_t* hitMaterialIndex) {
    for (uint32_t i = 0; i < planeCount; i++) {
		PLANES_QUALIFIER Plane* plane = &planes[i];
        float planeHitDistance = FLT_MAX;
        Vec3 planeIntersectionNormal;
        if (raytracer_intersectPlane(plane, ray, &planeHitDistance, &planeIntersectionNormal)) {
            if (planeHitDistance < *minHitDistance) {
                *intersectionNormal = planeIntersectionNormal;
                *minHitDistance = planeHitDistance;
                *hitMaterialIndex = plane->materialIndex;
            }
        }
    }
}

static void raytracer_calcClosestIntersectUsingOctree(SPHERES_QUALIFIER Sphere* spheres, uint32_t sphereCount, TRIANGLES_QUALIFIER Triangle* triangles, uint32_t triangleCount, 
                                                 Ray* ray, float* minHitDistance, Vec3* intersectionNormal,
                                                 uint32_t* hitMaterialIndex, OCTREENODES_QUALIFIER OctreeNode* octreeNodes, OCTREEINDEX_QUALIFIER uint32_t* octreeIndexes) {
	uint32_t nodesToCheck[200];
	uint32_t nodesToCheckCount = 0;

	// push root to the stack
	nodesToCheck[nodesToCheckCount++] = 0;

	while (nodesToCheckCount > 0) {
		uint32_t currentNodeIndex = nodesToCheck[--nodesToCheckCount];
		OCTREENODES_QUALIFIER OctreeNode* currentNode = &octreeNodes[currentNodeIndex];
		if (raytracer_intersectBoundingBox(ray, currentNode->boundingBox)) {
			// if we have a inner node we just add all children to the search
			if (currentNode->childNodeIndexes[0] != NODE_INDEX_UNDEF) {
				for (uint32_t i = 0; i < 8; i++) {
					nodesToCheck[nodesToCheckCount++] = currentNode->childNodeIndexes[i];
				}
			// otherwise we have a leaf node
			} else {

				for (uint32_t i = 0; i < currentNode->sphereIndexCount; i++) {
					SPHERES_QUALIFIER Sphere* sphere = &spheres[octreeIndexes[i + currentNode->sphereIndexOffset]];
					float sphereHitDistance = FLT_MAX;
					Vec3 sphereIntersectionNormal;
					if (raytracer_intersectSphere(sphere, ray, &sphereHitDistance, &sphereIntersectionNormal)) {
						if (sphereHitDistance < *minHitDistance) {
							*intersectionNormal = sphereIntersectionNormal;
							*minHitDistance = sphereHitDistance;
							*hitMaterialIndex = sphere->materialIndex;
						}
					}
				}

				for (uint32_t i = 0; i < currentNode->triangleIndexCount; i++) {
					TRIANGLES_QUALIFIER Triangle* triangle = &triangles[octreeIndexes[i + currentNode->triangleIndexOffset]];
					float triangleHitDistance = FLT_MAX;
					Vec3 triangleIntersectionNormal;
					if (raytracer_intersectTriangle(triangle, ray, &triangleHitDistance, &triangleIntersectionNormal)) {
						if (triangleHitDistance < *minHitDistance) {
							*intersectionNormal = triangleIntersectionNormal;
							*minHitDistance = triangleHitDistance;
							*hitMaterialIndex = triangle->materialIndex;
						}
					}
				}
			}
		}
	}
}

static Vec3 raytracer_raycast_helper_0(CAMERA_QUALIFIER Camera* camera, MATERIALS_QUALIFIER Material* materials, uint32_t materialCount, 
	PLANES_QUALIFIER Plane* planes, uint32_t planeCount, SPHERES_QUALIFIER Sphere* spheres, uint32_t sphereCount, 
	TRIANGLES_QUALIFIER Triangle* triangles, uint32_t triangleCount, 
	POINTLIGHTS_QUALIFIER PointLight* pointLights, uint32_t pointLightCount, 
	OCTREENODES_QUALIFIER OctreeNode* octreeNodes, OCTREEINDEX_QUALIFIER uint32_t* octreeIndexes, Ray* primaryRay) {
	Vec3 outColor;
	outColor.r = 0.0f;
	outColor.g = 0.0f;
	outColor.b = 0.0f;
	return outColor;
}

#define DEFINE_RAYCAST_HELPER(X, Y) \
static Vec3 raytracer_raycast_helper_##X(CAMERA_QUALIFIER Camera* camera, MATERIALS_QUALIFIER Material* materials, uint32_t materialCount, \
                                    PLANES_QUALIFIER Plane* planes, uint32_t planeCount, SPHERES_QUALIFIER Sphere* spheres, uint32_t sphereCount, \
									TRIANGLES_QUALIFIER Triangle* triangles, uint32_t triangleCount, POINTLIGHTS_QUALIFIER PointLight* pointLights, uint32_t pointLightCount, \
									OCTREENODES_QUALIFIER OctreeNode* octreeNodes, OCTREEINDEX_QUALIFIER uint32_t* octreeIndexes, Ray* primaryRay) { \
	Vec3 outColor; \
	outColor.r = 0.0f; \
	outColor.g = 0.0f; \
	outColor.b = 0.0f; \
	\
	float minHitDistance = FLT_MAX; \
	uint32_t hitMaterialIndex = 0; \
	Vec3 intersectionNormal; \
	raytracer_calcClosestPlaneIntersect(planes, planeCount, primaryRay, &minHitDistance, &intersectionNormal, &hitMaterialIndex); \
	raytracer_calcClosestIntersectUsingOctree(spheres, sphereCount, triangles, triangleCount, primaryRay, &minHitDistance, &intersectionNormal, &hitMaterialIndex, octreeNodes, octreeIndexes); \
	\
	if (hitMaterialIndex) { \
		MATERIALS_QUALIFIER Material* hitMaterial = &materials[hitMaterialIndex]; \
		/* if we got a hit, calculate the hitPoint and send a shadow rays to each lightsource */ \
		Vec3 hitPoint = raytracer_calculateHitpoint(primaryRay, minHitDistance); \
										\
		/* REFLECTION AND REFRACTION */ \
		if (hitMaterial->refractionIndex > 0) { \
			float kr = raytracer_fresnel(primaryRay->direction, intersectionNormal, hitMaterial->refractionIndex); \
			Vec3 refractionColor; \
			refractionColor.r = 0.0f; \
			refractionColor.g = 0.0f; \
			refractionColor.b = 0.0f; \
			/* compute refraction if it is not a case of total internal reflection */ \
			if (kr < 1) { \
				Ray refractedRay; \
				refractedRay.origin = hitPoint; \
				refractedRay.direction = raytracer_refract(primaryRay->direction, intersectionNormal, hitMaterial->refractionIndex); \
				raytracer_moveRayOutOfObject(&refractedRay); \
				refractionColor = raytracer_raycast_helper_##Y(camera, materials, materialCount, planes, planeCount, spheres, sphereCount, triangles, triangleCount, pointLights, pointLightCount, octreeNodes, octreeIndexes, &refractedRay); \
			} \
			\
			Ray reflectedRay; \
			reflectedRay.origin = hitPoint; \
			reflectedRay.direction = vec3_reflect(primaryRay->direction, intersectionNormal); \
			raytracer_moveRayOutOfObject(&reflectedRay); \
			Vec3 reflectionColor = raytracer_raycast_helper_##Y(camera, materials, materialCount, planes, planeCount, spheres, sphereCount, triangles, triangleCount, pointLights, pointLightCount, octreeNodes, octreeIndexes, &reflectedRay); \
			/* mix the two */ \
			outColor = vec3_add(outColor, vec3_add(vec3_mul(reflectionColor, kr), vec3_mul(refractionColor, (1 - kr)))); \
		} else \
			/* REFLECTION: */ \
		if (hitMaterial->reflectionIndex > 0) { \
			Ray reflectedRay; \
			reflectedRay.origin = hitPoint; \
			reflectedRay.direction = vec3_reflect(primaryRay->direction, intersectionNormal); \
			raytracer_moveRayOutOfObject(&reflectedRay); \
			Vec3 reflectionColor = raytracer_raycast_helper_##Y(camera, materials, materialCount, planes, planeCount, spheres, sphereCount, triangles, triangleCount, pointLights, pointLightCount, octreeNodes, octreeIndexes, &reflectedRay); \
			outColor = vec3_add(outColor, vec3_mul(reflectionColor, hitMaterial->reflectionIndex)); \
		} \
			\
		/* SHADOWS */ \
		for (uint32_t i = 0; i < pointLightCount; i++) { \
			POINTLIGHTS_QUALIFIER PointLight* pointLight = &pointLights[i]; \
			Ray shadowRay; \
			Vec3 hitToLight = vec3_sub(pointLight->position, hitPoint); \
			/* Vec3 randomOffset = vec3_norm((Vec3) { random_bilateral(), random_bilateral(), random_bilateral()}); */ \
			/* hitToLight = vec3_add(hitToLight, randomOffset); */ \
			float distanceToLight = vec3_length(hitToLight); \
			shadowRay.origin = hitPoint; \
			shadowRay.direction = vec3_norm(hitToLight); \
			raytracer_moveRayOutOfObject(&shadowRay); \
			uint32_t shadowRayHitMaterialIndex = 0; \
			float closestHitDistance = FLT_MAX; \
			Vec3 shadowRayIntersectionNormal; \
			raytracer_calcClosestPlaneIntersect(planes, planeCount, &shadowRay, &closestHitDistance, &shadowRayIntersectionNormal, &shadowRayHitMaterialIndex); \
			raytracer_calcClosestIntersectUsingOctree(spheres, sphereCount, triangles, triangleCount, &shadowRay, &closestHitDistance, &shadowRayIntersectionNormal, &shadowRayHitMaterialIndex, octreeNodes, octreeIndexes); \
			if (distanceToLight < closestHitDistance) { \
				/* we hit the light */ \
				float cosAngle = vec3_dot(shadowRay.direction, intersectionNormal); \
				cosAngle = math_clamp(cosAngle, 0.0f, 1.0f); \
				float lightStrength = (pointLight->strength / (4 * PI * distanceToLight * distanceToLight)); \
				Vec3 diffuseLighting = vec3_mul(pointLight->emissionColor, cosAngle * lightStrength); \
				Vec3 toView = vec3_norm(vec3_sub(camera->position, hitPoint)); \
				Vec3 toLight = vec3_mul(shadowRay.direction, -1); \
				Vec3 reflectionVector = vec3_reflect(toLight, intersectionNormal); \
				cosAngle = vec3_dot(toView, reflectionVector); \
				cosAngle = pow(cosAngle, 64); \
				Vec3 specularLighting = vec3_mul(pointLight->emissionColor, cosAngle * lightStrength); \
				outColor = vec3_add(outColor, vec3_mul(vec3_add(diffuseLighting, specularLighting), (1 - hitMaterial->reflectionIndex))); \
			} \
		} \
		outColor = vec3_hadamard(outColor, hitMaterial->color); \
	} \
	return outColor; \
} \

DEFINE_RAYCAST_HELPER(1, 0);
DEFINE_RAYCAST_HELPER(2, 1);
DEFINE_RAYCAST_HELPER(3, 2);
DEFINE_RAYCAST_HELPER(4, 3);
DEFINE_RAYCAST_HELPER(5, 4);

Vec3 raytracer_raycast(CAMERA_QUALIFIER Camera* camera, MATERIALS_QUALIFIER Material* materials, uint32_t materialCount, 
	PLANES_QUALIFIER Plane* planes, uint32_t planeCount, SPHERES_QUALIFIER Sphere* spheres, uint32_t sphereCount, 
	TRIANGLES_QUALIFIER Triangle* triangles, uint32_t triangleCount, POINTLIGHTS_QUALIFIER PointLight* pointLights, uint32_t pointLightCount, 
	OCTREENODES_QUALIFIER OctreeNode* octreeNodes, OCTREEINDEX_QUALIFIER uint32_t* octreeIndexes, Ray* primaryRay) {
    return raytracer_raycast_helper_5(camera, materials, materialCount, planes, planeCount, spheres, sphereCount, triangles, triangleCount, pointLights, pointLightCount, octreeNodes, octreeIndexes, primaryRay);
}


__kernel void raytrace(__global Camera* camera, __local Camera* sharedCamera, __global Material* materials, __local Material* sharedMaterials, uint32_t materialCount,
	__global Plane* planes, __local Plane* sharedPlanes, uint32_t planeCount, __global Sphere* spheres, __local Sphere* sharedSpheres, uint32_t sphereCount,
	__global Triangle* triangles, __local Triangle* sharedTriangles, uint32_t triangleCount,
	__global PointLight* pointLights, __local PointLight* sharedPointLights, uint32_t pointLightCount,
	__global OctreeNode* octreeNodes, __local OctreeNode* sharedOctreeNodes, uint32_t octreeNodeCount,
	__global uint32_t* octreeIndexes, __local uint32_t* sharedOctreeIndexes, uint32_t octreeIndexCount,
	__write_only image2d_t image, float rayColorContribution, float deltaX, float deltaY,
	float pixelWidth, float pixelHeight, uint32_t raysPerWidthPixel, uint32_t raysPerHeightPixel) {

	// copy global data into local shared memory
#ifdef USE_SHARED_MEMORY
	if (get_local_id(0)) {
#ifdef USE_SHARED_MEMORY_CAMERA
		*sharedCamera = *camera;
#define camera sharedCamera
#endif

#ifdef USE_SHARED_MEMORY_MATERIALS
		for (uint32_t i = 0; i < materialCount; i++) {
			sharedMaterials[i] = materials[i];
		}
#define materials sharedMaterials
#endif

#ifdef USE_SHARED_MEMORY_PLANES
		for (uint32_t i = 0; i < planeCount; i++) {
			sharedPlanes[i] = planes[i];
		}
#define planes sharedPlanes
#endif

#ifdef USE_SHARED_MEMORY_SPHERES
		for (uint32_t i = 0; i < sphereCount; i++) {
			sharedSpheres[i] = spheres[i];
		}
#define spheres sharedSpheres
#endif

#ifdef USE_SHARED_MEMORY_TRIANGLES
		for (uint32_t i = 0; i < triangleCount; i++) {
			sharedTriangles[i] = triangles[i];
		}
#define triangles sharedTriangles
#endif

#ifdef USE_SHARED_MEMORY_POINTLIGHTS
		for (uint32_t i = 0; i < pointLightCount; i++) {
			sharedPointLights[i] = pointLights[i];
		}
#define pointLights sharedPointLights
#endif

#ifdef USE_SHARED_MEMORY_OCTREENODES
		for (uint32_t i = 0; i < octreeNodeCount; i++) {
			sharedOctreeNodes[i] = octreeNodes[i];
		}
#define octreeNodes sharedOctreeNodes
#endif

#ifdef USE_SHARED_MEMORY_OCTREEINDEXES
		for (uint32_t i = 0; i < octreeIndexCount; i++) {
			sharedOctreeIndexes[i] = octreeIndexes[i];
		}
#define octreeIndexes sharedOctreeIndexes
#endif
		barrier(CLK_LOCAL_MEM_FENCE);
	}
#endif

	uint32_t x = get_global_id(0);
	uint32_t width = camera->width;
	uint32_t y = get_global_id(1);
	float PosX = -1.0f + 2.0f * ((float)x / (camera->width));
	float PosY = -1.0f + 2.0f * ((float)y / (camera->height));
	Vec3 color;
	color.r = 0.0f;
	color.g = 0.0f;
	color.b = 0.0f;
	// Supersampling loops
	for (uint32_t j = 0; j < raysPerHeightPixel; j++) {
		Vec3 OffsetY = vec3_mul(camera->y,
			(PosY - pixelHeight + j * deltaY)*camera->renderTargetHeight / 2.0f);
		for (uint32_t i = 0; i < raysPerWidthPixel; i++) {
			Vec3 OffsetX = vec3_mul(camera->x,
				(PosX - pixelWidth + i * deltaX)*camera->renderTargetWidth / 2.0f);
			// (0, 0) is the top left
			// so we have to sample the texture with flipped y
			Vec3 renderTargetPos = vec3_sub(vec3_add(camera->renderTargetCenter, OffsetX), OffsetY);
			Ray ray = {
				camera->position,
				vec3_norm(vec3_sub(renderTargetPos, camera->position))
			};

			Vec3 currentRayColor = raytracer_raycast(camera, materials, materialCount, planes, planeCount, spheres, sphereCount, triangles, triangleCount, pointLights, pointLightCount, octreeNodes, octreeIndexes, &ray);
			color = vec3_add(color, vec3_mul(currentRayColor, rayColorContribution));
		}
	}

	// currently values are clamped to [0,1]
	// in the future we may return floats > 1.0
	// and use hdr to map it back to the [0.0, 1.0] range after
	// all pixels are calculated
	// this would avoid that really bright areas look the same
	color = vec3_clamp(color, 0.0f, 1.0f);
	int2 pixelcoord;
	pixelcoord.x = x;
	pixelcoord.y = y;

	float4 pixel = (float4) (color.r, color.g, color.b, 1.0f);
	write_imagef(image, pixelcoord, pixel);
}
