cmake_minimum_required(VERSION 3.10)
project(Simulator)

# C/C++ version
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

# Platform config
## Common

option(ENABLE_DEBUG "Enable debug build" ON)

set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

if(ENABLE_DEBUG)
  message(STATUS "Build type: Debug")
  add_definitions(-DDEBUG)
  add_definitions(-DLV_USE_ASSERT=1)
  set(CMAKE_BUILD_TYPE "Debug")
  set(ENABLE_ASAN 1)
else()
  message(STATUS "Build type: Release")
  set(CMAKE_BUILD_TYPE "Release")
endif()

## Others
if (WIN32)
    message(STATUS "Platform: windows")
    add_definitions(-DUSE_WIN32DRV=1)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Simulator)
elseif (UNIX)
    message(STATUS "Platform: UNIX")

set(WARNING_FLAGS
    "-Wall -Wextra\
    -Wshadow -Wundef -Wmaybe-uninitialized -Wno-deprecated-declarations \
    -Wno-unused-function -Wno-error=strict-prototypes -Wpointer-arith -fno-strict-aliasing -Wno-error=cpp -Wuninitialized \
    -Wno-unused-parameter -Wno-missing-field-initializers -Wno-format-nonliteral -Wno-cast-qual -Wunreachable-code -Wno-switch-default  \
    -Wreturn-type -Wmultichar -Wformat-security -Wno-ignored-qualifiers -Wno-error=pedantic -Wno-sign-compare -Wno-error=missing-prototypes -Wdouble-promotion -Wclobbered -Wdeprecated  \
    -Wempty-body -Wshift-negative-value \
    -Wtype-limits -Wsizeof-pointer-memaccess -Wpointer-arith -Wno-cast-function-type -Wunused-but-set-variable")

set(WARNING_FLAGS "${WARNING_FLAGS} -Wno-shadow -Wno-unused-but-set-variable -Wno-double-promotion -Werror=narrowing -Werror=narrowing -Wunused-function")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARNING_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS}")

# 32-bit build
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")

    find_package(SDL2 REQUIRED SDL2)
    include_directories(
            ${SDL2_INCLUDE_DIRS}
        #     ${SDL2_INCLUDE_DIRS}/../
        #     ${PROJECT_SOURCE_DIR}
    )

    # Address Sanitizer
    if (ENABLE_ASAN)
        set(ASAN_FLAGS "-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -fsanitize=leak")
        message(STATUS "ASAN_FLAGS: ${ASAN_FLAGS}")
        add_definitions(${ASAN_FLAGS})
        set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} ${ASAN_FLAGS})
    endif ()

endif ()

set(EXTERNAL_SOURCE_DIR ${PROJECT_SOURCE_DIR}/external)

# Include
include_directories(
        ${PROJECT_SOURCE_DIR}
)

# Sources
# file(GLOB EXAMPLE_SOURCES
#         ${EXTERNAL_SOURCE_DIR}/example/*.c
#         )

## Freetype
add_definitions(-DHAVE_UNISTD_H=1 -DHAVE_FCNTL_H=1)
include_directories(${EXTERNAL_SOURCE_DIR}/freetype/include)
add_definitions(
        -DFT2_BUILD_LIBRARY
        -DFT_CONFIG_MODULES_H=<lvgl/src/libs/freetype/ftmodule.h>
        -DFT_CONFIG_OPTIONS_H=<lvgl/src/libs/freetype/ftoption.h>
)
add_subdirectory(${EXTERNAL_SOURCE_DIR}/freetype ${PROJECT_BINARY_DIR}/build_freetype)

## libyuv
# include_directories(${EXTERNAL_SOURCE_DIR}/libyuv/include)
# add_subdirectory(${EXTERNAL_SOURCE_DIR}/libyuv ${PROJECT_BINARY_DIR}/build_libyuv)

## ffmpeg
# find_package(PkgConfig REQUIRED)
# pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libswscale libavutil)

# include(FetchContent)
# # Set policy to allow to run the target_link_libraries cmd on targets that are
# # build in another directory. Currently, the linking is not handled by
# cmake_policy(SET CMP0079 NEW)
# # Fetch fastgltf library
# FetchContent_Declare(
#   fastgltf
#   GIT_REPOSITORY https://github.com/spnda/fastgltf
#   GIT_TAG 4e2261350888bae7c35a1f39991f6233d57795f5)
# set(FASTGLTF_ENABLE_DEPRECATED_EXT
#     ON
#     CACHE BOOL "" FORCE)
# set(FASTGLTF_DIFFUSE_TRANSMISSION_SUPPORT
#     ON
#     CACHE BOOL "" FORCE)
# FetchContent_MakeAvailable(fastgltf)

# # Fetch libwebp library
# FetchContent_Declare(
#   webp
#   GIT_REPOSITORY https://github.com/webmproject/libwebp
#   GIT_TAG fa6f56496a442eed59b103250021e4b14ebf1427)
# FetchContent_MakeAvailable(webp)
# include_directories(${PROJECT_SOURCE_DIR}/build/_deps/fastgltf-src/include)
# include_directories(${PROJECT_SOURCE_DIR}/build/_deps/webp-src/src)

set(SOURCES
        # ${EXAMPLE_SOURCES}
        )

# Packages
find_package(JPEG REQUIRED)
include_directories(${JPEG_INCLUDE_DIR})

find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIR})

# OpenGL
find_package(OpenGL REQUIRED)
# find_package(GLUT REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)

# DRM
find_package(PkgConfig REQUIRED)
pkg_check_modules(DRM REQUIRED libdrm)
include_directories(${DRM_INCLUDE_DIRS})

# GBM
find_package(PkgConfig REQUIRED)
pkg_check_modules(GBM REQUIRED gbm)
include_directories(${GBM_INCLUDE_DIRS})

# EGL
find_package(PkgConfig REQUIRED)
pkg_check_modules(EGL REQUIRED egl)
include_directories(${EGL_INCLUDE_DIRS})

# fastgltf
# include_directories(${EXTERNAL_SOURCE_DIR}/fastgltf/include)
# add_subdirectory(${EXTERNAL_SOURCE_DIR}/fastgltf ${PROJECT_BINARY_DIR}/build_fastgltf)

# Subdirectory
add_subdirectory(${PROJECT_SOURCE_DIR}/lvgl ${PROJECT_BINARY_DIR}/build_lvgl)
# add_subdirectory(${EXTERNAL_SOURCE_DIR}/rlottie ${PROJECT_BINARY_DIR}/build_rlottie)

# Executable
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
add_executable(Simulator main.c app_entry.cpp my_profiler.c ${SOURCES} ${INCLUDES})

# Link
target_link_libraries(
        Simulator PRIVATE
        stdc++
        lvgl
        lvgl::examples
        lvgl::demos
        lvgl::thorvg
        freetype
        m
        ${OPENGL_LIBRARIES}
        ${GLEW_LIBRARIES}
        ${DRM_LIBRARIES}
        ${GBM_LIBRARIES}
        ${EGL_LIBRARIES}
        glfw
        ${SDL2_LIBRARIES}
        ${PNG_LIBRARIES}
        ${JPEG_LIBRARIES}
        ${FFMPEG_LIBRARIES}
)

set_target_properties(
        Simulator
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build"
)
