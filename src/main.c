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

#define MS_PER_UPDATE 1000.0 / 120.0

// degrees per tick
#define CAMERA_ROTATION_SPEED 0.25f;

#define RENDER_WIDTH  1920
#define RENDER_HEIGHT 1080

#define RENDER_ASPECTRATIO (RENDER_WIDTH / (float)RENDER_HEIGHT)

uint32_t viewWidth = RENDER_WIDTH;
uint32_t viewHeight = RENDER_HEIGHT;

uint32_t raysPerPixel = 1;

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
"uniform vec2 scale;\n"
"\n"
"smooth out vec2 texel;\n"
"\n"
"void main(){\n"
"	texel = textureCoords;\n"
"   gl_Position = vec4(scale * position, 0.0, 1.0);\n"
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
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, RENDER_WIDTH, RENDER_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, RENDER_WIDTH, RENDER_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
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

	// initially no scaling is required, because renderAspectRatio is the same as viewAspectRatio
	GLint scaleLoc = glGetUniformLocation(shaderProgram, "scale");
	glUniform2f(scaleLoc, 1.0f, 1.0f);
	glViewport(0, 0, viewWidth, viewHeight);

	cl_mem dev_image = gpu_createImageBufferFromTextureId(gpuContext, textureId);

	Scene* scene = scene_init(RENDER_WIDTH, RENDER_HEIGHT);
	cl_mem dev_camera = gpu_createCameraBuffer(gpuContext, scene);
	cl_mem dev_materials = gpu_createMaterialsBuffer(gpuContext, scene);
	cl_mem dev_planes = gpu_createPlanesBuffer(gpuContext, scene);
	cl_mem dev_spheres = gpu_createSpheresBuffer(gpuContext, scene);
	cl_mem dev_triangles = gpu_createTrianglesBuffer(gpuContext, scene);
	cl_mem dev_pointLights = gpu_createPointLightsBuffer(gpuContext, scene);

	float rayColorContribution = 1.0f / (float)raysPerPixel;

	float pixelWidth = 1.0f / (float) RENDER_WIDTH;
	float pixelHeight = 1.0f / (float) RENDER_HEIGHT;
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
	gpuContext->err |= clSetKernelArg(raytrace_kernel, 1, sizeof(Camera), NULL); // sharedMemory camera
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 2, sizeof(cl_mem), &dev_materials);
	gpuContext->err |= clSetKernelArg(raytrace_kernel, 3, sizeof(Material) * scene->materialCount, NULL); // sharedMemory materials
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 4, sizeof(uint32_t), &scene->materialCount);
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 5, sizeof(cl_mem), &dev_planes);
	gpuContext->err |= clSetKernelArg(raytrace_kernel, 6, sizeof(Plane) * scene->planeCount, NULL); // sharedMemory planes
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 7, sizeof(uint32_t), &scene->planeCount);
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 8, sizeof(cl_mem), &dev_spheres);
	gpuContext->err |= clSetKernelArg(raytrace_kernel, 9, sizeof(Sphere) * scene->sphereCount, NULL); // sharedMemory spheres
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 10, sizeof(uint32_t), &scene->sphereCount);
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 11, sizeof(cl_mem), &dev_triangles);
	gpuContext->err |= clSetKernelArg(raytrace_kernel, 12, sizeof(Triangle) * scene->triangleCount, NULL); // sharedMemory triangles
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 13, sizeof(uint32_t), &scene->triangleCount);
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 14, sizeof(cl_mem), &dev_pointLights);
	gpuContext->err |= clSetKernelArg(raytrace_kernel, 15, sizeof(PointLight) * scene->pointLightCount, NULL); // sharedMemory pointLights
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 16, sizeof(uint32_t), &scene->pointLightCount);
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 17, sizeof(cl_mem), &dev_image);
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 18, sizeof(float), &rayColorContribution);
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 19, sizeof(float), &deltaX);
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 20, sizeof(float), &deltaY);
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 21, sizeof(float), &pixelWidth);
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 22, sizeof(float), &pixelHeight);
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 23, sizeof(uint32_t), &raysPerWidthPixel);
    gpuContext->err |= clSetKernelArg(raytrace_kernel, 24, sizeof(uint32_t), &raysPerHeightPixel);
    if (gpuContext->err != CL_SUCCESS) {
        printf("Couldn't set all kernel args correctly.\n");
        return 1;
    }

    // wait for quit event before quitting
    bool running = true;

	uint32_t previousTime = SDL_GetTicks();
	double delta = 0.0;

	float deg = 0;

    while(running) {

		uint32_t currentTime = SDL_GetTicks();
		uint32_t elapsed = currentTime - previousTime;
		previousTime = currentTime;
		delta += elapsed;

		// input handling
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
				case SDL_WINDOWEVENT:
					switch (event.window.event) {
					case SDL_WINDOWEVENT_SIZE_CHANGED:
					case SDL_WINDOWEVENT_RESIZED:
						// onResize: recalculate the scaling
						viewWidth = event.window.data1;
						viewHeight = event.window.data2;
						float xScale = 1.0f;
						float yScale = 1.0f;
						float viewAspectRatio = viewWidth / (float)viewHeight;
						if (RENDER_ASPECTRATIO > viewAspectRatio) {
							yScale = viewAspectRatio / RENDER_ASPECTRATIO;
						}
						else {
							xScale = RENDER_ASPECTRATIO / viewAspectRatio;
						}
						glUniform2f(scaleLoc, xScale, yScale);
						glViewport(0, 0, viewWidth, viewHeight);
						break;
					default:
						break;
					}
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym) {
					case SDLK_w:
						break;
					case SDLK_s:
						break;
					case SDLK_a:
						break;
					case SDLK_d:
						break;
					case SDLK_ESCAPE:
						running = false;
						break;
					default:
						break;
					}
					break;
				case SDL_KEYUP:
					break;
                default:
                    break;
            }
        }

		// update
		while (delta >= MS_PER_UPDATE) {
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
		const size_t threadsPerDim[2] = { RENDER_WIDTH, RENDER_HEIGHT};
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
