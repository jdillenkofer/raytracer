#include <float.h>
#include "utils/image.h"
#include "utils/vec3.h"
#include "camera.h"
#include "scene.h"
#include "raytracer.h"
#include "object.h"
#include "kernel.h"

#include <SDL2/SDL.h>
#include "utils/random.h"
#include "utils/math.h"

Scene* initScene(uint32_t width, uint32_t height) {
    Scene* scene = scene_create();
    {
        Vec3 camera_pos = { 40.0f , 2.0f, 0.0f };
        Vec3 lookAt = (Vec3) {0.0f, 0.0f, 0.0f};
        float FOV = 110.0f;
        Camera* camera = camera_create(camera_pos, lookAt, width, height, FOV);
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

        Material yellow = {0};
        yellow.color = (Vec3) { 1.0f, 0.6549f, 0.1019f };
        uint32_t yellowId = scene_addMaterial(scene, yellow);

        Plane floor = {0};
        floor.materialIndex = greyId;
        floor.normal = (Vec3) { 0.0f, 1.0f, 0.0f };
        floor.distanceFromOrigin = 0;
        scene_addPlane(scene, floor);

        Plane front = {0};
        front.materialIndex = greyId;
        front.normal = (Vec3) { 0.0f, 0.0f, 1.0f };
        front.distanceFromOrigin = 50;
        scene_addPlane(scene, front);

        Plane back = {0};
        back.materialIndex = greyId;
        back.normal = (Vec3) { 0.0f, 0.0f, 1.0f };
        back.distanceFromOrigin = -50;
        scene_addPlane(scene, back);

        Plane left = {0};
        left.materialIndex = greyId;
        left.normal = (Vec3) { 1.0f, 0.0f, 0.0f };
        left.distanceFromOrigin = -50;
        scene_addPlane(scene, left);

        Plane right = {0};
        right.materialIndex = greyId;
        right.normal = (Vec3) { 1.0f, 0.0f, 0.0f };
        right.distanceFromOrigin = 50;
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
        pointLight.position = (Vec3) { 0.0f, 10.0f, 10.0f };
        pointLight.emissionColor = (Vec3) { 1.0f, 1.0f, 1.0f };
        pointLight.strength = 1500.0f;
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

void renderImage(SDL_Renderer *renderer, Image *image) {
    SDL_Surface* surface =
            SDL_CreateRGBSurfaceFrom((void*)image->buffer, (int) image->width, (int) image->height, 32,
                                     (int) (sizeof(uint32_t) * image->width), 0xFFu << 16, 0xFFu << 8, 0xFFu, 0xFFu << 24);
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

    uint32_t width = 1920;
    uint32_t height = 1080;
	uint32_t raysPerPixel = 1;
	uint32_t maxRecursionDepth = 5;
	double MS_PER_UPDATE = 1000.0 / 60.0;

    oclContext* openCLContext = initOpenClContext();

    Image* image = image_create(width, height);
    cl_mem dev_image = image_create_gpu(openCLContext, image);

    Scene* scene = initScene(width, height);
    cl_mem dev_camera = scene_create_gpu_camera(openCLContext, scene);
    cl_mem dev_materials = scene_create_gpu_materials(openCLContext, scene);
    cl_mem dev_planes = scene_create_gpu_planes(openCLContext, scene);
    cl_mem dev_spheres = scene_create_gpu_spheres(openCLContext, scene);
    cl_mem dev_triangles = scene_create_gpu_triangles(openCLContext, scene);
    cl_mem dev_pointLights = scene_create_gpu_pointLights(openCLContext, scene);

	float rayColorContribution = 1.0f / (float) raysPerPixel;

	float pixelWidth = 1.0f / (float) width;
    float pixelHeight = 1.0f / (float) height;
    float rootTerm = sqrtf(pixelWidth/pixelHeight * raysPerPixel + powf(pixelWidth - pixelHeight, 2) / 4 * powf(pixelHeight, 2));
    uint32_t raysPerWidthPixel = 1;
    uint32_t raysPerHeightPixel = 1;
    float deltaX = pixelWidth;
    float deltaY = pixelHeight;
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

    cl_kernel raytrace_kernel = clCreateKernel(openCLContext->prog, "raytrace", &openCLContext->err);
    if (openCLContext->err != CL_SUCCESS) {
        printf("Couldn't create kernel raytrace.\n");
        return 1;
    }

    openCLContext->err = clSetKernelArg(raytrace_kernel, 0, sizeof(cl_mem), &dev_camera);
    openCLContext->err |= clSetKernelArg(raytrace_kernel, 1, sizeof(cl_mem), &dev_materials);
    openCLContext->err |= clSetKernelArg(raytrace_kernel, 2, sizeof(uint32_t), &scene->materialCount);
    openCLContext->err |= clSetKernelArg(raytrace_kernel, 3, sizeof(cl_mem), &dev_planes);
    openCLContext->err |= clSetKernelArg(raytrace_kernel, 4, sizeof(uint32_t), &scene->planeCount);
    openCLContext->err |= clSetKernelArg(raytrace_kernel, 5, sizeof(cl_mem), &dev_spheres);
    openCLContext->err |= clSetKernelArg(raytrace_kernel, 6, sizeof(uint32_t), &scene->sphereCount);
    openCLContext->err |= clSetKernelArg(raytrace_kernel, 7, sizeof(cl_mem), &dev_triangles);
    openCLContext->err |= clSetKernelArg(raytrace_kernel, 8, sizeof(uint32_t), &scene->triangleCount);
    openCLContext->err |= clSetKernelArg(raytrace_kernel, 9, sizeof(cl_mem), &dev_pointLights);
    openCLContext->err |= clSetKernelArg(raytrace_kernel, 10, sizeof(uint32_t), &scene->pointLightCount);
    openCLContext->err |= clSetKernelArg(raytrace_kernel, 11, sizeof(cl_mem), &dev_image);
    openCLContext->err |= clSetKernelArg(raytrace_kernel, 12, sizeof(uint32_t), &maxRecursionDepth);
    openCLContext->err |= clSetKernelArg(raytrace_kernel, 13, sizeof(float), &rayColorContribution);
    openCLContext->err |= clSetKernelArg(raytrace_kernel, 14, sizeof(float), &deltaX);
    openCLContext->err |= clSetKernelArg(raytrace_kernel, 15, sizeof(float), &deltaY);
    openCLContext->err |= clSetKernelArg(raytrace_kernel, 16, sizeof(float), &pixelWidth);
    openCLContext->err |= clSetKernelArg(raytrace_kernel, 17, sizeof(float), &pixelHeight);
    openCLContext->err |= clSetKernelArg(raytrace_kernel, 18, sizeof(uint32_t), &raysPerWidthPixel);
    openCLContext->err |= clSetKernelArg(raytrace_kernel, 19, sizeof(uint32_t), &raysPerHeightPixel);
    if (openCLContext->err != CL_SUCCESS) {
        printf("Couldn't set all kernel args correctly.\n");
        return 1;
    }

    // wait for quit event before quitting
    bool running = true;
	double previous = (double) SDL_GetTicks();
	double lag = 0.0;
	bool isRenderDirty = true;

	float deg = 0;

    while(running) {

		double current = (double)SDL_GetTicks();
		double elapsed = current - previous;
		previous = current;
		lag += elapsed;

		// input handling
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                default:
                    break;
            }
        }

		while (lag >= MS_PER_UPDATE) {
			// update

			// move the camera in a circle around the origin
			float radius = 40.0f;
			scene->camera->position.x = radius * cosf(math_deg2rad(deg));
			scene->camera->position.z = radius * sinf(math_deg2rad(deg));
			camera_setup(scene->camera);
			deg += 0.05f;
			if (deg >= 360.0) {
				deg = 0.0;
			}
			
			isRenderDirty = true;
			lag -= MS_PER_UPDATE;
		}

		// render
		if (isRenderDirty) {
			clEnqueueWriteBuffer(openCLContext->command_queue, dev_camera, CL_TRUE, 0, sizeof(Camera), scene->camera, 0, NULL, NULL);
			openCLContext->err = clSetKernelArg(raytrace_kernel, 0, sizeof(cl_mem), &dev_camera);
			const size_t threadsPerDim[2] = { width, height };
			openCLContext->err = clEnqueueNDRangeKernel(openCLContext->command_queue, raytrace_kernel, 2, NULL, threadsPerDim, NULL, 0, NULL, NULL);
			if (openCLContext->err != CL_SUCCESS) {
				printf("Couldn't enqueue kernel.\n");
				return 1;
			}

			// Copy back from device
			clEnqueueReadBuffer(openCLContext->command_queue, dev_image, CL_TRUE, 0, sizeof(uint32_t) * image->width * image->height, image->buffer, 0, NULL, NULL);

			renderImage(renderer, image);
			isRenderDirty = false;
		}
    }

    bitmap_save_image("test.bmp", image);
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    image_destroy(image);
    scene_destroy(scene);
    clReleaseKernel(raytrace_kernel);
    clReleaseMemObject(dev_camera);
    clReleaseMemObject(dev_materials);
    clReleaseMemObject(dev_planes);
    clReleaseMemObject(dev_spheres);
    clReleaseMemObject(dev_triangles);
    clReleaseMemObject(dev_pointLights);
    destroyOpenClContext(openCLContext);
    return 0;
}
