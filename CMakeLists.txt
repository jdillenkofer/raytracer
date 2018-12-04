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
        src/util/vec3.c
        src/util/color.c
        src/util/angle.c 
        src/camera.c
        src/ray.c
        src/triangle.c
        src/util/image.c)

set(HEADER_FILES
        src/util/vec3.h
        src/util/color.h
        src/util/dimension.h
        src/util/angle.h
        src/camera.h
        src/ray.h
        src/triangle.h
        src/util/image.h)

link_libraries(m)

add_executable(${EXECUTABLE_NAME} ${SOURCE_FILES} ${HEADER_FILES})
