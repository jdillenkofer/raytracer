cmake_minimum_required(VERSION 3.11)
project(Raytracer)

set(EXECUTABLE_NAME
        raytracer
        )

set(CMAKE_C_STANDARD
        11)

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
        src/utils/angle.c
        src/camera.c
        src/triangle.c
        src/utils/image.c
        src/scene.c
        src/raytracer.c)

set(HEADER_FILES
        src/utils/vec3.h
        src/utils/angle.h
        src/camera.h
        src/ray.h
        src/triangle.h
        src/utils/image.h
        src/scene.h
        src/material.h
        src/sphere.h
        src/plane.h
        src/raytracer.h
        src/pointlight.h)

if (UNIX)
	link_libraries(m)
endif (UNIX)

if (WIN32)
	find_package(OpenMP)
	if (OPENMP_FOUND)
		set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
		set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
		set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
	endif()
endif(WIN32)

add_executable(${EXECUTABLE_NAME} ${SOURCE_FILES} ${HEADER_FILES})
