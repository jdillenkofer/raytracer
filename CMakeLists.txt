cmake_minimum_required(VERSION 3.11)
project(Raytracer)

set(CMAKE_C_STANDARD
        11)

set(EXECUTABLE_NAME
		raytracer)

# find SDL2
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/modules")
find_package(SDL2 REQUIRED COMPONENTS main)
include_directories(${SDL2_INCLUDE_DIRS} ${SDL2main_INCLUDE_DIRS})

# find OpenGL
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})

if (("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU") OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "CLANG"))
    set(CMAKE_C_FLAGS
     "${CMAKE_C_FLAGS} \
     -Wall \
     -Wextra \
     -Wconversion \
     -Wunused \
     -Wunused-function \
     -Wunused-label \
     -Wunused-macros \
     -Wunused-parameter \
     -Wunused-value \
     -Wunused-variable \
     -Wcast-align \
     -Wcast-qual \
     -Wmissing-declarations \
     -Wredundant-decls \
     -Wmissing-prototypes \
     -Wnested-externs \
     -Wpointer-arith \
     -Wshadow \
     -Wstrict-prototypes \
     -Wwrite-strings \
     -Wswitch \
     -fno-common \
     -pedantic \
     -std=c11")
endif ()

include_directories(src/)
include_directories(vendor/glad/include)

set(SOURCE_FILES
        src/main.c
        src/utils/vec3.c
        src/utils/math.c
		src/utils/random.c
		src/utils/image.c
		src/utils/file.c
		src/camera.c
        src/triangle.c
        src/scene.c
        src/raytracer.c
		src/object.c
		src/vertextable.c
		src/kernel.cl
		vendor/glad/src/glad.c)

set(HEADER_FILES
        src/utils/vec3.h
        src/utils/math.h
		src/utils/random.h
		src/utils/file.h
        src/camera.h
        src/triangle.h
        src/utils/image.h
        src/scene.h
        src/ray.h
        src/raytracer.h
        src/material.h
        src/sphere.h
        src/plane.h
        src/pointlight.h
		src/object.h
		src/vertextable.h 
		src/gpu.h
		vendor/glad/include/glad/glad.h
		vendor/glad/include/KHR/khrplatform.h)

find_package(OpenCL REQUIRED)
include_directories(${OpenCL_INCLUDE_DIRS})
link_directories(${OpenCL_LIBRARY})

add_executable(raytracer ${SOURCE_FILES} ${HEADER_FILES})

if (UNIX)
	target_link_libraries(${EXECUTABLE_NAME} m dl)
endif (UNIX)

target_link_libraries(${EXECUTABLE_NAME} ${SDL2_LIBS} ${OpenCL_LIBRARY} ${OPENGL_LIBRARIES})
add_custom_command(TARGET raytracer POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/src/kernel.cl ${CMAKE_BINARY_DIR}/kernel.cl)
# Copy SDL2 DLLs to output folder on Windows
if(WIN32)
    foreach(DLL ${SDL2_DLLS})
        add_custom_command(TARGET raytracer POST_BUILD COMMAND
            ${CMAKE_COMMAND} -E copy_if_different ${DLL} $<TARGET_FILE_DIR:raytracer>)
    endforeach()
endif()
