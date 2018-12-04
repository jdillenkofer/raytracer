#include "camera.h"

#include <stdlib.h>
#include "utils/angle.h"

// See: http://web.cse.ohio-state.edu/~shen.94/681/Site/Slides_files/basic_algo.pdf
static void camera_setup(Camera *camera) {
    // Setup Camera Coordinate System
    //
    camera->z = vec3_norm(camera->position);
    camera->x = vec3_norm(vec3_cross(camera->up, camera->z));
    camera->y = vec3_cross(camera->z, camera->x);

    // calculate Distance between RenderTarget and Eye
    camera->distanceToRenderTarget = 1; //(camera->renderDim.width/(2.0f*tan(angle_deg2rad(camera->hFOV)/2.0f)));

    // calculate CPosition centerPos on RenderTarget
    camera->cPosition = vec3_sub(camera->position, vec3_mul(camera->z, camera->distanceToRenderTarget));

    // calculate LPosition leftPos on RenderTarget
    Vec3 cameraX_times_W_div_2 = vec3_div(camera->x, (double) camera->renderDim.width/2.0f);
    Vec3 cameraY_times_H_div2 = vec3_div(camera->y, (double) camera->renderDim.height/2.0f);
    camera->lPosition = vec3_sub(vec3_sub(camera->cPosition, cameraX_times_W_div_2), cameraY_times_H_div2);
}

Camera* camera_create(Vec3 position, Vec3 up, Vec3 lookAt, Dimension renderDim, double hFOV) {
    Camera* camera = malloc(sizeof(Camera));
    camera->position = position;
    camera->renderDim = renderDim;
    camera->up = up;
    camera->lookAt = lookAt;
    camera->hFOV = hFOV;
    camera_setup(camera);
    return camera;
}

Ray camera_ray_from_pixel(Camera *camera, uint32_t x, uint32_t y) {
    Vec3 cameraXScaledByXTimesWidth = vec3_mul(camera->x, x * camera->renderDim.width);
    Vec3 cameraYScaledByYTimesHeight = vec3_mul(camera->y, y * camera->renderDim.height);
    Vec3 pixelOnPlane = vec3_add(vec3_add(camera->lPosition, cameraXScaledByXTimesWidth), cameraYScaledByYTimesHeight);
    double FilmY = -1.0f + 2.0f * ((double) y / (double) camera->renderDim.height);
    double FilmX = -1.0f + 2.0f * ((double) x / (double) camera->renderDim.width);
    Vec3 FilmP = vec3_add(vec3_add(camera->cPosition, vec3_mul(camera->x, FilmX)), vec3_mul(camera->y, FilmY));
    Ray outgoingRay = {
        camera->position,
        vec3_norm(vec3_sub(pixelOnPlane, camera->position))
    };
    return outgoingRay;
}

void camera_destroy(Camera* cam) {
    if (cam) {
        free(cam);
    }
}
