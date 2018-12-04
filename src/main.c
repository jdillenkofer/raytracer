#include <float.h>
#include "utils/image.h"
#include "camera.h"
#include "scene.h"

Vec3 raycast(Scene* scene, Ray* ray) {
    Vec3 outColor = scene->materials[0].Color;

    double epsilon = 0.0001f;
    double hitDistance = DBL_MAX;

    for (uint32_t i = 0; i < scene->planeCount; i++) {
        Plane plane = scene->planes[i];

        double denom = vec3_dot(plane.normal, ray->direction);
        if ((denom < -epsilon) > (denom > epsilon)) {
            double t = (-plane.distanceFromOrigin - vec3_dot(plane.normal, ray->origin)) / denom;
            if ((t > 0) && (t < hitDistance)) {
                hitDistance = t;
                outColor = scene->materials[plane.materialIndex].Color;
            }
        }
    }

    for (uint32_t i = 0; i < scene->sphereCount; i++) {
        Sphere sphere = scene->spheres[i];

        Vec3 SphereRelativeOrigin = vec3_sub(ray->origin, sphere.position);
        double a = vec3_dot(ray->direction, ray->direction);
        double b = 2.0f * vec3_dot(ray->direction, SphereRelativeOrigin);
        double c = vec3_dot(SphereRelativeOrigin, SphereRelativeOrigin) - sphere.radius * sphere.radius;

        double denom = 2.0f * a;
        double rootTerm = sqrt(b*b - 4.0f * a * c);

        if (rootTerm > epsilon) {
            double tpos = (-b + rootTerm) / denom;
            double tneg = (-b - rootTerm) / denom;

            double t = tpos;
            // only hits in front of us, and if it is closer to us
            if ((tneg > 0) && (tneg < tpos)) {
                t = tneg;
            }
            if ((t > 0) && (t < hitDistance)) {
                    hitDistance = t;
                    outColor = scene->materials[sphere.materialIndex].Color;
            }
        }
    }

    return outColor;
}

int main(int argc, char* argv[]) {
    (void) argc;
    (void) argv;
    Vec3 camera_pos = { 0.0f , -10.0f, 1.0f };
    Vec3 camera_up = (Vec3) {0.0f, 0.0f, -1.0f};
    Vec3 lookAt = (Vec3) {0.0f, 0.0f, 0.0f};
    Dimension renderDim = { 1920, 1080 };
    double hFOV = 110.0f;
    Camera* camera = camera_create(camera_pos, camera_up, lookAt, renderDim, hFOV);
    Image* image = image_create(renderDim.width, renderDim.height);

    Material materials[3] = {0};
    materials[0].Color = (Vec3) { 0.0f, 0.0f, 0.0f };
    materials[1].Color = (Vec3) { 1.0f, 0.0f, 0.0f };
    materials[2].Color = (Vec3) { 0.0f, 0.0f, 1.0f };

    Plane plane = {0};
    plane.materialIndex = 1;
    plane.normal = (Vec3) { 0.0f, 0.0f, 1.0f };
    plane.distanceFromOrigin = 0;

    Sphere sphere = {0};
    sphere.materialIndex = 2;
    sphere.position = (Vec3) { 0.0f, 0.0f, 0.0f };
    sphere.radius = 1;

    Scene scene = {0};
    scene.materialCount = 3;
    scene.materials = materials;
    scene.planeCount = 1;
    scene.planes = &plane;
    scene.sphereCount = 1;
    scene.spheres = &sphere;
    scene.triangleCount = 0;
    scene.triangles = NULL;

    for (uint32_t y = 0; y < renderDim.height; y++) {
        for (uint32_t x = 0; x < renderDim.width; x++) {
            Ray ray = camera_ray_from_pixel(camera, x, y);
            Vec3 color = raycast(&scene, &ray);
            // TODO: clamp values to [0,1]
            image->buffer[y*renderDim.width + x] =
                    (0xFFu << 24)                     |
                    ((uint32_t) (color.r * 255) << 16)|
                    ((uint32_t) (color.g * 255) << 8) |
                    ((uint32_t) (color.b * 255) << 0);
        }
    }

    bitmap_save_image("test.bmp", image);

    image_destroy(image);
    camera_destroy(camera);
    return 0;
}
