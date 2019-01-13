#ifndef RAYTRACER_OCTREE_H
#define RAYTRACER_OCTREE_H

#include "scene.h"
#include "utils/vec3.h"

#define MIN_ELEMENTS_PER_NODE 8
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

typedef struct {
	OctreeNode* nodes;
	uint32_t nodeCount;
	uint32_t nodeCapacity;
	uint32_t* indexes;
	uint32_t indexCount;
	uint32_t indexCapacity;
} Octree;

Octree* octree_buildFromScene(Scene* scene);
void octree_destroy(Octree* octree);

#endif //RAYTRACER_OCTREE_H