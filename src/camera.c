#include "camera.h"

#include <stdio.h>
#include <stdlib.h>
#include "utils/random.h"
#include "utils/math.h"

// See: http://web.cse.ohio-state.edu/~shen.94/681/Site/Slides_files/basic_algo.pdf
void camera_setup(Camera *camera) {
    // Setup Camera Coordinate System
    //
    camera->z = vec3_norm(vec3_sub(camera->position, camera->lookAt));
    camera->x = vec3_norm(vec3_cross((Vec3) { 0.0f, 1.0f, 0.0f }, camera->z));
    camera->y = vec3_norm(vec3_cross(camera->z, camera->x));

    camera->renderTargetWidth = 1.0f;
    camera->renderTargetHeight = 1.0f;
    if (camera->width > camera->height) {
        camera->renderTargetHeight = camera->renderTargetWidth * 1.0f/ camera->aspectRatio;
    } else if (camera->height > camera->width) {
        camera->renderTargetWidth = camera->renderTargetHeight * camera->aspectRatio;
    }

    // calculate Distance between RenderTarget and Eye
    float diagonale = sqrtf(
            (camera->renderTargetWidth * camera->renderTargetWidth) +
            (camera->renderTargetHeight * camera->renderTargetHeight));
    camera->renderTargetDistance = (diagonale * (2.0f*tanf(math_deg2rad(camera->FOV)/2.0f)));

    // centerPos on RenderTarget
    camera->renderTargetCenter = vec3_sub(camera->position, vec3_mul(camera->z, camera->renderTargetDistance));
}

Camera* camera_create(Vec3 position, Vec3 lookAt, uint32_t width, uint32_t height, float FOV, float apertureSize) {
    Camera* camera = malloc(sizeof(Camera));
    camera->position = position;
    camera->width = width;
    camera->height = height;
    camera->aspectRatio = (float) width/ (float)height;
    camera->lookAt = lookAt;
    camera->FOV = FOV;
    camera->focalLength = vec3_length(vec3_sub(position, lookAt));
    camera->apertureSize = apertureSize;
    camera_setup(camera);
    return camera;
}

void move_camera(Camera *camera, int upDown, int side, int frontal) {
    camera->position = vec3_add(camera->position, vec3_mul(camera->x, side * 0.5f));
    camera->position = vec3_add(camera->position, vec3_mul(camera->y, upDown * 0.5f));
    camera->position = vec3_add(camera->position, vec3_mul(camera->z, frontal * 0.5f));
}

void camera_destroy(Camera* cam) {
    if (cam) {
        free(cam);
    }
}
