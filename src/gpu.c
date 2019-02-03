#include "gpu.h"

#include <math.h>
#include "utils/random.h"
#include "utils/stringbuilder.h"

// -------------------- OPENCL STATIC DECLS --------------------

static GPUContext* gpu_initCLContext();
// this needs to be done after gl texture creation
static bool gpu_allocateCLMemory(GPUContext* context, Scene* scene, Octree* octree);
static bool gpu_setupKernel(GPUContext* context, Scene* scene, Octree* octree, uint32_t raysPerPixel);
static void gpu_deleteCLMemory(GPUContext* context);

// -------------------- OPENGL STATIC DECLS --------------------

static void gpu_initGLContext(GPUContext* context, uint32_t renderWidth, uint32_t renderHeight);
static GLuint gpu_compileShaderProgram(const char* vertexShaderSrc, const char* fragmentShaderSrc);
static void gpu_deleteGLObjects(GPUContext* context);

// -------------------- MIXED --------------------

GPUContext* gpu_initContext(Scene* scene, Octree* octree, uint32_t raysPerPixel) {
	GPUContext* context = gpu_initCLContext();
	if (!context) {
		return NULL;
	}
	gpu_initGLContext(context, scene->camera->width, scene->camera->height);
    if (!gpu_allocateCLMemory(context, scene, octree) || !gpu_setupKernel(context, scene, octree, raysPerPixel)) {
        return NULL;
    }
	return context;
}

void gpu_renderScene(GPUContext* context, Scene* scene, Image* image) {
	glFinish();
	clEnqueueWriteBuffer(context->cl.commandQueue, context->cl.camera, CL_TRUE, 0, sizeof(Camera), scene->camera, 0, NULL, NULL);
	context->cl.err = clSetKernelArg(context->cl.kernel, 0, sizeof(cl_mem), &context->cl.camera);
	const size_t threadsPerDim[2] = { scene->camera->width, scene->camera->height };
	clEnqueueAcquireGLObjects(context->cl.commandQueue, 1, &context->cl.image, 0, NULL, NULL);
	context->cl.err = clEnqueueNDRangeKernel(context->cl.commandQueue, context->cl.kernel, 2, NULL, threadsPerDim, NULL, 0, NULL, NULL);
	if (context->cl.err != CL_SUCCESS) {
		printf("Couldn't enqueue kernel.\n");
		return;
	}
	if (image != NULL) {
		size_t origin[3] = { 0, 0, 0 };
		size_t region[3] = { image->width, image->height, 1 };
		size_t rowPitch = sizeof(uint32_t) * image->width;
		size_t slicePitch = 0;
		clEnqueueReadImage(context->cl.commandQueue, context->cl.image, CL_TRUE, origin, region, rowPitch, slicePitch, image->buffer, 0, NULL, NULL);
	}
	clEnqueueReleaseGLObjects(context->cl.commandQueue, 1, &context->cl.image, 0, NULL, NULL);
	context->cl.err = clFinish(context->cl.commandQueue);

	glClear(GL_COLOR_BUFFER_BIT);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void gpu_destroyContext(GPUContext* context) {
	if (context) {
		gpu_deleteGLObjects(context);
		gpu_deleteCLMemory(context);

		clReleaseProgram(context->cl.program);
		clReleaseCommandQueue(context->cl.commandQueue);
		clReleaseContext(context->cl.ctx);
		free(context);
	}
}

// -------------------- OPENCL --------------------

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

static cl_mem gpu_createOctreeNodesBuffer(GPUContext* context, Octree* octree) {
	cl_mem dev_octreeNodes = (void*)clCreateBuffer(context->cl.ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(OctreeNode) * octree->nodeCount, octree->nodes, &context->cl.err);
	if (context->cl.err != CL_SUCCESS) {
		printf("Couldn't create dev_octreeNodes.\n");
		return NULL;
	}
	return dev_octreeNodes;
}

static cl_mem gpu_createOctreeIndexesBuffer(GPUContext* context, Octree* octree) {
	cl_mem dev_octreeIndexes = (void*)clCreateBuffer(context->cl.ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(uint32_t) * octree->indexCount, octree->indexes, &context->cl.err);
	if (context->cl.err != CL_SUCCESS) {
		printf("Couldn't create dev_octreeIndexes.\n");
		return NULL;
	}
	return dev_octreeIndexes;
}

static cl_mem gpu_createRandomSeedBuffer(GPUContext* context, Scene* scene) {
    size_t seedSize = sizeof(seed128bit) * scene->camera->width * scene->camera->height;
    seed128bit* seed = malloc(seedSize);
    for (uint32_t i = 0; i < scene->camera->width * scene->camera->height; i++) {
        seed128bit rSeed;
        rSeed.x = rand();
        rSeed.y = rand();
        seed[i] = rSeed;
    }
    cl_mem dev_randomSeed = (void*)clCreateBuffer(context->cl.ctx, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, seedSize, seed, &context->cl.err);
    free(seed);
    if (context->cl.err != CL_SUCCESS) {
        printf("Couldn't create dev_randomSeed.\n");
        return NULL;
    }
    return dev_randomSeed;
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
	return context;
}

static bool gpu_allocateCLMemory(GPUContext* context, Scene* scene, Octree* octree) {
    context->cl.image = NULL;
    context->cl.camera = NULL;
    context->cl.materials = NULL;
    context->cl.planes = NULL;
    context->cl.spheres = NULL;
    context->cl.triangles = NULL;
    context->cl.pointLights = NULL;
    context->cl.octreeNodes = NULL;
    context->cl.octreeIndexes = NULL;
    context->cl.randomSeed = NULL;
    
	context->cl.image = gpu_createImageBufferFromTextureId(context, context->gl.texture);
	if (!context->cl.image) {
		return false;
	}
	context->cl.camera = gpu_createCameraBuffer(context, scene);
	if (!context->cl.camera) {
		return false;
	}
    
    if (scene->materialCount > 0) {
        context->cl.materials = gpu_createMaterialsBuffer(context, scene);
        if (!context->cl.materials) {
            return false;
        }
    }
    
    if (scene->planeCount > 0) {
        context->cl.planes = gpu_createPlanesBuffer(context, scene);
        if (!context->cl.planes) {
            return false;
        }
    }
    
    if (scene->sphereCount > 0) {
        context->cl.spheres = gpu_createSpheresBuffer(context, scene);
        if (!context->cl.spheres) {
            return false;
        }
    }

    if (scene->triangleCount > 0) {
        context->cl.triangles = gpu_createTrianglesBuffer(context, scene);
        if (!context->cl.triangles) {
            return false;
        }
    }

    if (scene->pointLightCount > 0) {
        context->cl.pointLights = gpu_createPointLightsBuffer(context, scene);
        if (!context->cl.pointLights) {
            return false;
        }
    }

    if (octree->nodeCount > 0) {
        context->cl.octreeNodes = gpu_createOctreeNodesBuffer(context, octree);
        if (!context->cl.octreeNodes) {
            return false;
        }
    }

    if (octree->indexCount > 0) {
        context->cl.octreeIndexes = gpu_createOctreeIndexesBuffer(context, octree);
        if (!context->cl.octreeIndexes) {
            return false;
        }
    }

    context->cl.randomSeed = gpu_createRandomSeedBuffer(context, scene);
    if (!context->cl.randomSeed) {
        return false;
    }
	return true;
}

static bool gpu_setupKernel(GPUContext* context, Scene* scene, Octree* octree, uint32_t raysPerPixel) {
	// check which part of the scene, we can fit into shared memory
	const char* sharedMemDef = "#define USE_SHARED_MEMORY\n";
	const char* sharedMemCameraDef = "#define USE_SHARED_MEMORY_CAMERA\n";
	const char* sharedMemMaterialsDef = "#define USE_SHARED_MEMORY_MATERIALS\n";
	const char* sharedMemPlanesDef = "#define USE_SHARED_MEMORY_PLANES\n";
	const char* sharedMemSpheresDef = "#define USE_SHARED_MEMORY_SPHERES\n";
	const char* sharedMemTrianglesDef = "#define USE_SHARED_MEMORY_TRIANGLES\n";
	const char* sharedMemPointLightsDef = "#define USE_SHARED_MEMORY_POINTLIGHTS\n";
	const char* sharedMemOctreeNodesDef = "#define USE_SHARED_MEMORY_OCTREENODES\n";
	const char* sharedMemOctreeIndexesDef = "#define USE_SHARED_MEMORY_OCTREEINDEXES\n";

	bool useSharedMem = false;
	bool useSharedMemCamera = false;
	bool useSharedMemMaterials = false;
	bool useSharedMemPlanes = false;
	bool useSharedMemSpheres = false;
	bool useSharedMemTriangles = false;
	bool useSharedMemPointLights = false;
	bool useSharedMemOctreeNodes = false;
	bool useSharedMemOctreeIndexes = false;

	size_t sharedMemCameraSize = sizeof(Camera);
	size_t sharedMemMaterialsSize = sizeof(Material) * scene->materialCount;
	size_t sharedMemPlanesSize = sizeof(Plane) * scene->planeCount;
	size_t sharedMemSpheresSize = sizeof(Sphere) * scene->sphereCount;
	size_t sharedMemTrianglesSize = sizeof(Triangle) * scene->triangleCount;
	size_t sharedMemPointLightsSize = sizeof(PointLight) * scene->pointLightCount;
	size_t sharedMemOctreeNodesSize = sizeof(OctreeNode) * octree->nodeCount;
	size_t sharedMemOctreeIndexesSize = sizeof(uint32_t) * octree->indexCount;

	// check if the gpu has a dedicated faster low latency local memory
	// if not don't use shared memory at all, because the copying process just makes the kernel slower
	cl_device_local_mem_type type;
	cl_ulong availableLocalMemSize = 0;
	clGetDeviceInfo(context->cl.deviceId, CL_DEVICE_LOCAL_MEM_TYPE, sizeof(cl_device_local_mem_type), &type, NULL);
	if (type == CL_LOCAL) {
		// get the size of the dedicated local memory storage
		clGetDeviceInfo(context->cl.deviceId, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &availableLocalMemSize, NULL);
	}

	if (availableLocalMemSize >= sharedMemCameraSize) {
		useSharedMemCamera = true;
		availableLocalMemSize -= sharedMemCameraSize;
	} else {
		sharedMemCameraSize = 0;
	}

	if (availableLocalMemSize >= sharedMemOctreeNodesSize) {
		useSharedMemOctreeNodes = true;
		availableLocalMemSize -= sharedMemOctreeNodesSize;
	} else {
		sharedMemOctreeNodesSize = 0;
	}

	if (availableLocalMemSize >= sharedMemOctreeIndexesSize) {
		useSharedMemOctreeIndexes = true;
		availableLocalMemSize -= sharedMemOctreeIndexesSize;
	} else {
		sharedMemOctreeIndexesSize = 0;
	}

	if (availableLocalMemSize >= sharedMemMaterialsSize) {
		useSharedMemMaterials = true;
		availableLocalMemSize -= sharedMemMaterialsSize;
	} else {
		sharedMemMaterialsSize = 0;
	}

	if (availableLocalMemSize >= sharedMemPlanesSize) {
		useSharedMemPlanes = true;
		availableLocalMemSize -= sharedMemPlanesSize;
	} else {
		sharedMemPlanesSize = 0;
	}

	if (availableLocalMemSize >= sharedMemSpheresSize) {
		useSharedMemSpheres = true;
		availableLocalMemSize -= sharedMemSpheresSize;
	} else {
		sharedMemSpheresSize = 0;
	}

	if (availableLocalMemSize >= sharedMemTrianglesSize) {
		useSharedMemTriangles = true;
		availableLocalMemSize -= sharedMemTrianglesSize;
	} else {
		sharedMemTrianglesSize = 0;
	}

	if (availableLocalMemSize >= sharedMemPointLightsSize) {
		useSharedMemPointLights = true;
		availableLocalMemSize -= sharedMemPointLightsSize;
	} else {
		sharedMemPointLightsSize = 0;
	}

	if (useSharedMemCamera || useSharedMemMaterials || useSharedMemPlanes || useSharedMemSpheres || useSharedMemTriangles || useSharedMemPointLights || useSharedMemOctreeNodes || useSharedMemOctreeIndexes) {
		useSharedMem = true;
	}

	size_t sourceSize = 0;
	const char* source = file_readFile("kernel.cl", &sourceSize);

	StringBuilder* builder = stringbuilder_create(sourceSize + 1000L);
	if (useSharedMem) {
		stringbuilder_append(builder, sharedMemDef);
	}
	if (useSharedMemCamera) {
		stringbuilder_append(builder, sharedMemCameraDef);
	}
	if (useSharedMemMaterials) {
		stringbuilder_append(builder, sharedMemMaterialsDef);
	}
	if (useSharedMemPlanes) {
		stringbuilder_append(builder, sharedMemPlanesDef);
	}
	if (useSharedMemSpheres) {
		stringbuilder_append(builder, sharedMemSpheresDef);
	}
	if (useSharedMemTriangles) {
		stringbuilder_append(builder, sharedMemTrianglesDef);
	}
	if (useSharedMemPointLights) {
		stringbuilder_append(builder, sharedMemPointLightsDef);
	}
	if (useSharedMemOctreeNodes) {
		stringbuilder_append(builder, sharedMemOctreeNodesDef);
	}
	if (useSharedMemOctreeIndexes) {
		stringbuilder_append(builder, sharedMemOctreeIndexesDef);
	}
	stringbuilder_append(builder, source);
	source = stringbuilder_cstr(builder);
	sourceSize = builder->length;
	stringbuilder_destroy(builder);
	
	context->cl.program = clCreateProgramWithSource(context->cl.ctx, 1, &source, &sourceSize, &context->cl.err);

	free((void*) source);
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
		return false;
	}
#endif

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
	context->cl.err |= clSetKernelArg(raytrace_kernel, 1, sharedMemCameraSize, NULL); // sharedMemory camera
	context->cl.err |= clSetKernelArg(raytrace_kernel, 2, sizeof(cl_mem), &context->cl.materials);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 3, sharedMemMaterialsSize, NULL); // sharedMemory materials
	context->cl.err |= clSetKernelArg(raytrace_kernel, 4, sizeof(uint32_t), &scene->materialCount);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 5, sizeof(cl_mem), &context->cl.planes);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 6, sharedMemPlanesSize, NULL); // sharedMemory planes
	context->cl.err |= clSetKernelArg(raytrace_kernel, 7, sizeof(uint32_t), &scene->planeCount);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 8, sizeof(cl_mem), &context->cl.spheres);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 9, sharedMemSpheresSize, NULL); // sharedMemory spheres
	context->cl.err |= clSetKernelArg(raytrace_kernel, 10, sizeof(uint32_t), &scene->sphereCount);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 11, sizeof(cl_mem), &context->cl.triangles);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 12, sharedMemTrianglesSize, NULL); // sharedMemory triangles
	context->cl.err |= clSetKernelArg(raytrace_kernel, 13, sizeof(uint32_t), &scene->triangleCount);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 14, sizeof(cl_mem), &context->cl.pointLights);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 15, sharedMemPointLightsSize, NULL); // sharedMemory pointLights
	context->cl.err |= clSetKernelArg(raytrace_kernel, 16, sizeof(uint32_t), &scene->pointLightCount);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 17, sizeof(cl_mem), &context->cl.octreeNodes);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 18, sharedMemOctreeNodesSize, NULL); // sharedMemory octreeNodes
	context->cl.err |= clSetKernelArg(raytrace_kernel, 19, sizeof(uint32_t), &octree->nodeCount);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 20, sizeof(cl_mem), &context->cl.octreeIndexes);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 21, sharedMemOctreeIndexesSize, NULL); // sharedMemory octreeIndexes
	context->cl.err |= clSetKernelArg(raytrace_kernel, 22, sizeof(uint32_t), &octree->indexCount);
    context->cl.err |= clSetKernelArg(raytrace_kernel, 23, sizeof(cl_mem), &context->cl.randomSeed);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 24, sizeof(cl_mem), &context->cl.image);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 25, sizeof(float), &rayColorContribution);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 26, sizeof(float), &deltaX);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 27, sizeof(float), &deltaY);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 28, sizeof(float), &pixelWidth);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 29, sizeof(float), &pixelHeight);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 30, sizeof(uint32_t), &raysPerWidthPixel);
	context->cl.err |= clSetKernelArg(raytrace_kernel, 31, sizeof(uint32_t), &raysPerHeightPixel);
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
	clReleaseMemObject(context->cl.octreeNodes);
	clReleaseMemObject(context->cl.octreeIndexes);
    clReleaseMemObject(context->cl.randomSeed);
}

// -------------------- OPENGL--------------------

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

void gpu_updateDimensions(GPUContext* context, uint32_t viewWidth, uint32_t viewHeight, uint32_t renderWidth, uint32_t renderHeight) {
	float xScale = 1.0f;
	float yScale = 1.0f;
	float viewAspectRatio = viewWidth / (float)viewHeight;
	float renderAspectRatio = renderWidth / (float)renderHeight;
	if (renderAspectRatio > viewAspectRatio) {
		yScale = viewAspectRatio / renderAspectRatio;
	}
	else {
		xScale = renderAspectRatio / viewAspectRatio;
	}
	glUniform2f(context->gl.scaleLocation, xScale, yScale);
	glViewport(0, 0, viewWidth, viewHeight);
}

static void gpu_deleteGLObjects(GPUContext* context) {
	glDeleteVertexArrays(1, &context->gl.vao);
	glDeleteBuffers(1, &context->gl.vbo);
	glDeleteBuffers(1, &context->gl.ibo);
	glDeleteTextures(1, &context->gl.texture);
}