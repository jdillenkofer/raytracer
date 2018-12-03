#include "camera.h"

#include <stdlib.h>
#include "util/angle.h"

// See: http://web.cse.ohio-state.edu/~shen.94/681/Site/Slides_files/basic_algo.pdf
static void camera_setup(Camera *camera) {
    // Setup Camera Coordinate System
    camera->n = vec3_norm(vec3_sub(camera->lookAt, camera->position));
    camera->u = vec3_norm(vec3_cross(camera->up, camera->n));
    camera->v = vec3_cross(camera->n, camera->u);

    // calculate Distance between RenderTarget and Eye
    camera->distanceToRenderTarget = (camera->renderDim.width/(2.0f*tan(angle_deg2rad(camera->hFOV)/2.0f)));

    // calculate CPosition centerPos on RenderTarget
    camera->cPosition = vec3_sub(camera->position, vec3_mul(camera->n, camera->distanceToRenderTarget));

    // calculate LPosition leftPos on RenderTarget
    Vec3 u_times_W_div_2 = vec3_mul(camera->u, camera->renderDim.width/2.0f);
    Vec3 v_times_H_div2 = vec3_mul(camera->v, camera->renderDim.height/2.0f);
    camera->lPosition = vec3_sub(vec3_sub(camera->cPosition, u_times_W_div_2), v_times_H_div2);
}

Camera *camera_create(Vec3 position, Vec3 up, Vec3 lookAt, Dimension renderDim, double hFOV) {
    Camera* camera = malloc(sizeof(camera));
    camera->position = position;
    camera->up = up;
    camera->lookAt = lookAt;
    camera->renderDim = renderDim;
    camera->hFOV = hFOV;
    camera_setup(camera);
    return camera;
}

Ray camera_get_ray_for_pixel(Camera* camera, int x, int y) {
    Vec3 uScaledByXTimesWidth = vec3_mul(camera->u, x * camera->renderDim.width);
    Vec3 vScaledByYTimesHeight = vec3_mul(camera->v, y * camera->renderDim.height);
    Vec3 pixelOnPlane = vec3_add(vec3_add(camera->lPosition, uScaledByXTimesWidth), vScaledByYTimesHeight);
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
