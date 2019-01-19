#include "scene.h"

#include <stdlib.h>

#define DEFAULT_CAPACITY 200

Scene* scene_create(void) {
    Scene* scene = malloc(sizeof(Scene));
    scene->materialCapacity = DEFAULT_CAPACITY;
    scene->materialCount = 0;
    scene->materials = malloc(sizeof(Material) * scene->materialCapacity);

    scene->planeCapacity = DEFAULT_CAPACITY;
    scene->planeCount = 0;
    scene->planes = malloc(sizeof(Plane) * scene->planeCapacity);

    scene->sphereCapacity = DEFAULT_CAPACITY;
    scene->sphereCount = 0;
    scene->spheres = malloc(sizeof(Sphere) * scene->sphereCapacity);

    scene->triangleCapacity = DEFAULT_CAPACITY;
    scene->triangleCount = 0;
    scene->triangles = malloc(sizeof(Triangle) * scene->triangleCapacity);

    scene->pointLightCapacity = DEFAULT_CAPACITY;
    scene->pointLightCount = 0;
    scene->pointLights = malloc(sizeof(PointLight) * scene->pointLightCapacity);

    return scene;
}

Scene* scene_init(uint32_t width, uint32_t height) {
	Scene* scene = scene_create();
	{
		Vec3 camera_pos = { 40.0f , 2.0f, 0.0f };
		Vec3 lookAt = (Vec3) { 0.0f, 0.0f, 0.0f };
		float FOV = 110.0f;
		Camera* camera = camera_create(camera_pos, lookAt, width, height, FOV);
		scene->camera = camera;

		// background has to be added first
		Material background = { 0 };
		background.color = (Vec3) { 0.0f, 0.0f, 0.0f };
		background.reflectionIndex = 0.0f;
		background.refractionIndex = 0.0f;
        background.ambientWeight = 0.0f;
        background.diffuseWeight = 0.0f;
        background.specularWeight = 0.0f;
        background.specularExponent = 1.0f;
		scene_addMaterial(scene, background);

		Material grey = { 0 };
		grey.color = (Vec3) { 0.4f, 0.4f, 0.4f };
		grey.reflectionIndex = 0.0f;
        grey.ambientWeight = 1.0f;
        grey.diffuseWeight = 0.0f;
        grey.specularWeight = 0.0f;
        grey.specularExponent = 1.0f;
		uint32_t greyId = scene_addMaterial(scene, grey);

		Material redMirror = { 0 };
		redMirror.color = (Vec3) { 1.0f, 0.0f, 0.0f };
		redMirror.reflectionIndex = 1.0f;
        redMirror.ambientWeight = 0.2f;
        redMirror.diffuseWeight = 1.0f;
        redMirror.specularWeight = 1.0f;
        redMirror.specularExponent = 64.0f;
		uint32_t redMirrorId = scene_addMaterial(scene, redMirror);

		Material mirror = { 0 };
		mirror.color = (Vec3) { 1.0f, 1.0f, 1.0f };
		mirror.reflectionIndex = 1.0f;
        mirror.ambientWeight = 0.2f;
        mirror.diffuseWeight = 1.0f;
        mirror.specularWeight = 1.0f;
        mirror.specularExponent = 64.0f;
		uint32_t mirrorId = scene_addMaterial(scene, mirror);

		Material glass = { 0 };
		glass.color = (Vec3) { 1.0f, 1.0f, 1.0f };
		glass.reflectionIndex = 1.0f;
		glass.refractionIndex = 1.4f;
        glass.ambientWeight = 0.0f;
        glass.diffuseWeight = 0.0f;
        glass.specularWeight = 0.0f;
        glass.specularExponent = 1.0f;
		uint32_t glassId = scene_addMaterial(scene, glass);

		Material yellow = { 0 };
		yellow.color = (Vec3) { 1.0f, 0.6549f, 0.1019f };
        yellow.ambientWeight = 0.2f;
        yellow.diffuseWeight = 1.0f;
        yellow.specularWeight = 1.0f;
        yellow.specularExponent = 64.0f;
		uint32_t yellowId = scene_addMaterial(scene, yellow);

		Plane floor = { 0 };
		floor.materialIndex = greyId;
		floor.normal = (Vec3) { 0.0f, 1.0f, 0.0f };
		floor.distanceFromOrigin = 0;
		scene_addPlane(scene, floor);

		Plane front = { 0 };
		front.materialIndex = greyId;
		front.normal = (Vec3) { 0.0f, 0.0f, 1.0f };
		front.distanceFromOrigin = 50;
		scene_addPlane(scene, front);

		Plane back = { 0 };
		back.materialIndex = greyId;
		back.normal = (Vec3) { 0.0f, 0.0f, 1.0f };
		back.distanceFromOrigin = -50;
		scene_addPlane(scene, back);

		Plane left = { 0 };
		left.materialIndex = greyId;
		left.normal = (Vec3) { 1.0f, 0.0f, 0.0f };
		left.distanceFromOrigin = -50;
		scene_addPlane(scene, left);

		Plane right = { 0 };
		right.materialIndex = greyId;
		right.normal = (Vec3) { 1.0f, 0.0f, 0.0f };
		right.distanceFromOrigin = 50;
		scene_addPlane(scene, right);

		Sphere redLeftSphere = { 0 };
		redLeftSphere.materialIndex = redMirrorId;
		redLeftSphere.position = (Vec3) { -3.0f, 1.0f, 0.0f };
		redLeftSphere.radius = 1;
		scene_addSphere(scene, redLeftSphere);

		Sphere mirrorSphere = { 0 };
		mirrorSphere.materialIndex = mirrorId;
		mirrorSphere.position = (Vec3) { 0.0f, 1.5f, 0.0f };
		mirrorSphere.radius = 1;
		scene_addSphere(scene, mirrorSphere);

		Sphere glassSphere = { 0 };
		glassSphere.materialIndex = glassId;
		glassSphere.position = (Vec3) { 3.0f, 1.0f, 3.0f };
		glassSphere.radius = 1;
		scene_addSphere(scene, glassSphere);

		Triangle triangle = { 0 };
		triangle.materialIndex = redMirrorId;
		triangle.v0 = (Vec3) { 2.0f, 0.0f, 0.0f };
		triangle.v1 = (Vec3) { 4.0f, 0.0f, 0.0f };
		triangle.v2 = (Vec3) { 3.0f, 1.0f, 0.0f };
		scene_addTriangle(scene, triangle);

		PointLight pointLight = { 0 };
		pointLight.position = (Vec3) { 0.0f, 20.0f, 10.0f };
		pointLight.emissionColor = (Vec3) { 1.0f, 1.0f, 1.0f };
		pointLight.strength = 10000.0f;
		scene_addPointLight(scene, pointLight);

		/*
				Object* teapot = object_loadFromFile("teapot.obj");
				object_scale(teapot, 0.01f);
				object_translate(teapot, (Vec3) { 3.0f, 1.0f, 5.0f });
				object_materialIndex(teapot, redMirrorId);
				scene_addObject(scene, *teapot);
				object_destroy(teapot);

				Object* cube = object_loadFromFile("cube.obj");
				object_translate(cube, (Vec3) { -3.0f, 1.0f, 5.0f });
				object_materialIndex(cube, mirrorId);
				scene_addObject(scene, *cube);
				object_destroy(cube);

				Object* airboat = object_loadFromFile("airboat.obj");
				object_scale(airboat, 0.3f);
				object_translate(airboat, (Vec3) { 0.0f, 1.0f, 5.0f });
				object_materialIndex(airboat, yellowId);
				scene_addObject(scene, *airboat);
				object_destroy(airboat);

				Object* cessna = object_loadFromFile("cessna.obj");
				object_scale(cessna, 0.08f);
				object_translate(cessna, (Vec3) { 0.0f, 1.0f, 5.0f });
				object_materialIndex(cessna, yellowId);
				scene_addObject(scene, *cessna);
				object_destroy(cessna);
		*/
		scene_shrinkToFit(scene);
	}
	return scene;
}

uint32_t scene_addMaterial(Scene* scene, Material material) {
    if (scene->materialCapacity < scene->materialCount + 1) {
        scene->materialCapacity *= 2;
        scene->materials = realloc(scene->materials, sizeof(Material) * scene->materialCapacity);
    }
    uint32_t materialId = scene->materialCount++;
    scene->materials[materialId] = material;
    return materialId;
}

void scene_addPlane(Scene* scene, Plane plane) {
    if (scene->planeCapacity < scene->planeCount + 1) {
        scene->planeCapacity *= 2;
        scene->planes = realloc(scene->planes, sizeof(Plane) * scene->planeCapacity);
    }
    scene->planes[scene->planeCount++] = plane;
}

void scene_addSphere(Scene* scene, Sphere sphere) {
    if (scene->sphereCapacity < scene->sphereCount + 1) {
        scene->sphereCapacity *= 2;
        scene->spheres = realloc(scene->spheres, sizeof(Sphere) * scene->sphereCapacity);
    }
    scene->spheres[scene->sphereCount++] = sphere;
}

void scene_addTriangle(Scene* scene, Triangle triangle) {
    if (scene->triangleCapacity < scene->triangleCount + 1) {
        scene->triangleCapacity *= 2;
        scene->triangles = realloc(scene->triangles, sizeof(Triangle) * scene->triangleCapacity);
    }
    scene->triangles[scene->triangleCount++] = triangle;
}

void scene_addObject(Scene* scene, Object object) {
    for (uint32_t i = 0; i < object.triangleCount; i++) {
        Triangle triangle = object.triangles[i];
        scene_addTriangle(scene, triangle);
    }
}

void scene_addPointLight(Scene* scene, PointLight pointLight) {
    if (scene->pointLightCapacity < scene->pointLightCount + 1) {
        scene->pointLightCapacity *= 2;
        scene->pointLights = realloc(scene->pointLights, sizeof(PointLight) * scene->triangleCapacity);
    }
    scene->pointLights[scene->pointLightCount++] = pointLight;
}

void scene_shrinkToFit(Scene *scene) {
    if (scene->materialCapacity > scene->materialCount) {
        scene->materials = realloc(scene->materials, sizeof(Material) * scene->materialCount);
        scene->materialCapacity = scene->materialCount;
    }
    if (scene->planeCapacity > scene->planeCount) {
        scene->planes = realloc(scene->planes, sizeof(Plane) * scene->planeCount);
        scene->planeCapacity = scene->planeCount;
    }
    if (scene->sphereCapacity > scene->sphereCount) {
        scene->spheres = realloc(scene->spheres, sizeof(Sphere) * scene->sphereCount);
        scene->sphereCapacity = scene->sphereCount;
    }
    if (scene->triangleCapacity > scene->triangleCount) {
        scene->triangles = realloc(scene->triangles, sizeof(Triangle) * scene->triangleCount);
        scene->triangleCapacity = scene->triangleCount;
    }
    if (scene->pointLightCapacity > scene->pointLightCount) {
        scene->pointLights = realloc(scene->pointLights, sizeof(PointLight) * scene->pointLightCount);
        scene->pointLightCapacity = scene->pointLightCount;
    }
}

void scene_destroy(Scene* scene) {
    if (scene) {
        camera_destroy(scene->camera);
        free(scene->materials);
        free(scene->planes);
        free(scene->spheres);
        free(scene->triangles);
        free(scene->pointLights);
        free(scene);
    }
}
