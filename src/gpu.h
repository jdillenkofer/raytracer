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
    cl_platform_id plat;
    cl_device_id dev;
    cl_context ctx;
    cl_command_queue command_queue;
    cl_program prog;
    cl_int err;
} gpuContext;

gpuContext* gpu_initContext() {
    gpuContext* context = malloc(sizeof(gpuContext));
    clGetPlatformIDs(1, &context->plat, NULL);
    clGetDeviceIDs(context->plat, CL_DEVICE_TYPE_GPU, 1, &context->dev, NULL);

	/*
	 * These properties are required for the OpenGL <-> OpenCL interop
	 * Note: The OpenGL context needs to be created before creating the OpenCL context.
	 */
#ifdef WIN32
	cl_context_properties props[] = { 
		CL_GL_CONTEXT_KHR, (cl_context_properties) wglGetCurrentContext(),
		CL_WGL_HDC_KHR, (cl_context_properties) wglGetCurrentDC(),
		CL_CONTEXT_PLATFORM, (cl_context_properties) context->plat,
		0 };
#endif
#ifdef __linux__
    cl_context_properties props[] = {
            CL_GL_CONTEXT_KHR, (cl_context_properties) glXGetCurrentContext(),
            CL_GLX_DISPLAY_KHR, (cl_context_properties) glXGetCurrentDisplay(),
            CL_CONTEXT_PLATFORM, (cl_context_properties) context->plat,
            0};
#endif
    context->ctx = clCreateContext(props, 1, &context->dev, NULL, NULL, &context->err);
    context->command_queue = clCreateCommandQueue(context->ctx, context->dev, 0, &context->err);

    size_t sourceSize = 0;
    char* source = file_readFile("kernel.cl", &sourceSize);

    context->prog = clCreateProgramWithSource(context->ctx, 1, &source, &sourceSize, &context->err);
    clBuildProgram(context->prog, 1, &context->dev, NULL, NULL, NULL);
#ifndef NDEBUG
    cl_build_status status;
    clGetProgramBuildInfo(context->prog, context->dev, CL_PROGRAM_BUILD_STATUS, sizeof(cl_build_status), &status, NULL);
    if (status != CL_BUILD_SUCCESS) {
        char* log;
        size_t log_size = 0;

        // get the size of the log
        clGetProgramBuildInfo(context->prog, context->dev, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

        log = malloc(sizeof(char) * (log_size + 1));
        // get the log itself
        clGetProgramBuildInfo(context->prog, context->dev, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        log[log_size] = '\0';
        // print the log
        printf("Build log:\n%s\n", log);
        free(log);
        return NULL;
    }
#endif
    return context;
}

cl_mem gpu_createImageBufferFromTextureId(gpuContext* context, GLuint textureId) {
	cl_mem dev_image = clCreateFromGLTexture(context->ctx, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, textureId, &context->err);
    if (context->err != CL_SUCCESS) {
        printf("Couldn't create dev_image.\n");
        return NULL;
    }
    return dev_image;
}

cl_mem gpu_createCameraBuffer(gpuContext* context, Scene* scene) {
    cl_mem dev_camera = (void*) clCreateBuffer(context->ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Camera), scene->camera, &context->err);
    if (context->err != CL_SUCCESS) {
        printf("Couldn't create dev_camera.\n");
        return NULL;
    }
    return dev_camera;
}

cl_mem gpu_createMaterialsBuffer(gpuContext* context, Scene* scene) {
    cl_mem dev_materials = (void*) clCreateBuffer(context->ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Material) * scene->materialCount, scene->materials, &context->err);
    if (context->err != CL_SUCCESS) {
        printf("Couldn't create dev_materials.\n");
        return NULL;
    }
    return dev_materials;
}

cl_mem gpu_createPlanesBuffer(gpuContext* context, Scene* scene) {
    cl_mem dev_planes = (void*) clCreateBuffer(context->ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Plane) * scene->planeCount, scene->planes, &context->err);
    if (context->err != CL_SUCCESS) {
        printf("Couldn't create dev_planes.\n");
        return NULL;
    }
    return dev_planes;
}

cl_mem gpu_createSpheresBuffer(gpuContext* context, Scene* scene) {
    cl_mem dev_spheres = (void*) clCreateBuffer(context->ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Sphere) * scene->sphereCount, scene->spheres, &context->err);
    if (context->err != CL_SUCCESS) {
        printf("Couldn't create dev_spheres.\n");
        return NULL;
    }
    return dev_spheres;
}

cl_mem gpu_createTrianglesBuffer(gpuContext* context, Scene* scene) {
    cl_mem dev_triangles = (void*) clCreateBuffer(context->ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Triangle) * scene->triangleCount, scene->triangles, &context->err);
    if (context->err != CL_SUCCESS) {
        printf("Couldn't create dev_triangles.\n");
        return NULL;
    }
    return dev_triangles;
}

cl_mem gpu_createPointLightsBuffer(gpuContext* context, Scene* scene) {
    cl_mem dev_pointLights = (void*) clCreateBuffer(context->ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(PointLight) * scene->pointLightCount, scene->pointLights, &context->err);
    if (context->err != CL_SUCCESS) {
        printf("Couldn't create dev_pointlights.\n");
        return NULL;
    }
    return dev_pointLights;
}

uint32_t gpu_compileShaderProgram(const char* vertexShaderSrc, const char* fragmentShaderSrc) {
	unsigned int vertexShader;
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSrc, NULL);
	glCompileShader(vertexShader);
	int  success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s\n", infoLog);
	}

	unsigned int fragmentShader;
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSrc, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s\n", infoLog);
	}

	unsigned int shaderProgram;
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		printf("ERROR::SHADER::PROGRAM::COMPILATION_FAILED\n%s\n", infoLog);
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	return shaderProgram;
}

void gpu_destroyContext(gpuContext* context) {
    if (context) {
        clReleaseProgram(context->prog);
        clReleaseCommandQueue(context->command_queue);
        clReleaseContext(context->ctx);
        free(context);
    }
}


#endif //RAYTRACER_GPU_H
