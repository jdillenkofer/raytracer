#include <float.h>
#include "utils/image.h"
#include "utils/vec3.h"
#include "camera.h"
#include "scene.h"
#include "raytracer.h"

#include <SDL2/SDL.h>
#include <omp.h>
#include <utils/random.h>

void handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_QUIT:
                SDL_Quit();
                exit(0);
            default:
                break;
        }
    }
}

void renderImage(SDL_Renderer *renderer, Image *image) {
    SDL_Surface* surface =
            SDL_CreateRGBSurfaceFrom((void*)image->buffer, (int) image->width, (int) image->height, 32,
                                     (int) (sizeof(uint32_t) * image->width), 0xFF << 16, 0xFF << 8, 0xFF, 0xFF << 24);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_DestroyTexture(texture);
    SDL_RenderPresent(renderer);
}

int main(int argc, char* argv[]) {
    (void) argc;
    (void) argv;
    Vec3 camera_pos = { 0.0f , 40.0f, 2.0f };
    Vec3 lookAt = (Vec3) {0.0f, 0.0f, 0.0f};
    uint32_t width = 1920;
    uint32_t height = 1080;
    double FOV = 110.0f;
	uint32_t raysPerPixel = 16;
    uint32_t numShadowRays = 16;
	uint32_t maxRecursionDepth = 10;
    Camera* camera = camera_create(camera_pos, lookAt, width, height, FOV);
    Image* image = image_create(width, height);

    Material materials[5] = {0};
    materials[0].color = (Vec3) { 0.0f, 0.0f, 0.0f };
	materials[0].reflectionIndex = 0.0f;
	materials[0].refractionIndex = 0.0f;
    
	materials[1].color = (Vec3) { 0.4f, 0.4f, 0.4f };
	materials[1].reflectionIndex = 0.0f;
	materials[1].refractionIndex = 0.0f;

    materials[2].color = (Vec3) { 1.0f, 0.0f, 0.0f };
	materials[2].reflectionIndex = 1.0f;
	materials[2].refractionIndex = 0.0f;

    materials[3].color = (Vec3) { 1.0f, 1.0f, 1.0f };
	materials[3].reflectionIndex = 1.0f;
	materials[3].refractionIndex = 0.0f;

    materials[4].color = (Vec3) { 0.0f, 0.0f, 1.0f };
	materials[4].reflectionIndex = 0.5f;
	materials[4].refractionIndex = 0.0f;

    Plane planes[5] = {0};
    // floor
    planes[0].materialIndex = 1;
    planes[0].normal = (Vec3) { 0.0f, 0.0f, 1.0f };
    planes[0].distanceFromOrigin = 0;
    // front wall
    planes[1].materialIndex = 1;
    planes[1].normal = (Vec3) { 0.0f, 1.0f, 0.0f };
    planes[1].distanceFromOrigin = 8;
    // back wall
	planes[2].materialIndex = 1;
	planes[2].normal = (Vec3) { 0.0f, 1.0f, 0.0f };
	planes[2].distanceFromOrigin = 40;
	// left wall
	planes[3].materialIndex = 1;
	planes[3].normal = (Vec3) { 1.0f, 0.0f, 0.0f };
	planes[3].distanceFromOrigin = -8;
	// right wall
	planes[4].materialIndex = 1;
	planes[4].normal = (Vec3) { 1.0f, 0.0f, 0.0f };
	planes[4].distanceFromOrigin = 8;

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

    Triangle triangles[1] = {0};
    triangles[0].materialIndex = 2;
    triangles[0].v0 = (Vec3) { -1.0f, 5.0f, 0.0f };
    triangles[0].v1 = (Vec3) { 1.0f, 5.0f, 0.0f };
    triangles[0].v2 = (Vec3) { 0.0f, 5.0f, 1.0f };

    PointLight pointLights[1] = {0};
    pointLights[0].position = (Vec3) { 2.0f, 30.0f, 40.0f };
    pointLights[0].emissionColor = (Vec3) { 1.0f, 1.0f, 1.0f };
    pointLights[0].strength = 1500.0f;

    Scene scene = {0};
    scene.camera = camera;
    scene.materialCount = 5;
    scene.materials = materials;
    scene.planeCount = 5;
    scene.planes = planes;
    scene.sphereCount = 3;
    scene.spheres = spheres;
    scene.triangleCount = 1;
    scene.triangles = triangles;
    scene.pointLightCount = 1;
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


    if (SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Raytracer",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, (int32_t) width, (int32_t) height, SDL_WINDOW_OPENGL);
    if (!window) {
        SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Unable to create window: %s", SDL_GetError());
        return 2;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);


    for (uint32_t y = 0; y < height; y++) {
        double PosY = -1.0f + 2.0f * ((double)y / (camera->height));
#ifdef OPENMP
    #ifndef _WINDOWS
            #pragma omp parallel for
    #endif
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
                    Vec3 renderTargetPos = vec3_add(vec3_add(camera->renderTargetCenter, OffsetX), OffsetY);
                    Ray ray = {
                            camera->position,
                            vec3_norm(vec3_sub(renderTargetPos, camera->position))
                    };

                    Vec3 currentRayColor = raytracer_raycast(&scene, &ray, numShadowRays, 0, maxRecursionDepth);
                    color = vec3_add(color, vec3_mul(currentRayColor, rayColorContribution));
                }
            }

            // currently values are clamped to [0,1]
            // in the future we may return doubles > 1.0
            // and use hdr to map it back to the [0.0, 1.0] range after
            // all pixels are calculated
            // this would avoid that really bright areas look the "same"
            color = vec3_clamp(color, 0.0f, 1.0f);

			image->buffer[y*width + x] =
				(0xFFu << 24) |
				((uint32_t)(color.r * 255) << 16) |
				((uint32_t)(color.g * 255) << 8) |
				((uint32_t)(color.b * 255) << 0);
        }
		printf("\rRaytracing %d%%", (uint32_t)(100 * ((double)(y) / (double)(height))));
        handleEvents();
        renderImage(renderer, image);
    }

    bitmap_save_image("test.bmp", image);
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    image_destroy(image);
    camera_destroy(camera);
    return 0;
}
