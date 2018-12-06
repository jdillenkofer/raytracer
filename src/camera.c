#include "camera.h"

#include <stdio.h>
#include <stdlib.h>
#include "utils/random.h"
#include "utils/angle.h"

// See: http://web.cse.ohio-state.edu/~shen.94/681/Site/Slides_files/basic_algo.pdf
static void camera_setup(Camera *camera) {
    // Setup Camera Coordinate System
    //
    camera->z = vec3_norm(vec3_sub(camera->position, camera->lookAt));
    camera->x = vec3_norm(vec3_cross((Vec3) { 0, 1, 0 }, camera->z));
    camera->y = vec3_cross(camera->z, camera->x);

    camera->renderTargetWidth = 1.0f;
    camera->renderTargetHeight = 1.0f;
    if (camera->width > camera->height) {
        camera->renderTargetHeight = camera->renderTargetWidth * 1.0f/ camera->aspectRatio;
    } else if (camera->height > camera->width) {
        camera->renderTargetWidth = camera->renderTargetHeight * camera->aspectRatio;
    }

    // calculate Distance between RenderTarget and Eye
    double diagonale = sqrt(
            (camera->renderTargetWidth * camera->renderTargetWidth) +
            (camera->renderTargetHeight * camera->renderTargetHeight));
    camera->renderTargetDistance = (diagonale * (2.0f*tan(angle_deg2rad(camera->FOV)/2.0f)));

    // centerPos on RenderTarget
    camera->renderTargetCenter = vec3_sub(camera->position, vec3_mul(camera->z, camera->renderTargetDistance));
}

Camera* camera_create(Vec3 position, Vec3 lookAt, uint32_t width, uint32_t height, double FOV) {
    Camera* camera = malloc(sizeof(Camera));
    camera->position = position;
    camera->width = width;
    camera->height = height;
    camera->aspectRatio = (double) width/ (double)height;
    camera->lookAt = lookAt;
    camera->FOV = FOV;
	camera->halfPixelWidth = 0.5f / width;
	camera->halfPixelHeight = 0.5f / height;
    camera_setup(camera);
    return camera;
}

Ray camera_rayFromPixelPos(Camera *camera, uint32_t x, uint32_t y) {
	double PosY = -1.0f + 2.0f * ((double)y / (double)camera->height) + random_bilateral() * camera->halfPixelHeight;
	double PosX = -1.0f + 2.0f * ((double)x / (double)camera->width) + random_bilateral() * camera->halfPixelWidth;
    Vec3 renderTargetPos = vec3_add(vec3_add(camera->renderTargetCenter,
            vec3_mul(camera->x, PosX*camera->renderTargetWidth/2.0f)),
            vec3_mul(camera->y, PosY*camera->renderTargetHeight/2.0f));
    Ray outgoingRay = {
        camera->position,
        vec3_norm(vec3_sub(renderTargetPos, camera->position))
    };
    return outgoingRay;
}

void camera_destroy(Camera* cam) {
    if (cam) {
        free(cam);
    }
}
