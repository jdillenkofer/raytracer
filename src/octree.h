#ifndef RAYTRACER_OCTREE_H
#define RAYTRACER_OCTREE_H

#include "scene.h"
#include "utils/vec3.h"

#define MAX_ELEMENTS_PER_NODE 32
#define NODE_INDEX_UNDEF -1

typedef struct {
	Vec3 bottomLeftFrontCorner;
	Vec3 topRightBackCorner;
} BoundingBox;

typedef struct {
	BoundingBox boundingBox;

	uint32_t sphereIndexes[MAX_ELEMENTS_PER_NODE];
	uint32_t sphereIndexCount;

	uint32_t triangleIndexes[MAX_ELEMENTS_PER_NODE];
	uint32_t triangleIndexCount;

	int32_t childNodeIndexes[8];
} OctreeNode;

typedef struct {
	OctreeNode* nodes;
	uint32_t nodeCount;
	uint32_t nodeCapacity;
} Octree;

Octree* octree_buildFromScene(Scene* scene);
void octree_delete(Octree* octree);

#endif //RAYTRACER_OCTREE_H