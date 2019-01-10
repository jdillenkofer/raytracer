#ifndef RAYTRACER_GPU_H
#define RAYTRACER_GPU_H

#ifdef __linux__
#include <GL/glx.h>
#endif

#include <glad/glad.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <CL/cl_gl.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "utils/file.h"
#include "utils/image.h"
#include "scene.h"

typedef struct {
	struct {
		cl_platform_id platformId;
		cl_device_id deviceId;
		cl_context ctx;
		cl_command_queue commandQueue;
		cl_program program;
		cl_kernel kernel;
		cl_mem image;
		cl_mem camera;
		cl_mem materials;
		cl_mem planes;
		cl_mem spheres;
		cl_mem triangles;
		cl_mem pointLights;
		cl_int err;
	} cl;
	struct {
		GLuint vao;
		GLuint vbo;
		GLuint ibo;
		GLuint texture;
		GLuint shaderProgram;
		GLint texture1Location;
		GLint scaleLocation;
	} gl;
} GPUContext;

// -------------------- MIXED --------------------

GPUContext* gpu_initContext(Scene* scene, uint32_t raysPerPixel);
void gpu_renderScene(GPUContext* context, Scene* scene, Image* image);
void gpu_destroyContext(GPUContext* context);

// -------------------- OPENCL --------------------

static GPUContext* gpu_initCLContext();
// this needs to be done after gl texture creation
static bool gpu_allocateCLMemory(GPUContext* context, Scene* scene);
static bool gpu_setupKernel(GPUContext* context, Scene* scene, uint32_t raysPerPixel);
static void gpu_deleteCLMemory(GPUContext* context);

static cl_mem gpu_createImageBufferFromTextureId(GPUContext* context, GLuint textureId) {
	cl_mem dev_image = clCreateFromGLTexture(context->cl.ctx, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, textureId, &context->cl.err);
	if (context->cl.err != CL_SUCCESS) {
		printf("Couldn't create dev_image.\n");
		return NULL;
	}
	return dev_image;
}

static cl_mem gpu_createCameraBuffer(GPUContext* context, Scene* scene) {
	cl_mem dev_camera = (void*)clCreateBuffer(context->cl.ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Camera), scene->camera, &context->cl.err);
	if (context->cl.err != CL_SUCCESS) {
		printf("Couldn't create dev_camera.\n");
		return NULL;
	}
	return dev_camera;
}

static cl_mem gpu_createMaterialsBuffer(GPUContext* context, Scene* scene) {
	cl_mem dev_materials = (void*)clCreateBuffer(context->cl.ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Material) * scene->materialCount, scene->materials, &context->cl.err);
	if (context->cl.err != CL_SUCCESS) {
		printf("Couldn't create dev_materials.\n");
		return NULL;
	}
	return dev_materials;
}

static cl_mem gpu_createPlanesBuffer(GPUContext* context, Scene* scene) {
	cl_mem dev_planes = (void*)clCreateBuffer(context->cl.ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Plane) * scene->planeCount, scene->planes, &context->cl.err);
	if (context->cl.err != CL_SUCCESS) {
		printf("Couldn't create dev_planes.\n");
		return NULL;
	}
	return dev_planes;
}

static cl_mem gpu_createSpheresBuffer(GPUContext* context, Scene* scene) {
	cl_mem dev_spheres = (void*)clCreateBuffer(context->cl.ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Sphere) * scene->sphereCount, scene->spheres, &context->cl.err);
	if (context->cl.err != CL_SUCCESS) {
		printf("Couldn't create dev_spheres.\n");
		return NULL;
	}
	return dev_spheres;
}

static cl_mem gpu_createTrianglesBuffer(GPUContext* context, Scene* scene) {
	cl_mem dev_triangles = (void*)clCreateBuffer(context->cl.ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Triangle) * scene->triangleCount, scene->triangles, &context->cl.err);
	if (context->cl.err != CL_SUCCESS) {
		printf("Couldn't create dev_triangles.\n");
		return NULL;
	}
	return dev_triangles;
}

static cl_mem gpu_createPointLightsBuffer(GPUContext* context, Scene* scene) {
	cl_mem dev_pointLights = (void*)clCreateBuffer(context->cl.ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(PointLight) * scene->pointLightCount, scene->pointLights, &context->cl.err);
	if (context->cl.err != CL_SUCCESS) {
		printf("Couldn't create dev_pointlights.\n");
		return NULL;
	}
	return dev_pointLights;
}

static GPUContext* gpu_initCLContext() {
	GPUContext* context = malloc(sizeof(GPUContext));
	clGetPlatformIDs(1, &context->cl.platformId, NULL);
	clGetDeviceIDs(context->cl.platformId, CL_DEVICE_TYPE_GPU, 1, &context->cl.deviceId, NULL);

	/*
	 * These properties are required for the OpenGL <-> OpenCL interop
	 * Note: The OpenGL context needs to be created before creating the OpenCL context.
	 */
#ifdef WIN32
	cl_context_properties props[] = {
		CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
		CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
		CL_CONTEXT_PLATFORM, (cl_context_properties)context->cl.platformId,
		0 };
#endif
#ifdef __linux__
	cl_context_properties props[] = {
			CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(),
			CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(),
			CL_CONTEXT_PLATFORM, (cl_context_properties)context->plat,
			0 };
#endif
	context->cl.ctx = clCreateContext(props, 1, &context->cl.deviceId, NULL, NULL, &context->cl.err);
	context->cl.commandQueue = clCreateCommandQueue(context->cl.ctx, context->cl.deviceId, 0, &context->cl.err);

	size_t sourceSize = 0;
	char* source = file_readFile("kernel.cl", &sourceSize);

	context->cl.program = clCreateProgramWithSource(context->cl.ctx, 1, &source, &sourceSize, &context->cl.err);
	clBuildProgram(context->cl.program, 1, &context->cl.deviceId, NULL, NULL, NULL);
#ifndef NDEBUG
	cl_build_status status;
	clGetProgramBuildInfo(context->cl.program, context->cl.deviceId, CL_PROGRAM_BUILD_STATUS, sizeof(cl_build_status), &status, NULL);
	if (status != CL_BUILD_SUCCESS) {
		char* log;
		size_t log_size = 0;

		// get the size of the log
		clGetProgramBuildInfo(context->cl.program, context->cl.deviceId, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

		log = malloc(sizeof(char) * (log_size + 1));
		// get the log itself
		clGetProgramBuildInfo(context->cl.program, context->cl.deviceId, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
		log[log_size] = '\0';
		// print the log
		printf("Build log:\n%s\n", log);
		free(log);
		return NULL;
	}
#endif
	return context;
}

static bool gpu_allocateCLMemory(GPUContext* context, Scene* scene) {
	context->cl.image = gpu_createImageBufferFromTextureId(context, context->gl.texture);
	if (!context->cl.image) {
		return false;
	}
	context->cl.camera = gpu_createCameraBuffer(context, scene);
	if (!context->cl.camera) {
		return false;
	}
	context->cl.materials = gpu_createMaterialsBuffer(context, scene);
	if (!context->cl.materials) {
		return false;
	}
	context->cl.planes = gpu_createPlanesBuffer(context, scene);
	if (!context->cl.planes) {
		return false;
	}
	context->cl.spheres = gpu_createSpheresBuffer(context, scene);
	if (!context->cl.spheres) {
		return false;
	}
	context->cl.triangles = gpu_createTrianglesBuffer(context, scene);
	if (!context->cl.triangles) {
		return false;
	}
	context->cl.pointLights = gpu_createPointLightsBuffer(context, scene);
	if (!context->cl.pointLights) {
		return false;
	}
	return true;
}

static bool gpu_setupKernel(GPUContext* context, Scene* scene, uint32_t raysPerPixel) {
	cl_kernel raytrace_kernel = context->cl.kernel = clCreateKernel(context->cl.program, "raytrace", &context->cl.err);
	if (context->cl.err != CL_SUCCESS) {
		printf("Couldn't create kernel raytrace.\n");
		return false;
	}

	// this calculates how many rays we have on X and Y, and how much the deltaX/Y for these subpixel samples are
	assert(raysPerPixel > 0);
	float rayColorContribution = 1.0f / (float)raysPerPixel;

	float pixelWidth = 1.0f / (float)(scene->camera->width);
	float pixelHeight = 1.0f / (float)(scene->camera->height);
	float rootTerm = sqrtf(pixelWidth / pixelHeight * raysPerPixel + powf(pixelWidth - pixelHeight, 2) / 4 * powf(pixelHeight, 2));

	uint32_t raysPerWidthPixel = 1;
	uint32_t raysPerHeightPixel = 1;
	float deltaX = pixelWidth;
	float deltaY = pixelHeight;

	// prevent division by 0
	if (raysPerPixel > 1) {
		raysPerWidthPixel = (uint32_t)(rootTerm - (pixelWidth - pixelHeight / 2 * pixelHeight));
		raysPerHeightPixel = (uint32_t)(raysPerPixel / raysPerWidthPixel);
		deltaX = pixelWidth / raysPerWidthPixel;
		deltaY = pixelHeight / raysPerHeightPixel;
	}

	context->cl.err = clSetKernelArg(raytrace_kernel, 0, sizeof(cl_mem), &context->cl.camera);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 1, sizeof(Camera), NULL); // sharedMemory camera
	context->cl.err |= clSetKernelArg(raytrace_kernel, 2, sizeof(cl_mem), &context->cl.materials);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 3, sizeof(Material) * scene->materialCount, NULL); // sharedMemory materials
	context->cl.err |= clSetKernelArg(raytrace_kernel, 4, sizeof(uint32_t), &scene->materialCount);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 5, sizeof(cl_mem), &context->cl.planes);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 6, sizeof(Plane) * scene->planeCount, NULL); // sharedMemory planes
	context->cl.err |= clSetKernelArg(raytrace_kernel, 7, sizeof(uint32_t), &scene->planeCount);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 8, sizeof(cl_mem), &context->cl.spheres);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 9, sizeof(Sphere) * scene->sphereCount, NULL); // sharedMemory spheres
	context->cl.err |= clSetKernelArg(raytrace_kernel, 10, sizeof(uint32_t), &scene->sphereCount);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 11, sizeof(cl_mem), &context->cl.triangles);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 12, sizeof(Triangle) * scene->triangleCount, NULL); // sharedMemory triangles
	context->cl.err |= clSetKernelArg(raytrace_kernel, 13, sizeof(uint32_t), &scene->triangleCount);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 14, sizeof(cl_mem), &context->cl.pointLights);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 15, sizeof(PointLight) * scene->pointLightCount, NULL); // sharedMemory pointLights
	context->cl.err |= clSetKernelArg(raytrace_kernel, 16, sizeof(uint32_t), &scene->pointLightCount);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 17, sizeof(cl_mem), &context->cl.image);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 18, sizeof(float), &rayColorContribution);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 19, sizeof(float), &deltaX);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 20, sizeof(float), &deltaY);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 21, sizeof(float), &pixelWidth);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 22, sizeof(float), &pixelHeight);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 23, sizeof(uint32_t), &raysPerWidthPixel);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 24, sizeof(uint32_t), &raysPerHeightPixel);
	if (context->cl.err != CL_SUCCESS) {
		printf("Couldn't set all kernel args correctly.\n");
		return false;
	}
	return true;
}

static void gpu_deleteCLMemory(GPUContext* context) {
	clReleaseKernel(context->cl.kernel);
	clReleaseMemObject(context->cl.camera);
	clReleaseMemObject(context->cl.materials);
	clReleaseMemObject(context->cl.planes);
	clReleaseMemObject(context->cl.spheres);
	clReleaseMemObject(context->cl.triangles);
	clReleaseMemObject(context->cl.pointLights);
}


// -------------------- OPENGL --------------------

static void gpu_initGLContext(GPUContext* context, uint32_t renderWidth, uint32_t renderHeight);
static GLuint gpu_compileShaderProgram(const char* vertexShaderSrc, const char* fragmentShaderSrc);

void gpu_updateDimensions(GPUContext* context, uint32_t viewWidth, uint32_t viewHeight, uint32_t renderWidth, uint32_t renderHeight);
static void gpu_deleteGLObjects(GPUContext* context);


static void gpu_initGLContext(GPUContext* context, uint32_t renderWidth, uint32_t renderHeight) {

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

	// GL Initialization code
	glGenVertexArrays(1, &context->gl.vao);
	glBindVertexArray(context->gl.vao);

	glGenBuffers(1, &context->gl.vbo);

	glBindBuffer(GL_ARRAY_BUFFER, context->gl.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &context->gl.ibo);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, context->gl.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);

	glEnable(GL_TEXTURE_2D);

	glGenTextures(1, &context->gl.texture);
	glBindTexture(GL_TEXTURE_2D, context->gl.texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, renderWidth, renderHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	context->gl.shaderProgram = gpu_compileShaderProgram(vertexShader, fragmentShader);

	// set initial drawing state once
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glUseProgram(context->gl.shaderProgram);
	glBindVertexArray(context->gl.vao);

	context->gl.texture1Location = glGetUniformLocation(context->gl.shaderProgram, "texture1");
	glUniform1i(context->gl.texture1Location, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, context->gl.texture);

	// initially no scaling is required, because renderAspectRatio is the same as viewAspectRatio
	context->gl.scaleLocation = glGetUniformLocation(context->gl.shaderProgram, "scale");
	glUniform2f(context->gl.scaleLocation, 1.0f, 1.0f);
	glViewport(0, 0, renderWidth, renderHeight);
}

static GLuint gpu_compileShaderProgram(const char* vertexShaderSrc, const char* fragmentShaderSrc) {
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSrc, NULL);
	glCompileShader(vertexShader);
	GLint success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		printf("Vertex Shader Compilation Error:\n%s\n", infoLog);
	}

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSrc, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		printf("Fragment Shader Compilation Error:\n%s\n", infoLog);
	}

	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		printf("Shader Program Compilation Error\n%s\n", infoLog);
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	return shaderProgram;
}

static void gpu_deleteGLObjects(GPUContext* context) {
	glDeleteVertexArrays(1, &context->gl.vao);
	glDeleteBuffers(1, &context->gl.vbo);
	glDeleteBuffers(1, &context->gl.ibo);
	glDeleteTextures(1, &context->gl.texture);
}


#endif //RAYTRACER_GPU_H
