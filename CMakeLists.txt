cmake_minimum_required(VERSION 3.11)
project(Raytracer)

set(CMAKE_C_STANDARD
        11)

set(EXECUTABLE_NAME
		raytracer)

# find SDL2
find_package(SDL2 REQUIRED COMPONENTS SDL2::SDL2)
include_directories(${SDL2_INCLUDE_DIRS})


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
     -std=c11 \
     -O3")
endif ()

include_directories(src/)

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
		src/vertextable.c)

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
		src/vertextable.h src/kernel.h)

if(FALSE) # fake a block comment
if (UNIX)
	find_package(OpenMP)
	if (OPENMP_FOUND)
		set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
		set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
		set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
	endif()
endif(UNIX)
endif()


find_package(OpenCL REQUIRED)
include_directories(${OpenCL_INCLUDE_DIRS})
link_directories(${OpenCL_LIBRARY})

add_executable(raytracer ${SOURCE_FILES} ${HEADER_FILES})

if (UNIX)
	target_link_libraries(${EXECUTABLE_NAME} m)
endif (UNIX)

target_link_libraries(${EXECUTABLE_NAME} SDL2::SDL2 ${OpenCL_LIBRARY})