#include "gpu.h"


// -------------------- OPENCL --------------------

GPUContext* gpu_initCLContext() {
	GPUContext* context = malloc(sizeof(GPUContext));
	clGetPlatformIDs(1, &context->cl.plat, NULL);
	clGetDeviceIDs(context->cl.plat, CL_DEVICE_TYPE_GPU, 1, &context->cl.dev, NULL);

	/*
	 * These properties are required for the OpenGL <-> OpenCL interop
	 * Note: The OpenGL context needs to be created before creating the OpenCL context.
	 */
#ifdef WIN32
	cl_context_properties props[] = {
		CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
		CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
		CL_CONTEXT_PLATFORM, (cl_context_properties)context->cl.plat,
		0 };
#endif
#ifdef __linux__
	cl_context_properties props[] = {
			CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(),
			CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(),
			CL_CONTEXT_PLATFORM, (cl_context_properties)context->plat,
			0 };
#endif
	context->cl.ctx = clCreateContext(props, 1, &context->cl.dev, NULL, NULL, &context->cl.err);
	context->cl.command_queue = clCreateCommandQueue(context->cl.ctx, context->cl.dev, 0, &context->cl.err);

	size_t sourceSize = 0;
	char* source = file_readFile("kernel.cl", &sourceSize);

	context->cl.prog = clCreateProgramWithSource(context->cl.ctx, 1, &source, &sourceSize, &context->cl.err);
	clBuildProgram(context->cl.prog, 1, &context->cl.dev, NULL, NULL, NULL);
#ifndef NDEBUG
	cl_build_status status;
	clGetProgramBuildInfo(context->cl.prog, context->cl.dev, CL_PROGRAM_BUILD_STATUS, sizeof(cl_build_status), &status, NULL);
	if (status != CL_BUILD_SUCCESS) {
		char* log;
		size_t log_size = 0;

		// get the size of the log
		clGetProgramBuildInfo(context->cl.prog, context->cl.dev, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

		log = malloc(sizeof(char) * (log_size + 1));
		// get the log itself
		clGetProgramBuildInfo(context->cl.prog, context->cl.dev, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
		log[log_size] = '\0';
		// print the log
		printf("Build log:\n%s\n", log);
		free(log);
		return NULL;
	}
#endif
	return context;
}

bool gpu_allocateCLMemory(GPUContext* context, Scene* scene) {
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

bool gpu_setupKernel(GPUContext* context, Scene* scene, uint32_t raysPerPixel) {
	cl_kernel raytrace_kernel = context->cl.raytrace_kernel = clCreateKernel(context->cl.prog, "raytrace", &context->cl.err);
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

void gpu_deleteCLMemory(GPUContext* context) {
	clReleaseKernel(context->cl.raytrace_kernel);
	clReleaseMemObject(context->cl.camera);
	clReleaseMemObject(context->cl.materials);
	clReleaseMemObject(context->cl.planes);
	clReleaseMemObject(context->cl.spheres);
	clReleaseMemObject(context->cl.triangles);
	clReleaseMemObject(context->cl.pointLights);
}

// -------------------- OPENGL --------------------

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

void gpu_initGLContext(GPUContext* context, uint32_t renderWidth, uint32_t renderHeight) {
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

	context->gl.texture1Loc = glGetUniformLocation(context->gl.shaderProgram, "texture1");
	glUniform1i(context->gl.texture1Loc, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, context->gl.texture);

	// initially no scaling is required, because renderAspectRatio is the same as viewAspectRatio
	context->gl.scaleLoc = glGetUniformLocation(context->gl.shaderProgram, "scale");
	glUniform2f(context->gl.scaleLoc, 1.0f, 1.0f);
	glViewport(0, 0, renderWidth, renderHeight);
}

GLuint gpu_compileShaderProgram(const char* vertexShaderSrc, const char* fragmentShaderSrc) {
	GLuint vertexShader;
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
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
	glUniform2f(context->gl.scaleLoc, xScale, yScale);
	glViewport(0, 0, viewWidth, viewHeight);
}

void gpu_deleteGLObjects(GPUContext* context) {
	glDeleteVertexArrays(1, &context->gl.vao);
	glDeleteBuffers(1, &context->gl.vbo);
	glDeleteBuffers(1, &context->gl.ibo);
	glDeleteTextures(1, &context->gl.texture);
}


// -------------------- MIXED --------------------

GPUContext* gpu_initContext(Scene* scene, uint32_t raysPerPixel) {
	GPUContext* context = gpu_initCLContext();
	if (!context) {
		return NULL;
	}
	gpu_initGLContext(context, scene->camera->width, scene->camera->height);
	gpu_allocateCLMemory(context, scene);
	gpu_setupKernel(context, scene, raysPerPixel);
	return context;
}

void gpu_renderScene(GPUContext* context, Scene* scene) {
	glFinish();
	clEnqueueWriteBuffer(context->cl.command_queue, context->cl.camera, CL_TRUE, 0, sizeof(Camera), scene->camera, 0, NULL, NULL);
	context->cl.err = clSetKernelArg(context->cl.raytrace_kernel, 0, sizeof(cl_mem), &context->cl.camera);
	const size_t threadsPerDim[2] = { scene->camera->width, scene->camera->height };
	clEnqueueAcquireGLObjects(context->cl.command_queue, 1, &context->cl.image, 0, NULL, NULL);
	context->cl.err = clEnqueueNDRangeKernel(context->cl.command_queue, context->cl.raytrace_kernel, 2, NULL, threadsPerDim, NULL, 0, NULL, NULL);
	if (context->cl.err != CL_SUCCESS) {
		printf("Couldn't enqueue kernel.\n");
		return 1;
	}
	clEnqueueReleaseGLObjects(context->cl.command_queue, 1, &context->cl.image, 0, NULL, NULL);
	context->cl.err = clFinish(context->cl.command_queue);

	glClear(GL_COLOR_BUFFER_BIT);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void gpu_destroyContext(GPUContext* context) {
	if (context) {
		gpu_deleteGLObjects(context);
		gpu_deleteCLMemory(context);

		clReleaseProgram(context->cl.prog);
		clReleaseCommandQueue(context->cl.command_queue);
		clReleaseContext(context->cl.ctx);
		free(context);
	}
}