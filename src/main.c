#include <float.h>
#include "utils/image.h"
#include "utils/vec3.h"
#include "camera.h"
#include "scene.h"
#include "raytracer.h"
#include "object.h"

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
    Vec3 camera_pos = { 0.0f , 2.0f, 40.0f };
    Vec3 lookAt = (Vec3) {0.0f, 0.0f, 0.0f};
    uint32_t width = 1920;
    uint32_t height = 1080;
    double FOV = 110.0f;
	uint32_t raysPerPixel = 1;
	uint32_t maxRecursionDepth = 8;
    Camera* camera = camera_create(camera_pos, lookAt, width, height, FOV);
    Image* image = image_create(width, height);

    Scene* scene = scene_create();
    scene->camera = camera;

    // background has to be added first
    Material background = {0};
    background.color = (Vec3) { 0.0f, 0.0f, 0.0f };
    background.reflectionIndex = 0.0f;
    background.refractionIndex = 0.0f;
    scene_addMaterial(scene, background);

    Material grey = {0};
    grey.color = (Vec3) { 0.4f, 0.4f, 0.4f };
    grey.reflectionIndex = 0.0f;
    uint32_t greyId = scene_addMaterial(scene, grey);

    Material redMirror = {0};
    redMirror.color = (Vec3) { 1.0f, 0.0f, 0.0f };
    redMirror.reflectionIndex = 1.0f;
    uint32_t redMirrorId = scene_addMaterial(scene, redMirror);

    Material mirror = {0};
    mirror.color = (Vec3) { 1.0f, 1.0f, 1.0f };
    mirror.reflectionIndex = 1.0f;
    uint32_t mirrorId = scene_addMaterial(scene, mirror);

    Material glass = {0};
    glass.color = (Vec3) { 1.0f, 1.0f, 1.0f };
    glass.reflectionIndex = 1.0f;
    glass.refractionIndex = 1.4f;
    uint32_t glassId = scene_addMaterial(scene, glass);

    Plane floor = {0};
    floor.materialIndex = greyId;
    floor.normal = (Vec3) { 0.0f, 1.0f, 0.0f };
    floor.distanceFromOrigin = 0;
    scene_addPlane(scene, floor);

    Plane front = {0};
    front.materialIndex = greyId;
    front.normal = (Vec3) { 0.0f, 0.0f, 1.0f };
    front.distanceFromOrigin = 8;
    scene_addPlane(scene, front);

    Plane back = {0};
    back.materialIndex = greyId;
    back.normal = (Vec3) { 0.0f, 0.0f, 1.0f };
    back.distanceFromOrigin = -40;
    scene_addPlane(scene, back);

    Plane left = {0};
    left.materialIndex = greyId;
    left.normal = (Vec3) { 1.0f, 0.0f, 0.0f };
    left.distanceFromOrigin = -8;
    scene_addPlane(scene, left);

    Plane right = {0};
    right.materialIndex = greyId;
    right.normal = (Vec3) { 1.0f, 0.0f, 0.0f };
    right.distanceFromOrigin = 8;
    scene_addPlane(scene, right);

    Sphere redLeftSphere = {0};
    redLeftSphere.materialIndex = redMirrorId;
    redLeftSphere.position = (Vec3) { -3.0f, 1.0f, 0.0f };
    redLeftSphere.radius = 1;
    scene_addSphere(scene, redLeftSphere);

    Sphere mirrorSphere = {0};
    mirrorSphere.materialIndex = mirrorId;
    mirrorSphere.position = (Vec3) { 0.0f, 1.5f, 0.0f };
    mirrorSphere.radius = 1;
    scene_addSphere(scene, mirrorSphere);

    Sphere glassSphere = {0};
    glassSphere.materialIndex = glassId;
    glassSphere.position = (Vec3) { 3.0f, 1.0f, 3.0f };
    glassSphere.radius = 1;
    scene_addSphere(scene, glassSphere);

    Triangle triangle = {0};
    triangle.materialIndex = redMirrorId;
    triangle.v0 = (Vec3) { 2.0f, 0.0f, 0.0f };
    triangle.v1 = (Vec3) { 4.0f, 0.0f, 0.0f };
    triangle.v2 = (Vec3) { 3.0f, 1.0f, 0.0f };
    scene_addTriangle(scene, triangle);

    PointLight pointLight = {0};
    pointLight.position = (Vec3) { -3.0f, 40.0f, 30.0f };
    pointLight.emissionColor = (Vec3) { 1.0f, 1.0f, 1.0f };
    pointLight.strength = 20000.0f;
    scene_addPointLight(scene, pointLight);

    Object* teapot = object_loadFromFile("teapot.obj");
    object_scale(teapot, 0.01);
    object_translate(teapot, (Vec3) { 3.0f, 1.0f, 5.0f });
    object_materialIndex(teapot, redMirrorId);
    scene_addObject(scene, *teapot);
    object_destroy(teapot);

    scene_shrink_to_fit(scene);

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
        deltaX = pixelWidth / raysPerWidthPixel;
        deltaY = pixelHeight / raysPerHeightPixel;
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
                                            (PosY - pixelWidth + j * deltaY)*camera->renderTargetHeight/2.0f);
                    Vec3 OffsetX = vec3_mul(camera->x,
                                            (PosX - pixelHeight + i * deltaX)*camera->renderTargetWidth/2.0f);
                    Vec3 renderTargetPos = vec3_sub(vec3_add(camera->renderTargetCenter, OffsetX), OffsetY);
                    Ray ray = {
                            camera->position,
                            vec3_norm(vec3_sub(renderTargetPos, camera->position))
                    };

                    Vec3 currentRayColor = raytracer_raycast(scene, &ray, 0, maxRecursionDepth);
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
    scene_destroy(scene);
    return 0;
}
