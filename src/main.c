#include <float.h>
#include <time.h>

#include <glad/glad.h>
#include <SDL2/SDL.h>

#include "utils/image.h"
#include "utils/vec3.h"
#include "camera.h"
#include "scene.h"
#include "raytracer.h"
#include "gpu.h"

#include "utils/random.h"
#include "utils/math.h"

#define MS_PER_UPDATE 1000.0 / 120.0

// degrees per tick
#define CAMERA_ROTATION_SPEED 0.25f;

#define RENDER_WIDTH  1920
#define RENDER_HEIGHT 1080

uint32_t viewWidth = RENDER_WIDTH;
uint32_t viewHeight = RENDER_HEIGHT;

uint32_t raysPerPixel = 1;

int main(int argc, char* argv[]) {
    (void) argc;
    (void) argv;

    if (SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE); //OpenGL core profile
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

    SDL_Window* window = SDL_CreateWindow("Raytracer",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, RENDER_WIDTH, RENDER_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Unable to create window: %s", SDL_GetError());
        return 2;
    }

	SDL_GLContext glContext = SDL_GL_CreateContext(window);
	// try to set adaptive swap interval
	if (SDL_GL_SetSwapInterval(-1) != 0) {
		// if this is not supported, try to enable vsync
		SDL_GL_SetSwapInterval(1);
	}

	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
		SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Failed to initialize GLAD.");
		return 2;
	}
	
	Scene* scene = scene_init(RENDER_WIDTH, RENDER_HEIGHT);
	Image* image = image_create(RENDER_WIDTH, RENDER_HEIGHT);

	GPUContext* context = gpu_initContext(scene, raysPerPixel);

    // wait for quit event before quitting
    bool running = true;
	bool takeScreenshot = false;

	uint32_t previousTime = SDL_GetTicks();
	double delta = 0.0;

	float deg = 0.0f;

    while(running) {

		uint32_t currentTime = SDL_GetTicks();
		uint32_t elapsed = currentTime - previousTime;
		previousTime = currentTime;
		delta += elapsed;

		// input handling
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
				case SDL_WINDOWEVENT:
					switch (event.window.event) {
					case SDL_WINDOWEVENT_SIZE_CHANGED:
					case SDL_WINDOWEVENT_RESIZED:
						// onResize: recalculate the scaling
						viewWidth = event.window.data1;
						viewHeight = event.window.data2;
						gpu_updateDimensions(context, viewWidth, viewHeight, RENDER_WIDTH, RENDER_HEIGHT);
						break;
					default:
						break;
					}
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym) {
					case SDLK_w:
						break;
					case SDLK_s:
						break;
					case SDLK_a:
						break;
					case SDLK_d:
						break;
					case SDLK_PRINTSCREEN:
						takeScreenshot = true;
						break;
					case SDLK_ESCAPE:
						running = false;
						break;
					default:
						break;
					}
					break;
				case SDL_KEYUP:
					break;
                default:
                    break;
            }
        }

		// update
		while (delta >= MS_PER_UPDATE) {
			// move the camera in a circle around the origin
			float radius = 40.0f;
			scene->camera->position.x = radius * cosf(math_deg2rad(deg));
			scene->camera->position.z = radius * sinf(math_deg2rad(deg));
			camera_setup(scene->camera);
			deg += CAMERA_ROTATION_SPEED;
			if (deg >= 360.0f) {
				deg = 0.0f;
			}
			
			delta -= MS_PER_UPDATE;
		}

		// render
		if (takeScreenshot) {
			// render to the backbuffer and copy the clImage to the image struct
			gpu_renderScene(context, scene, image);

			char filename[255];
			time_t now = time(NULL);
			snprintf(filename, sizeof(filename), "%d_raytracer.bmp", (int) now);
			
			bitmap_save_image(filename, image);
			takeScreenshot = false;
		} else {
			// just render to the backbuffer
			gpu_renderScene(context, scene, NULL);
		}
		SDL_GL_SwapWindow(window);
    }

    gpu_destroyContext(context);
	
	image_destroy(image);
	scene_destroy(scene);

	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(window);
	SDL_Quit();
    return 0;
}
