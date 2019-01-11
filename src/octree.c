#include "octree.h"

#include <stdlib.h>
#include <float.h>

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
}

static uint32_t octree_buildNode(Octree* octree, Scene* scene, 
	int32_t* sphereIndexes, uint32_t sphereIndexCount, 
	int32_t* triangleIndexes, uint32_t triangleIndexCount, 
	BoundingBox boundingBox) {
	if (octree->nodeCount + 1 > octree->nodeCapacity) {
		octree->nodeCapacity *= 2;
		octree->nodes = realloc(octree->nodes, sizeof(OctreeNode) * octree->nodeCapacity);
	}
	int32_t parentNodeId = octree->nodeCount - 1;
	int32_t currentNodeId = octree->nodeCount++;
	OctreeNode* currentNode = &octree->nodes[currentNodeId];

	currentNode->boundingBox = boundingBox;

	uint32_t sphereElementsInside = 0;
	uint32_t sphereElementsCapacity = 100;
	uint32_t* sphereIndexesInside = malloc(sizeof(uint32_t) * sphereElementsCapacity);

	uint32_t triangleElementsInside = 0;
	uint32_t triangleElementsCapacity = 100;
	uint32_t* triangleIndexesInside = malloc(sizeof(uint32_t) * triangleElementsCapacity);

	// TODO: check if the triangle or sphere intersects the boundingbox
	// and add its index to the corresponding array


	// shrink the array to save space
	if (sphereElementsCapacity > sphereElementsInside) {
		sphereIndexesInside = realloc(sphereIndexesInside, sizeof(uint32_t) * sphereElementsInside);
		sphereElementsCapacity = sphereElementsInside;
	}
	if (triangleElementsCapacity > triangleElementsInside) {
		triangleIndexesInside = realloc(triangleIndexesInside, sizeof(uint32_t) * triangleElementsInside);
		triangleElementsCapacity = triangleElementsInside;
	}

	if (sphereElementsInside > MAX_ELEMENTS_PER_NODE || triangleElementsInside > MAX_ELEMENTS_PER_NODE) {
		Vec3 diagonal = vec3_sub(boundingBox.topRightBackCorner, boundingBox.bottomLeftFrontCorner);
		Vec3 halfDiagonal = vec3_mul(diagonal, 0.5f);
		Vec3 centerOfBoundingBox = vec3_add(boundingBox.bottomLeftFrontCorner, halfDiagonal);

		// front bottom left
		BoundingBox bbFrontBottomLeft = (BoundingBox) { boundingBox.bottomLeftFrontCorner, centerOfBoundingBox };
		currentNode->childNodeIndexes[0] = octree_buildNode(octree, scene, sphereIndexesInside, sphereElementsInside, triangleIndexesInside, triangleElementsInside, bbFrontBottomLeft);

		// front bottom right
		BoundingBox bbFrontBottomRight = 
			(BoundingBox) { 
				vec3_add(boundingBox.bottomLeftFrontCorner, (Vec3) { halfDiagonal.x, 0.0f, 0.0f }), 
				vec3_add(centerOfBoundingBox, (Vec3) { halfDiagonal.x, 0.0f, 0.0f })
			};
		currentNode->childNodeIndexes[1] = octree_buildNode(octree, scene, sphereIndexesInside, sphereElementsInside, triangleIndexesInside, triangleElementsInside, bbFrontBottomRight);

		// back bottom left
		BoundingBox bbBackBottomLeft =
			(BoundingBox) {
				vec3_add(boundingBox.bottomLeftFrontCorner, (Vec3) { 0.0f, halfDiagonal.y, 0.0f }),
				vec3_add(centerOfBoundingBox, (Vec3) { 0.0f, halfDiagonal.y, 0.0f })
			};
		currentNode->childNodeIndexes[2] = octree_buildNode(octree, scene, sphereIndexesInside, sphereElementsInside, triangleIndexesInside, triangleElementsInside, bbBackBottomLeft);

		// back bottom right
		BoundingBox bbBackBottomRight =
			(BoundingBox) {
				vec3_add(centerOfBoundingBox, (Vec3) { 0.0f, -halfDiagonal.y, 0.0f }),
				vec3_add(centerOfBoundingBox, (Vec3) { halfDiagonal.x, 0.0f, -halfDiagonal.z })
			};
		currentNode->childNodeIndexes[3] = octree_buildNode(octree, scene, sphereIndexesInside, sphereElementsInside, triangleIndexesInside, triangleElementsInside, bbBackBottomRight);

		// front top left
		BoundingBox bbFrontTopLeft =
			(BoundingBox) {
				vec3_add(centerOfBoundingBox, (Vec3) { -halfDiagonal.x, 0.0f, halfDiagonal.z }),
				vec3_add(centerOfBoundingBox, (Vec3) { 0.0f, halfDiagonal.y, 0.0f })
			};
		currentNode->childNodeIndexes[4] = octree_buildNode(octree, scene, sphereIndexesInside, sphereElementsInside, triangleIndexesInside, triangleElementsInside, bbFrontTopLeft);

		// front top right
		BoundingBox bbFrontTopRight =
			(BoundingBox) {
				vec3_add(centerOfBoundingBox, (Vec3) { 0.0f, 0.0f, halfDiagonal.z }),
				vec3_add(centerOfBoundingBox, (Vec3) { halfDiagonal.x, halfDiagonal.y, 0.0f })
			};
		currentNode->childNodeIndexes[5] = octree_buildNode(octree, scene, sphereIndexesInside, sphereElementsInside, triangleIndexesInside, triangleElementsInside, bbFrontTopRight);

		// back top left
		BoundingBox bbBackTopLeft =
			(BoundingBox) {
				vec3_add(centerOfBoundingBox, (Vec3) { -halfDiagonal.x, 0.0f, 0.0f }),
				vec3_add(centerOfBoundingBox, (Vec3) { 0.0f, halfDiagonal.y, -halfDiagonal.z })
			};
		currentNode->childNodeIndexes[6] = octree_buildNode(octree, scene, sphereIndexesInside, sphereElementsInside, triangleIndexesInside, triangleElementsInside, bbBackTopLeft);

		// back top right
		BoundingBox bbBackTopRight = (BoundingBox) { centerOfBoundingBox, vec3_add(centerOfBoundingBox, halfDiagonal) };
		currentNode->childNodeIndexes[7] = octree_buildNode(octree, scene, sphereIndexesInside, sphereElementsInside, triangleIndexesInside, triangleElementsInside, bbBackTopRight);

		currentNode->sphereIndexCount = 0;
		currentNode->triangleIndexCount = 0;
	} else {
		// we are a leaf node
		// no children indexes
		for (uint32_t i = 0; i < 8; i++) {
			currentNode->childNodeIndexes[i] = NODE_INDEX_UNDEF;
		}
		
		// copy the spheres
		for (uint32_t i = 0; i < sphereElementsInside; i++) {
			currentNode->sphereIndexes[i] = sphereIndexesInside[i];
		}
		currentNode->sphereIndexCount = sphereElementsInside;

		// copy the triangles
		for (uint32_t i = 0; i < triangleElementsInside; i++) {
			currentNode->triangleIndexes[i] = triangleIndexesInside[i];
		}
		currentNode->triangleIndexCount = triangleElementsInside;
	}
	free(sphereIndexesInside);
	free(triangleIndexesInside);
	return currentNodeId;
}

Octree* octree_buildFromScene(Scene* scene) {
	Octree* octree = malloc(sizeof(Octree));
	if (!octree) {
		return NULL;
	}

	octree->nodeCapacity = 1;
	octree->nodeCount = 0;
	octree->nodes = malloc(sizeof(OctreeNode) * octree->nodeCapacity);

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

	octree_buildNode(octree, scene, sphereIndexes, scene->sphereCount, triangleIndexes, scene->triangleCount, rootBoundingBox);
	
	free(sphereIndexes);
	free(triangleIndexes);
	
	octree_shrinkToFit(octree);
	return octree;
}

void octree_delete(Octree* octree) {
	if (octree) {
		free(octree->nodes);
		free(octree);
	}
}