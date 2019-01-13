#include "octree.h"

#include <stdlib.h>
#include <float.h>
#include <stdbool.h>

static BoundingBox octree_calculateRootBoundingBox(Scene* scene) {
	BoundingBox boundingBox = { 0 };
	for (uint32_t i = 0; i < scene->sphereCount; i++) {
		Sphere* sphere = &scene->spheres[i];
		Vec3 extremePoints[6] = {
			vec3_add(sphere->position, (Vec3) { 0.0f, 0.0f, sphere->radius }), // FRONT
			vec3_add(sphere->position, (Vec3) { 0.0f, 0.0f, -sphere->radius }), // BACK
			vec3_add(sphere->position, (Vec3) { 0.0f, sphere->radius, 0.0f }), // TOP
			vec3_add(sphere->position, (Vec3) { 0.0f, -sphere->radius, 0.0f }), // BOTTOM
			vec3_add(sphere->position, (Vec3) { -sphere->radius, 0.0f, 0.0f }), // LEFT
			vec3_add(sphere->position, (Vec3) { sphere->radius, 0.0f, 0.0f }) // RIGHT
		};
		for (uint32_t i = 0; i < 6; i++) {
			Vec3* v = &extremePoints[i];
			if (v->x < boundingBox.bottomLeftFrontCorner.x) {
				boundingBox.bottomLeftFrontCorner.x = v->x;
			}
			if (v->y < boundingBox.bottomLeftFrontCorner.y) {
				boundingBox.bottomLeftFrontCorner.y = v->y;
			}
			if (v->z < boundingBox.bottomLeftFrontCorner.z) {
				boundingBox.bottomLeftFrontCorner.z = v->z;
			}

			if (v->x > boundingBox.topRightBackCorner.x) {
				boundingBox.topRightBackCorner.x = v->x;
			}
			if (v->y > boundingBox.topRightBackCorner.y) {
				boundingBox.topRightBackCorner.y = v->y;
			}
			if (v->z > boundingBox.topRightBackCorner.z) {
				boundingBox.topRightBackCorner.z = v->z;
			}
		}
	}
	for (uint32_t i = 0; i < scene->triangleCount; i++) {
		Triangle* triangle = &scene->triangles[i];
		Vec3 vertices[3] = { triangle->v0, triangle->v1, triangle->v2 };
		for (uint32_t i = 0; i < 3; i++) {
			Vec3* v = &vertices[i];
			if (v->x < boundingBox.bottomLeftFrontCorner.x) {
				boundingBox.bottomLeftFrontCorner.x = v->x;
			}
			if (v->y < boundingBox.bottomLeftFrontCorner.y) {
				boundingBox.bottomLeftFrontCorner.y = v->y;
			}
			if (v->z < boundingBox.bottomLeftFrontCorner.z) {
				boundingBox.bottomLeftFrontCorner.z = v->z;
			}

			if (v->x > boundingBox.topRightBackCorner.x) {
				boundingBox.topRightBackCorner.x = v->x;
			}
			if (v->y > boundingBox.topRightBackCorner.y) {
				boundingBox.topRightBackCorner.y = v->y;
			}
			if (v->z > boundingBox.topRightBackCorner.z) {
				boundingBox.topRightBackCorner.z = v->z;
			}
		}
	}
	return boundingBox;
}

static void octree_shrinkToFit(Octree* octree) {
	if (octree->nodeCapacity > octree->nodeCount) {
		octree->nodes = realloc(octree->nodes, sizeof(OctreeNode) * octree->nodeCount);
		octree->nodeCapacity = octree->nodeCount;
	}
	if (octree->indexCapacity > octree->indexCount) {
		octree->indexes = realloc(octree->indexes, sizeof(uint32_t) * octree->indexCount);
		octree->indexCapacity = octree->indexCount;
	}
}

static bool octree_intersectSphere(Sphere* sphere, BoundingBox boundingBox) {
	float distSquared = sphere->radius * sphere->radius;
	if (sphere->position.x < boundingBox.bottomLeftFrontCorner.x) {
		distSquared -= (sphere->position.x - boundingBox.bottomLeftFrontCorner.x) * (sphere->position.x - boundingBox.bottomLeftFrontCorner.x);
	}
	else if (sphere->position.x > boundingBox.topRightBackCorner.x) {
		distSquared -= (sphere->position.x - boundingBox.topRightBackCorner.x) * (sphere->position.x - boundingBox.topRightBackCorner.x);
	}

	if (sphere->position.y < boundingBox.bottomLeftFrontCorner.y) {
		distSquared -= (sphere->position.y - boundingBox.bottomLeftFrontCorner.y) * (sphere->position.y - boundingBox.bottomLeftFrontCorner.y);
	}
	else if (sphere->position.y > boundingBox.topRightBackCorner.y) {
		distSquared -= (sphere->position.y - boundingBox.topRightBackCorner.y) * (sphere->position.y - boundingBox.topRightBackCorner.y);
	}

	// @POSSIBLE BUG: i think i may have to swap the condition here
	if (sphere->position.z < boundingBox.bottomLeftFrontCorner.z) {
		distSquared -= (sphere->position.z - boundingBox.bottomLeftFrontCorner.z) * (sphere->position.z - boundingBox.bottomLeftFrontCorner.z);
	}
	else if (sphere->position.z > boundingBox.topRightBackCorner.z) {
		distSquared -= (sphere->position.z - boundingBox.topRightBackCorner.z) * (sphere->position.z - boundingBox.topRightBackCorner.z);
	}

	return distSquared > 0;
}

static void octree_project(Vec3* points, uint32_t pointCount, Vec3 axis, float* min, float* max) {
	*min = INFINITY;
	*max = -INFINITY;
	for (uint32_t i = 0; i < pointCount; i++) {
		Vec3 point = points[i];
		float val = vec3_dot(axis, point);
		if (val < *min) {
			*min = val;
		}
		if (val > *max) {
			*max = val;
		}
	}
}

static bool octree_intersectTriangle(Triangle* triangle, BoundingBox boundingBox) {
	float triangleMin;
	float triangleMax;
	float boxMin;
	float boxMax;

	Vec3 boxNormals[3] = {
		(Vec3) {1.0f, 0.0f, 0.0f},
		(Vec3) {0.0f, 1.0f, 0.0f},
		(Vec3) {0.0f, 0.0f, 1.0f}
	};

	Vec3 triangleVertices[3] = {
		triangle->v0,
		triangle->v1,
		triangle->v2
	};

	float boundingBoxV0[3] = {
		boundingBox.bottomLeftFrontCorner.x,
		boundingBox.bottomLeftFrontCorner.y,
		boundingBox.bottomLeftFrontCorner.z
	};

	float boundingBoxV1[3] = {
		boundingBox.topRightBackCorner.x,
		boundingBox.topRightBackCorner.y,
		boundingBox.topRightBackCorner.z
	};

	for (uint32_t i = 0; i < 3; i++) {
		Vec3 n = boxNormals[i];
		octree_project(triangleVertices, 3, n, &triangleMin, &triangleMax);
		if (triangleMax < boundingBoxV0[i] || triangleMin > boundingBoxV1[i]) {
			return false;
		}
	}

	Vec3 boxVertices[8] = {
		boundingBox.bottomLeftFrontCorner, // bottom left front
		(Vec3) {boundingBox.topRightBackCorner.x, boundingBox.bottomLeftFrontCorner.y, boundingBox.bottomLeftFrontCorner.z}, // bottom right front
		(Vec3) {boundingBox.bottomLeftFrontCorner.x, boundingBox.topRightBackCorner.y, boundingBox.bottomLeftFrontCorner.z}, // top left front
		(Vec3) {boundingBox.topRightBackCorner.x, boundingBox.topRightBackCorner.y, boundingBox.bottomLeftFrontCorner.z}, // top right front
		(Vec3) {boundingBox.bottomLeftFrontCorner.x, boundingBox.bottomLeftFrontCorner.y, boundingBox.topRightBackCorner.z}, // bottom left back
		(Vec3) {boundingBox.topRightBackCorner.x, boundingBox.bottomLeftFrontCorner.y, boundingBox.topRightBackCorner.z}, // bottom right back
		(Vec3) {boundingBox.bottomLeftFrontCorner.x, boundingBox.topRightBackCorner.y, boundingBox.topRightBackCorner.z}, // top left back
		boundingBox.topRightBackCorner // top right back
	};

	Vec3 v0v1 = vec3_sub(triangle->v1, triangle->v0);
	Vec3 v0v2 = vec3_sub(triangle->v2, triangle->v0);
	Vec3 triangleNormal = vec3_norm(vec3_cross(v0v1, v0v2));
	float triangleOffset = vec3_dot(triangleNormal, triangle->v0);

	octree_project(boxVertices, 8, triangleNormal, &boxMin, &boxMax);
	if (boxMax < triangleOffset || boxMin > triangleOffset) {
		return false;
	}

	Vec3 triangleEdges[3] = {
		vec3_sub(triangle->v0, triangle->v1),
		vec3_sub(triangle->v1, triangle->v2),
		vec3_sub(triangle->v2, triangle->v0)
	};

	for (uint32_t i = 0; i < 3; i++) {
		for (uint32_t j = 0; j < 3; j++) {
			Vec3 axis = vec3_cross(triangleEdges[i], boxNormals[j]);
			octree_project(boxVertices, 8, axis, &boxMin, &boxMax);
			octree_project(triangleVertices, 3, axis, &triangleMin, &triangleMax);
			if (boxMax < triangleMin || boxMin > triangleMax) {
				return false;
			}
		}
	}

	return true;
}

static void octree_buildNode(Octree* octree, Scene* scene, int32_t nodeId,
	int32_t* sphereIndexes, uint32_t sphereIndexCount, 
	int32_t* triangleIndexes, uint32_t triangleIndexCount, 
	BoundingBox boundingBox) {
	if (octree->nodeCount + 1 > octree->nodeCapacity) {
		octree->nodeCapacity *= 2;
		octree->nodes = realloc(octree->nodes, sizeof(OctreeNode) * octree->nodeCapacity);
	}

	assert(boundingBox.bottomLeftFrontCorner.x <= boundingBox.topRightBackCorner.x);
	assert(boundingBox.bottomLeftFrontCorner.y <= boundingBox.topRightBackCorner.y);
	assert(boundingBox.bottomLeftFrontCorner.z <= boundingBox.topRightBackCorner.z);

	// we can't save a pointer to the currentNode,
	// because the underlying array may change it's address due to a realloc call
	octree->nodes[nodeId].boundingBox = boundingBox;

	uint32_t sphereElementsInside = 0;
	uint32_t sphereElementsCapacity = 100;
	uint32_t* sphereIndexesInside = malloc(sizeof(uint32_t) * sphereElementsCapacity);

	uint32_t triangleElementsInside = 0;
	uint32_t triangleElementsCapacity = 100;
	uint32_t* triangleIndexesInside = malloc(sizeof(uint32_t) * triangleElementsCapacity);

	for (uint32_t i = 0; i < sphereIndexCount; i++) {
		uint32_t index = sphereIndexes[i];
		Sphere* sphere = &scene->spheres[index];
		if (octree_intersectSphere(sphere, boundingBox)) {
			// add index to sphereIndexesInside
			if (sphereElementsInside + 1 > sphereElementsCapacity) {
				sphereElementsCapacity *= 2;
				sphereIndexesInside = realloc(sphereIndexesInside, sizeof(uint32_t) * sphereElementsCapacity);
			}
			sphereIndexesInside[sphereElementsInside++] = index;
		}
	}

	for (uint32_t i = 0; i < triangleIndexCount; i++) {
		uint32_t index = triangleIndexes[i];
		Triangle* triangle = &scene->triangles[index];
		if (octree_intersectTriangle(triangle, boundingBox)) {
			// add index to triangleIndexesInside
			if (triangleElementsInside + 1 > triangleElementsCapacity) {
				triangleElementsCapacity *= 2;
				triangleIndexesInside = realloc(triangleIndexesInside, sizeof(uint32_t) * triangleElementsCapacity);
			}
			triangleIndexesInside[triangleElementsInside++] = index;
		}
	}

	// shrink the array to save space
	if (sphereElementsCapacity > sphereElementsInside) {
		sphereIndexesInside = realloc(sphereIndexesInside, sizeof(uint32_t) * sphereElementsInside);
		sphereElementsCapacity = sphereElementsInside;
	}
	if (triangleElementsCapacity > triangleElementsInside) {
		triangleIndexesInside = realloc(triangleIndexesInside, sizeof(uint32_t) * triangleElementsInside);
		triangleElementsCapacity = triangleElementsInside;
	}

	// we keep splitting, if our subdivision has changed one of the array sizes
	if ((sphereIndexCount != sphereElementsInside || triangleIndexCount != triangleElementsInside || nodeId == 0) && !(sphereElementsInside < MIN_ELEMENTS_PER_NODE && triangleElementsInside < MIN_ELEMENTS_PER_NODE)) {
		Vec3 diagonal = vec3_sub(boundingBox.topRightBackCorner, boundingBox.bottomLeftFrontCorner);
		Vec3 halfDiagonal = vec3_mul(diagonal, 0.5f);
		Vec3 centerOfBoundingBox = vec3_add(boundingBox.bottomLeftFrontCorner, halfDiagonal);

		int32_t childId1 = octree->nodeCount++;
		int32_t childId2 = octree->nodeCount++;
		int32_t childId3 = octree->nodeCount++;
		int32_t childId4 = octree->nodeCount++;
		int32_t childId5 = octree->nodeCount++;
		int32_t childId6 = octree->nodeCount++;
		int32_t childId7 = octree->nodeCount++;
		int32_t childId8 = octree->nodeCount++;

		octree->nodes[nodeId].childNodeIndexes[0] = childId1;
		octree->nodes[nodeId].childNodeIndexes[1] = childId2;
		octree->nodes[nodeId].childNodeIndexes[2] = childId3;
		octree->nodes[nodeId].childNodeIndexes[3] = childId4;
		octree->nodes[nodeId].childNodeIndexes[4] = childId5;
		octree->nodes[nodeId].childNodeIndexes[5] = childId6;
		octree->nodes[nodeId].childNodeIndexes[6] = childId7;
		octree->nodes[nodeId].childNodeIndexes[7] = childId8;

		// front bottom left
		BoundingBox bbFrontBottomLeft = (BoundingBox) { boundingBox.bottomLeftFrontCorner, centerOfBoundingBox };
		octree_buildNode(octree, scene, childId1, sphereIndexesInside, sphereElementsInside, triangleIndexesInside, triangleElementsInside, bbFrontBottomLeft);

		// front bottom right
		BoundingBox bbFrontBottomRight = 
			(BoundingBox) { 
				vec3_add(boundingBox.bottomLeftFrontCorner, (Vec3) { halfDiagonal.x, 0.0f, 0.0f }), 
				vec3_add(centerOfBoundingBox, (Vec3) { halfDiagonal.x, 0.0f, 0.0f })
			};
		octree_buildNode(octree, scene, childId2, sphereIndexesInside, sphereElementsInside, triangleIndexesInside, triangleElementsInside, bbFrontBottomRight);

		// back bottom left
		BoundingBox bbBackBottomLeft =
			(BoundingBox) {
				vec3_add(boundingBox.bottomLeftFrontCorner, (Vec3) { 0.0f, 0.0f, halfDiagonal.z }),
				vec3_add(centerOfBoundingBox, (Vec3) { 0.0f, 0.0f, halfDiagonal.z })
			};
		octree_buildNode(octree, scene, childId3, sphereIndexesInside, sphereElementsInside, triangleIndexesInside, triangleElementsInside, bbBackBottomLeft);

		// back bottom right
		BoundingBox bbBackBottomRight =
			(BoundingBox) {
				vec3_add(boundingBox.bottomLeftFrontCorner, (Vec3) { halfDiagonal.x, 0.0f, halfDiagonal.z }),
				vec3_add(centerOfBoundingBox, (Vec3) { halfDiagonal.x, 0.0f, halfDiagonal.z })
			};
		octree_buildNode(octree, scene, childId4, sphereIndexesInside, sphereElementsInside, triangleIndexesInside, triangleElementsInside, bbBackBottomRight);

		// front top left
		BoundingBox bbFrontTopLeft =
			(BoundingBox) {
				vec3_add(boundingBox.bottomLeftFrontCorner, (Vec3) { 0.0f, halfDiagonal.y, 0.0f }),
				vec3_add(centerOfBoundingBox, (Vec3) { 0.0f, halfDiagonal.y, 0.0f })
			};
		octree_buildNode(octree, scene, childId5, sphereIndexesInside, sphereElementsInside, triangleIndexesInside, triangleElementsInside, bbFrontTopLeft);

		// front top right
		BoundingBox bbFrontTopRight =
			(BoundingBox) {
				vec3_add(boundingBox.bottomLeftFrontCorner, (Vec3) { halfDiagonal.x, halfDiagonal.y, 0.0f }),
				vec3_add(centerOfBoundingBox, (Vec3) { halfDiagonal.x, halfDiagonal.y, 0.0f })
			};
		octree_buildNode(octree, scene, childId6, sphereIndexesInside, sphereElementsInside, triangleIndexesInside, triangleElementsInside, bbFrontTopRight);

		// front - back +
		// back top left
		BoundingBox bbBackTopLeft =
			(BoundingBox) {
				vec3_add(boundingBox.bottomLeftFrontCorner, (Vec3) { 0.0f, 0.0f, halfDiagonal.z }),
				vec3_add(centerOfBoundingBox, (Vec3) { 0.0f, halfDiagonal.y, halfDiagonal.z })
			};
		octree_buildNode(octree, scene, childId7, sphereIndexesInside, sphereElementsInside, triangleIndexesInside, triangleElementsInside, bbBackTopLeft);

		// back top right
		BoundingBox bbBackTopRight = (BoundingBox) { centerOfBoundingBox, boundingBox.topRightBackCorner };
		octree_buildNode(octree, scene, childId8, sphereIndexesInside, sphereElementsInside, triangleIndexesInside, triangleElementsInside, bbBackTopRight);

		octree->nodes[nodeId].sphereIndexCount = 0;
		octree->nodes[nodeId].triangleIndexCount = 0;
	} else {
		// here we can temporarily take a pointer to the array, because
		// we don't call octree_buildNode here.
		// so no reallocations are happening in between usages
		OctreeNode* currentNode = &octree->nodes[nodeId];

		// we are a leaf node
		// no children indexes
		for (uint32_t i = 0; i < 8; i++) {
			currentNode->childNodeIndexes[i] = NODE_INDEX_UNDEF;
		}
		
		// realloc index array, if too small
		if (octree->indexCount + sphereElementsInside + triangleElementsInside > octree->indexCapacity) {
			octree->indexCapacity = octree->indexCapacity + octree->indexCount + sphereElementsInside + triangleElementsInside;
			octree->indexes = realloc(octree->indexes, sizeof(uint32_t) * octree->indexCapacity);
		}

		// copy the spheres
		currentNode->sphereIndexOffset = octree->indexCount;
		currentNode->sphereIndexCount = sphereElementsInside;
		for (uint32_t i = 0; i < sphereElementsInside; i++) {
			octree->indexes[i + octree->indexCount] = sphereIndexesInside[i];
		}
		octree->indexCount += sphereElementsInside;

		// copy the triangles
		currentNode->triangleIndexOffset = octree->indexCount;
		currentNode->triangleIndexCount = triangleElementsInside;
		for (uint32_t i = 0; i < triangleElementsInside; i++) {
			octree->indexes[i + octree->indexCount] = triangleIndexesInside[i];
		}
		octree->indexCount += triangleElementsInside;
	}
	free(sphereIndexesInside);
	free(triangleIndexesInside);
}

Octree* octree_buildFromScene(Scene* scene) {
	Octree* octree = malloc(sizeof(Octree));
	if (!octree) {
		return NULL;
	}

	octree->nodeCapacity = 1;
	octree->nodeCount = 0;
	octree->nodes = malloc(sizeof(OctreeNode) * octree->nodeCapacity);
	octree->indexCapacity = 1;
	octree->indexCount= 0;
	octree->indexes = malloc(sizeof(uint32_t) * octree->indexCapacity);

	BoundingBox rootBoundingBox = octree_calculateRootBoundingBox(scene);
	
	// this array indicates which elements with which index are inside the boundingBox of the parent
	// for the root node this should be all elements
	uint32_t* sphereIndexes = malloc(sizeof(uint32_t) * scene->sphereCount);
	for (uint32_t i = 0; i < scene->sphereCount; i++) {
		sphereIndexes[i] = i;
	}
	uint32_t* triangleIndexes = malloc(sizeof(uint32_t) * scene->triangleCount);
	for (uint32_t i = 0; i < scene->triangleCount; i++) {
		triangleIndexes[i] = i;
	}

	int32_t rootId = octree->nodeCount++;
	assert(rootId == 0);
	octree_buildNode(octree, scene, rootId, sphereIndexes, scene->sphereCount, triangleIndexes, scene->triangleCount, rootBoundingBox);
	
	free(sphereIndexes);
	free(triangleIndexes);
	
	octree_shrinkToFit(octree);
	return octree;
}

void octree_destroy(Octree* octree) {
	if (octree) {
		free(octree->nodes);
		free(octree->indexes);
		free(octree);
	}
}