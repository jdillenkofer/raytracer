#include <float.h>
#include "utils/image.h"
#include "utils/vec3.h"
#include "camera.h"
#include "scene.h"
#include "raytracer.h"

#include <omp.h>
#include <utils/random.h>

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
    materials[0].color = (Vec3) { 0.0f, 0.0f, 0.0f };
    materials[1].color = (Vec3) { 0.4f, 0.4f, 0.4f };
    materials[2].color = (Vec3) { 1.0f, 0.0f, 0.0f };
    materials[3].color = (Vec3) { 0.0f, 1.0f, 0.0f };
    materials[4].color = (Vec3) { 0.0f, 0.0f, 1.0f };

    Plane planes[2] = {0};
    planes[0].materialIndex = 1;
    planes[0].normal = (Vec3) { 0.0f, 0.0f, 1.0f };
    planes[0].distanceFromOrigin = 0;
    planes[1].materialIndex = 1;
    planes[1].normal = (Vec3) { 0.0f, -1.0f, 0.0f };
    planes[1].distanceFromOrigin = -4;

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
    pointLights[1].position = (Vec3) { 0.0f, -4.0f, 5.f };
    pointLights[1].emissionColor = (Vec3) { 0.5f, 0.5f, 0.5f };

    Scene scene = {0};
    scene.materialCount = 5;
    scene.materials = materials;
    scene.planeCount = 2;
    scene.planes = planes;
    scene.sphereCount = 3;
    scene.spheres = spheres;
    scene.triangleCount = 0;
    scene.triangles = NULL;
    scene.pointLightCount = 2;
    scene.pointLights = pointLights;

	double rayColorContribution = 1.0f / (double) raysPerPixel;

	double pixelWidth = 1.0f / (double) width;
    double pixelHeight = 1.0f / (double) height;
    double rootTerm = sqrt(pixelWidth/pixelHeight * raysPerPixel + pow(pixelWidth - pixelHeight, 2) / 4 * pow(pixelHeight, 2));
    uint32_t raysPerWidthPixel = 1;
    uint32_t raysPerHeightPixel = 1;
    double deltaX = pixelWidth;
    double deltaY = pixelHeight;
    if (raysPerPixel > 1) {
        raysPerWidthPixel = (uint32_t) (rootTerm - (pixelWidth - pixelHeight / 2 * pixelHeight));
        raysPerHeightPixel = (uint32_t) (raysPerPixel / raysPerWidthPixel);
        deltaX = pixelWidth / (raysPerWidthPixel - 1);
        deltaY = pixelHeight / (raysPerHeightPixel - 1);
    }


    for (uint32_t y = 0; y < height; y++) {
        double PosY = -1.0f + 2.0f * ((double)y / (camera->height));
#ifndef _WINDOWS
        #pragma omp parallel for
#endif
        for (uint32_t x = 0; x < width; x++) {
            double PosX = -1.0f + 2.0f * ((double)x / (camera->width));

			Vec3 color = {0};
            for (uint32_t j = 0; j < raysPerHeightPixel; j++) {
                for (uint32_t i = 0; i < raysPerWidthPixel; i++) {
                    Vec3 OffsetY = vec3_mul(camera->y,
                                            (PosY - pixelWidth + (j + 1) * deltaY)*camera->renderTargetHeight/2.0f);
                    Vec3 OffsetX = vec3_mul(camera->x,
                                            (PosX - pixelHeight + (i + 1) * deltaX)*camera->renderTargetWidth/2.0f);
                    Vec3 renderTargetPos = vec3_add(vec3_add(camera->renderTargetCenter,
                                                             OffsetX),
                                                    OffsetY);
                    Ray ray = {
                            camera->position,
                            vec3_norm(vec3_sub(renderTargetPos, camera->position))
                    };

                    Vec3 currentRayColor = raytracer_raycast(&scene, &ray);
                    color = vec3_add(color, vec3_mul(currentRayColor, rayColorContribution));
                }
            }

            // currently values are clamped to [0,1]
            // in the future we may return doubles > 1.0
            // and use hdr to map it back to the [0.0, 1.0] range after
            // all pixels are calculated
            // this would avoid that really bright areas look the "same"
            color = vec3_clamp(color);

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
