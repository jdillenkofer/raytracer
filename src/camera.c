#include "camera.h"

#include <stdlib.h>

Camera *camera_create(Vec3 position, Vec3 up, Vec3 lookAt, double renderWidth, double renderHeight) {
    Camera* camera = malloc(sizeof(camera));
    camera->position = position;
    camera->up = up;
    camera->lookAt = lookAt;
    camera->renderWidth = renderWidth;
    camera->renderHeight = renderHeight;
    camera_calcEyeCoordinateSystem(camera);
    return camera;
}


static void camera_calcEyeCoordinateSystem(Camera* camera) {
    // See: http://web.cse.ohio-state.edu/~shen.94/681/Site/Slides_files/basic_algo.pdf
    camera->n = vec3_norm(vec3_sub(camera->lookAt, camera->position));
    camera->u = vec3_norm(vec3_cross(camera->up, camera->n));
    camera->v = vec3_cross(camera->n, camera->u);
}

void camera_destroy(Camera* cam) {
    if (cam) {
        free(cam);
    }
}
