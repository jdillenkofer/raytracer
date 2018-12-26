#include <float.h>
#include "utils/image.h"
#include "utils/vec3.h"
#include "camera.h"
#include "scene.h"
#include "raytracer.h"
#include "gpu.h"

#include <SDL2/SDL.h>
#ifdef WIN32
// this is sadly required for <gl/gl.h> to compile...
// it defines some weird windows macro thingy
#include <windows.h>
#endif
#include <gl/gl.h>
#include "utils/random.h"
#include "utils/math.h"

void renderImage(SDL_Renderer *renderer, SDL_Texture* texture, Image* image) {
	SDL_UpdateTexture(texture, NULL, image->buffer, (int)(sizeof(uint32_t) * image->width));
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

int main(int argc, char* argv[]) {
    (void) argc;
    (void) argv;

    uint32_t width = 1920;
    uint32_t height = 1080;
	uint32_t raysPerPixel = 1;
	double MS_PER_UPDATE = 1000.0 / 120.0;
	float CAMERA_ROTATION_SPEED = 0.05f;

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
	SDL_GLContext glContext = SDL_GL_CreateContext(window);

	gpuContext* gpuContext = gpu_initContext();
	if (gpuContext == NULL) {
		return 1;
	}

	glEnable(GL_TEXTURE_2D);

	GLuint textureId;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	Image* image = image_create(width, height);
	cl_mem dev_image = gpu_createImageBufferFromTextureId(gpuContext, textureId);

	Scene* scene = scene_init(width, height);
	cl_mem dev_camera = gpu_createCameraBuffer(gpuContext, scene);
	cl_mem dev_materials = gpu_createMaterialsBuffer(gpuContext, scene);
	cl_mem dev_planes = gpu_createPlanesBuffer(gpuContext, scene);
	cl_mem dev_spheres = gpu_createSpheresBuffer(gpuContext, scene);
	cl_mem dev_triangles = gpu_createTrianglesBuffer(gpuContext, scene);
	cl_mem dev_pointLights = gpu_createPointLightsBuffer(gpuContext, scene);

	float rayColorContribution = 1.0f / (float)raysPerPixel;

	float pixelWidth = 1.0f / (float)width;
	float pixelHeight = 1.0f / (float)height;
	float rootTerm = sqrtf(pixelWidth / pixelHeight * raysPerPixel + powf(pixelWidth - pixelHeight, 2) / 4 * powf(pixelHeight, 2));
	uint32_t raysPerWidthPixel = 1;
	uint32_t raysPerHeightPixel = 1;
	float deltaX = pixelWidth;
	float deltaY = pixelHeight;
	if (raysPerPixel > 1) {
		raysPerWidthPixel = (uint32_t)(rootTerm - (pixelWidth - pixelHeight / 2 * pixelHeight));
		raysPerHeightPixel = (uint32_t)(raysPerPixel / raysPerWidthPixel);
		deltaX = pixelWidth / raysPerWidthPixel;
		deltaY = pixelHeight / raysPerHeightPixel;
	}

    cl_kernel raytrace_kernel = clCreateKernel(gpuContext->prog, "raytrace", &gpuContext->err);
    if (gpuContext->err != CL_SUCCESS) {
        printf("Couldn't create kernel raytrace.\n");
        return 1;
    }

    gpuContext->err = clSetKernelArg(raytrace_kernel, 0, sizeof(cl_mem), &dev_camera);
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 1, sizeof(cl_mem), &dev_materials);
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 2, sizeof(uint32_t), &scene->materialCount);
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 3, sizeof(cl_mem), &dev_planes);
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 4, sizeof(uint32_t), &scene->planeCount);
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 5, sizeof(cl_mem), &dev_spheres);
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 6, sizeof(uint32_t), &scene->sphereCount);
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 7, sizeof(cl_mem), &dev_triangles);
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 8, sizeof(uint32_t), &scene->triangleCount);
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 9, sizeof(cl_mem), &dev_pointLights);
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 10, sizeof(uint32_t), &scene->pointLightCount);
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 11, sizeof(cl_mem), &dev_image);
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 12, sizeof(float), &rayColorContribution);
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 13, sizeof(float), &deltaX);
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 14, sizeof(float), &deltaY);
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 15, sizeof(float), &pixelWidth);
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 16, sizeof(float), &pixelHeight);
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 17, sizeof(uint32_t), &raysPerWidthPixel);
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 18, sizeof(uint32_t), &raysPerHeightPixel);
    if (gpuContext->err != CL_SUCCESS) {
        printf("Couldn't set all kernel args correctly.\n");
        return 1;
    }

    // wait for quit event before quitting
    bool running = true;
	uint32_t lastTime = SDL_GetTicks();
	double delta = 0.0;

	float deg = 0;

    while(running) {

		uint32_t current = SDL_GetTicks();
		uint32_t elapsed = current - lastTime;
		lastTime = current;
		delta += elapsed;

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

		while (delta >= MS_PER_UPDATE) {
			// update

			// move the camera in a circle around the origin
			float radius = 40.0f;
			scene->camera->position.x = radius * cosf(math_deg2rad(deg));
			scene->camera->position.z = radius * sinf(math_deg2rad(deg));
			camera_setup(scene->camera);
			deg += CAMERA_ROTATION_SPEED;
			if (deg >= 360.0f) {
				deg = 0.0f;
			}
			
			delta -= MS_PER_UPDATE;
		}

		// render
		clEnqueueWriteBuffer(gpuContext->command_queue, dev_camera, CL_TRUE, 0, sizeof(Camera), scene->camera, 0, NULL, NULL);
		gpuContext->err = clSetKernelArg(raytrace_kernel, 0, sizeof(cl_mem), &dev_camera);
		const size_t threadsPerDim[2] = { width, height };
		glFinish();
		clEnqueueAcquireGLObjects(gpuContext->command_queue, 1, &dev_image, 0, NULL, NULL);
		gpuContext->err = clEnqueueNDRangeKernel(gpuContext->command_queue, raytrace_kernel, 2, NULL, threadsPerDim, NULL, 0, NULL, NULL);
		if (gpuContext->err != CL_SUCCESS) {
			printf("Couldn't enqueue kernel.\n");
			return 1;
		}
		clEnqueueReleaseGLObjects(gpuContext->command_queue, 1, &dev_image, 0, NULL, NULL);
		clFinish(gpuContext->command_queue);

		// Copy back from device
		// clEnqueueReadBuffer(gpuContext->command_queue, dev_image, CL_TRUE, 0, sizeof(uint32_t) * image->width * image->height, image->buffer, 0, NULL, NULL);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glBindTexture(GL_TEXTURE_2D, textureId);
		glBegin(GL_QUADS);

		glTexCoord2f(0.0f, 1.0f);
		glVertex2f(-1.0f, -1.0f);

		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(-1.0f, 1.0f);
		
		glTexCoord2f(1.0f, 0.0f);
		glVertex2f(1.0f, 1.0f);
		
		glTexCoord2f(1.0f, 1.0f);
		glVertex2f(1.0f, -1.0f);

		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0);

		SDL_GL_SwapWindow(window);
    }

	// bitmap_save_image("test.bmp", image);
    
	SDL_GL_DeleteContext(glContext);
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
    gpu_destroyContext(gpuContext);
    return 0;
}
