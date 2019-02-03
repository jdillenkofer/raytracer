# Raytracer

## How to build

Dev-Dependencies:
- Cmake
- Visual Studio 2017 (Windows)
- gcc (Linux)
- make (Linux)

Dependencies:
- OpenCL
- GLAD (source already included)
- SDL2

### Windows
- Download cmake here: https://cmake.org/download/ and install it.
- Install the OpenCL runtime for your graphics-card
- Download the SDL2 2.0.8 version from here: https://www.libsdl.org/release/SDL2-devel-2.0.8-VC.zip
    - The newest version (2.0.9) has a bug that makes it unusable for our purpose
- Copy all header files in the SDL2 include folder into a new folder include/SDL2/
- Set the OpenCL_INCLUDE_DIR and OpenCL_LIBRARY Variables in cmake correctly.
- Set the SDL2_DIR correctly to the root folder of SDL2, you previously downloaded.
- Generate the VS Solution and open it with Visual Studio 2017.
- Select raytracer as the start project and run it.

### Linux
- Download cmake, OpenCL and SDL2 with your distro's package manager.
- Create a build folder under the project root dir.
- Switch to the build folder directory and run cmake ..
- This will generate a make file for you.
- Run make to generate the binary "raytracer".

## How to run

- To change the scene edit the scene.c file in the src folder and recompile the project.

### Windows
- Run the binary from visual studio by clicking run.
- If you want to run the executable outside of visual studio, make sure that the sourcecode of the OpenCL kernel and the SDL2.dll is in the current working directory of the binary.

### Linux
- On Linux the binary should already be in the working directory of the executable.