#include <float.h>

#include <glad/glad.h>
#include <SDL2/SDL.h>

#include "utils/image.h"
#include "utils/vec3.h"
#include "camera.h"
#include "scene.h"
#include "raytracer.h"
#include "gpu.h"

#include "utils/random.h"
#include "utils/math.h"

float vertices[] = {
	// VERTEX POS; TEXTURE POS
	 1.0f,  1.0f, 1.0f, 0.0f, // top right
	 1.0f, -1.0f, 1.0f, 1.0f, // bottom right
	-1.0f, -1.0f, 0.0f, 1.0f, // bottom left
	-1.0f,  1.0f, 0.0f, 0.0f, // top left
};

uint32_t indices[] = {
	0, 1, 3, // first triangle
	1, 2, 3  // second triangle
};

const char* vertexShader =
"#version 330 core\n"
"layout(location = 0) in vec2 position;\n"
"layout(location = 1) in vec2 textureCoords;\n"
"\n"
"smooth out vec2 texel;\n"
"\n"
"void main(){\n"
"	texel = textureCoords;\n"
"   gl_Position = vec4(position, 0.0, 1.0);\n"
"}\n";

const char* fragmentShader =
"#version 330 core\n"
"uniform sampler2D texture1;\n"
"smooth in vec2 texel;\n"
"\n"
"layout(location = 0) out vec4 color;\n"
"\n"
"void main(){\n"
"   color = texture(texture1, texel);\n"
"}\n";

int main(int argc, char* argv[]) {
    (void) argc;
    (void) argv;

    uint32_t width = 1920;
    uint32_t height = 1080;
	uint32_t raysPerPixel = 1;
	double MS_PER_UPDATE = 1000.0 / 120.0;
	// degrees per tick
	float CAMERA_ROTATION_SPEED = 0.25f;

    if (SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE); //OpenGL core profile
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

    SDL_Window* window = SDL_CreateWindow("Raytracer",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, (int32_t) width, (int32_t) height, SDL_WINDOW_OPENGL);
    if (!window) {
        SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Unable to create window: %s", SDL_GetError());
        return 2;
    }

	SDL_GLContext glContext = SDL_GL_CreateContext(window);
	// try to set adaptive swap interval
	if (SDL_GL_SetSwapInterval(-1) != 0) {
		// if this is not supported, try to enable vsync
		SDL_GL_SetSwapInterval(1);
	}

	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
		SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Failed to initialize GLAD.");
		return 2;
	}

	gpuContext* gpuContext = gpu_initContext();
	if (gpuContext == NULL) {
		return 1;
	}

	uint32_t VBO;
	glGenBuffers(1, &VBO);

	unsigned int EBO;
	glGenBuffers(1, &EBO);

	unsigned int VAO;
	glGenVertexArrays(1, &VAO);

	// GL Initialization code
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);

	glEnable(GL_TEXTURE_2D);

	GLuint textureId;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	uint32_t shaderProgram = gpu_compileShaderProgram(vertexShader, fragmentShader);
	
	// set the drawing state once
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glUseProgram(shaderProgram);
	glBindVertexArray(VAO);

	GLint texture1Loc = glGetUniformLocation(shaderProgram, "texture1");
	glUniform1i(texture1Loc, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureId);

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
		glFinish();
		clEnqueueWriteBuffer(gpuContext->command_queue, dev_camera, CL_TRUE, 0, sizeof(Camera), scene->camera, 0, NULL, NULL);
		gpuContext->err = clSetKernelArg(raytrace_kernel, 0, sizeof(cl_mem), &dev_camera);
		const size_t threadsPerDim[2] = { width, height };
		clEnqueueAcquireGLObjects(gpuContext->command_queue, 1, &dev_image, 0, NULL, NULL);
		gpuContext->err = clEnqueueNDRangeKernel(gpuContext->command_queue, raytrace_kernel, 2, NULL, threadsPerDim, NULL, 0, NULL, NULL);
		if (gpuContext->err != CL_SUCCESS) {
			printf("Couldn't enqueue kernel.\n");
			return 1;
		}
		clEnqueueReleaseGLObjects(gpuContext->command_queue, 1, &dev_image, 0, NULL, NULL);
		clFinish(gpuContext->command_queue);

		glClear(GL_COLOR_BUFFER_BIT);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		
		SDL_GL_SwapWindow(window);
    }
    
	SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

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
