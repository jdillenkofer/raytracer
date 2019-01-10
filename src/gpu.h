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
		cl_platform_id plat;
		cl_device_id dev;
		cl_context ctx;
		cl_command_queue command_queue;
		cl_program prog;
		cl_kernel raytrace_kernel;
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
		GLint texture1Loc;
		GLint scaleLoc;
	} gl;
} GPUContext;

// -------------------- OPENCL --------------------

GPUContext* gpu_initCLContext();
// this needs to be done after gl texture creation
bool gpu_allocateCLMemory(GPUContext* context, Scene* scene);
bool gpu_setupKernel(GPUContext* context, Scene* scene, uint32_t raysPerPixel);

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

void gpu_deleteCLMemory(GPUContext* context);


// -------------------- OPENGL --------------------

void gpu_initGLContext(GPUContext* context, uint32_t renderWidth, uint32_t renderHeight);
GLuint gpu_compileShaderProgram(const char* vertexShaderSrc, const char* fragmentShaderSrc);

void gpu_updateDimensions(GPUContext* context, uint32_t viewWidth, uint32_t viewHeight, uint32_t renderWidth, uint32_t renderHeight);
void gpu_deleteGLObjects(GPUContext* context);

// -------------------- MIXED --------------------

GPUContext* gpu_initContext(Scene* scene, uint32_t raysPerPixel);
void gpu_renderScene(GPUContext* context, Scene* scene);
void gpu_destroyContext(GPUContext* context);


#endif //RAYTRACER_GPU_H
