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
#include "octree.h"

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
		cl_mem octreeNodes;
		cl_mem octreeIndexes;
        cl_mem randomSeed;
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

GPUContext* gpu_initContext(Scene* scene, Octree* octree, uint32_t raysPerPixel);
void gpu_renderScene(GPUContext* context, Scene* scene, Image* image);
void gpu_destroyContext(GPUContext* context);

// -------------------- OPENGL --------------------

void gpu_updateDimensions(GPUContext* context, uint32_t viewWidth, uint32_t viewHeight, uint32_t renderWidth, uint32_t renderHeight);
#endif //RAYTRACER_GPU_H
