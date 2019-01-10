#include "gpu.h"

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

// -------------------- OPENGL--------------------

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