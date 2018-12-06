#include <float.h>
#include "utils/image.h"
#include "utils/vec3.h"
#include "camera.h"
#include "scene.h"
#include "raytracer.h"

#include <omp.h>

int main(int argc, char* argv[]) {
    (void) argc;
    (void) argv;
    Vec3 camera_pos = { 0.0f , 40.0f, 2.0f };
    Vec3 lookAt = (Vec3) {0.0f, 0.0f, 0.0f};
    uint32_t width = 1920;
    uint32_t height = 1080;
    double FOV = 110.0f;
	uint32_t raysPerPixel = 64;
    Camera* camera = camera_create(camera_pos, lookAt, width, height, FOV);
    Image* image = image_create(width, height);

    Material materials[5] = {0};
    materials[0].color = (Vec3) { 0.53f, 0.80f, 0.92f };
    materials[1].color = (Vec3) { 0.4f, 0.4f, 0.4f };
    materials[2].color = (Vec3) { 1.0f, 0.0f, 0.0f };
    materials[3].color = (Vec3) { 0.0f, 1.0f, 0.0f };
    materials[4].color = (Vec3) { 0.0f, 0.0f, 1.0f };

    Plane planes[1] = {0};
    planes[0].materialIndex = 1;
    planes[0].normal = (Vec3) { 0.0f, 0.0f, 1.0f };
    planes[0].distanceFromOrigin = 0;

    Sphere spheres[3] = {0};
    spheres[0].materialIndex = 2;
    spheres[0].position = (Vec3) { -3.0f, 0.0f, 1.0f };
    spheres[0].radius = 1;
    spheres[1].materialIndex = 3;
    spheres[1].position = (Vec3) { 0.0f, 0.0f, 1.5f };
    spheres[1].radius = 1;
    spheres[2].materialIndex = 4;
    spheres[2].position = (Vec3) { 3.0f, 0.0f, 2.0f };
    spheres[2].radius = 1;

    PointLight pointLights[2] = {0};
    pointLights[0].position = (Vec3) { 0.0f, 4.0f, 5.f };
    pointLights[0].emissionColor = (Vec3) { 0.5f, 0.5f, 0.5f };
    pointLights[1].position = (Vec3) { 0.0f, 0.0f, 5.f };
    pointLights[1].emissionColor = (Vec3) { 1.0f, 1.0f, 1.0f };

    Scene scene = {0};
    scene.materialCount = 5;
    scene.materials = materials;
    scene.planeCount = 1;
    scene.planes = planes;
    scene.sphereCount = 3;
    scene.spheres = spheres;
    scene.triangleCount = 0;
    scene.triangles = NULL;
    scene.pointLightCount = 2;
    scene.pointLights = pointLights;

	double rayColorContribution = 1.0f / raysPerPixel;

    for (uint32_t y = 0; y < height; y++) {
#ifndef _WINDOWS
        #pragma omp parallel for
#endif
        for (uint32_t x = 0; x < width; x++) {
			Vec3 color = {0};
			for (uint32_t i = 0; i < raysPerPixel; i++) {
				Ray ray = camera_rayFromPixelPos(camera, x, y);
				Vec3 currentRayColor = raytracer_raycast(&scene, &ray);
				color = vec3_add(color, vec3_mul(currentRayColor, rayColorContribution));
			}

			// TODO: clamp values to [0,1]
			image->buffer[y*width + x] =
				(0xFFu << 24) |
				((uint32_t)(color.r * 255) << 16) |
				((uint32_t)(color.g * 255) << 8) |
				((uint32_t)(color.b * 255) << 0);

        }
		printf("\rRaytracing %d%%", (uint32_t)(100 * ((double)(y) / (double)(height))));
    }

    bitmap_save_image("test.bmp", image);

    image_destroy(image);
    camera_destroy(camera);
    return 0;
}
