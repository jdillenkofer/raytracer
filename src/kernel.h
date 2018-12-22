#ifndef RAYTRACER_KERNEL_H
#define RAYTRACER_KERNEL_H

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "utils/file.h"

typedef struct {
    cl_platform_id plat;
    cl_device_id dev;
    cl_context ctx;
    cl_command_queue command_queue;
    cl_program prog;
    cl_int err;
} oclContext;

oclContext* initOpenClContext() {
    oclContext* context = malloc(sizeof(oclContext));
    clGetPlatformIDs(1, &context->plat, NULL);
    clGetDeviceIDs(context->plat, CL_DEVICE_TYPE_GPU, 1, &context->dev, NULL);
    context->ctx = clCreateContext(0, 1, &context->dev, NULL, NULL, &context->err);
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

#include "utils/image.h"
#include "scene.h"

cl_mem image_create_gpu(oclContext* oclContext, Image* image) {
    cl_mem dev_image = clCreateBuffer(oclContext->ctx, CL_MEM_WRITE_ONLY, sizeof(uint32_t) * image->width * image->height, image->buffer, &oclContext->err);
    if (oclContext->err != CL_SUCCESS) {
        printf("Couldn't create dev_image.\n");
        return NULL;
    }
    return dev_image;
}

cl_mem scene_create_gpu_camera(oclContext* oclContext, Scene* scene) {
    cl_mem dev_camera = (void*) clCreateBuffer(oclContext->ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Camera), scene->camera, &oclContext->err);
    if (oclContext->err != CL_SUCCESS) {
        printf("Couldn't create dev_camera.\n");
        return NULL;
    }
    return dev_camera;
}

cl_mem scene_create_gpu_materials(oclContext* oclContext, Scene* scene) {
    cl_mem dev_materials = (void*) clCreateBuffer(oclContext->ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Material) * scene->materialCount, scene->materials, &oclContext->err);
    if (oclContext->err != CL_SUCCESS) {
        printf("Couldn't create dev_materials.\n");
        return NULL;
    }
    return dev_materials;
}

cl_mem scene_create_gpu_planes(oclContext* oclContext, Scene* scene) {
    cl_mem dev_planes = (void*) clCreateBuffer(oclContext->ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Plane) * scene->planeCount, scene->planes, &oclContext->err);
    if (oclContext->err != CL_SUCCESS) {
        printf("Couldn't create dev_planes.\n");
        return NULL;
    }
    return dev_planes;
}

cl_mem scene_create_gpu_spheres(oclContext* oclContext, Scene* scene) {
    cl_mem dev_spheres = (void*) clCreateBuffer(oclContext->ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Sphere) * scene->sphereCount, scene->spheres, &oclContext->err);
    if (oclContext->err != CL_SUCCESS) {
        printf("Couldn't create dev_spheres.\n");
        return NULL;
    }
    return dev_spheres;
}

cl_mem scene_create_gpu_triangles(oclContext* oclContext, Scene* scene) {
    cl_mem dev_triangles = (void*) clCreateBuffer(oclContext->ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Triangle) * scene->triangleCount, scene->triangles, &oclContext->err);
    if (oclContext->err != CL_SUCCESS) {
        printf("Couldn't create dev_triangles.\n");
        return NULL;
    }
    return dev_triangles;
}

cl_mem scene_create_gpu_pointLights(oclContext *oclContext, Scene *scene) {
    cl_mem dev_pointLights = (void*) clCreateBuffer(oclContext->ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(PointLight) * scene->pointLightCount, scene->pointLights, &oclContext->err);
    if (oclContext->err != CL_SUCCESS) {
        printf("Couldn't create dev_pointlights.\n");
        return NULL;
    }
    return dev_pointLights;
}

void destroyOpenClContext(oclContext* oclContext) {
    if (oclContext) {
        clReleaseProgram(oclContext->prog);
        clReleaseCommandQueue(oclContext->command_queue);
        clReleaseContext(oclContext->ctx);
        free(oclContext);
    }
}


#endif //RAYTRACER_KERNEL_H
