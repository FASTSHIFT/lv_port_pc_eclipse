cmake_minimum_required(VERSION 3.10)
project(Simulator)

# C/C++ version
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

# Platform config Common

option(ENABLE_DEBUG "Enable debug build" OFF)
option(ENABLE_FREETYPE "Enable freetype library" OFF)
option(ENABLE_PROFILER "Enable LVGL profiler" OFF)
option(ENABLE_SYSMON "Enable LVGL system monitor" OFF)
option(ENABLE_RPI_PORT "Enable Raspberry Pi port" OFF)

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

set(EXTERNAL_SOURCE_DIR ${PROJECT_SOURCE_DIR}/external)

if(ENABLE_RPI_PORT)
  message(STATUS "Raspberry Pi port enabled")
  add_definitions(-DLV_USE_RPI_PORT=1 -DLV_USE_SDL=0)

  # Add RPI specific library paths - try system location first
  find_library(BCM_HOST_LIB bcm_host PATHS /opt/vc/lib)
  if(BCM_HOST_LIB)
    message(STATUS "BCM_HOST_LIB found at ${BCM_HOST_LIB}")
    link_directories(/opt/vc/lib)
    set(VC_INCLUDE_DIR /opt/vc/include)
  else()
    message(STATUS "BCM_HOST_LIB not found, using custom location")
    link_directories(${EXTERNAL_SOURCE_DIR}/rpi_port/vc/lib)
    set(VC_INCLUDE_DIR ${EXTERNAL_SOURCE_DIR}/rpi_port/vc/include)

    # Set cross-compilation toolchain using find_program
    find_program(RPI_C_COMPILER arm-linux-gnueabihf-gcc)
    find_program(RPI_CXX_COMPILER arm-linux-gnueabihf-g++)
    if(NOT RPI_C_COMPILER OR NOT RPI_CXX_COMPILER)
      message(
        FATAL_ERROR
          "Could not find RPI cross-compiler tools. Please ensure they are in PATH"
      )
    endif()
    set(CMAKE_C_COMPILER
        ${RPI_C_COMPILER}
        CACHE FILEPATH "C compiler" FORCE)
    set(CMAKE_CXX_COMPILER
        ${RPI_CXX_COMPILER}
        CACHE FILEPATH "C++ compiler" FORCE)
  endif()
endif()

if(ENABLE_PROFILER)
  message(STATUS "LVGL Profiler enabled")
  add_definitions(-DLV_USE_PROFILER=1)
endif()

if(ENABLE_SYSMON)
  message(STATUS "LVGL System Monitor enabled")
  add_definitions(-DLV_USE_SYSMON=1)
endif()

# Warning flags
set(WARNING_FLAGS
    "-Wall -Wextra\
    -Wshadow -Wundef -Wmaybe-uninitialized -Wno-deprecated-declarations \
    -Wno-unused-function -Wno-error=strict-prototypes -Wpointer-arith -fno-strict-aliasing -Wno-error=cpp -Wuninitialized \
    -Wno-unused-parameter -Wno-missing-field-initializers -Wno-format-nonliteral -Wno-cast-qual -Wunreachable-code -Wno-switch-default  \
    -Wreturn-type -Wmultichar -Wformat-security -Wno-ignored-qualifiers -Wno-error=pedantic -Wno-sign-compare -Wno-error=missing-prototypes -Wdouble-promotion -Wclobbered -Wdeprecated  \
    -Wempty-body -Wshift-negative-value \
    -Wtype-limits -Wsizeof-pointer-memaccess -Wpointer-arith -Wno-cast-function-type -Wunused-but-set-variable"
)
set(WARNING_FLAGS
    "${WARNING_FLAGS} -Wno-shadow -Wno-unused-but-set-variable -Wno-double-promotion -Werror=narrowing -Werror=narrowing -Wunused-function"
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARNING_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS}")

# Address Sanitizer
if(ENABLE_ASAN)
  set(ASAN_FLAGS
      "-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -fsanitize=leak"
  )
  message(STATUS "ASAN_FLAGS: ${ASAN_FLAGS}")
  add_definitions(${ASAN_FLAGS})
  set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} ${ASAN_FLAGS})
endif()

# Include
include_directories(${PROJECT_SOURCE_DIR})

# Sources
set(SOURCES ${SOURCES} ${EXTERNAL_SOURCE_DIR}/argparse/argparse.c)

if(ENABLE_FREETYPE)
  # Freetype
  add_definitions(-DLV_USE_FREETYPE=1)
  include_directories(${EXTERNAL_SOURCE_DIR}/freetype/include)
  add_definitions(
    -DHAVE_UNISTD_H=1 -DHAVE_FCNTL_H=1 -DFT2_BUILD_LIBRARY
    -DFT_CONFIG_MODULES_H=<lvgl/src/libs/freetype/ftmodule.h>
    -DFT_CONFIG_OPTIONS_H=<lvgl/src/libs/freetype/ftoption.h>)
  add_subdirectory(${EXTERNAL_SOURCE_DIR}/freetype
                   ${PROJECT_BINARY_DIR}/build_freetype)
endif()

if(ENABLE_RPI_PORT)
  include_directories(${VC_INCLUDE_DIR})

  set(SOURCES ${SOURCES} ${EXTERNAL_SOURCE_DIR}/rpi_port/rpi_port.c)

  # RPI specific libraries
  set(RPI_LIBS brcmEGL brcmGLESv2 m bcm_host vchiq_arm vcos)
else()
  add_definitions(-DLV_USE_OPENGLES=1 -DLV_USE_GLFW=1)

  find_package(SDL2 REQUIRED)
  include_directories(${SDL2_INCLUDE_DIRS})

  find_package(glfw3 REQUIRED)
  set(GLFW_LIBRARIES glfw)

  # OpenGL
  find_package(OpenGL REQUIRED)
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

  find_package(JPEG REQUIRED)
  include_directories(${JPEG_INCLUDE_DIR})

  find_package(PNG REQUIRED)
  include_directories(${PNG_INCLUDE_DIR})
endif()

# Subdirectory
add_subdirectory(${PROJECT_SOURCE_DIR}/lvgl ${PROJECT_BINARY_DIR}/build_lvgl)

# Executable
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
add_executable(Simulator main.c ${SOURCES} ${INCLUDES})

# Link
target_link_libraries(
  Simulator
  PRIVATE stdc++
          lvgl
          lvgl::examples
          lvgl::demos
          lvgl::thorvg
          $<$<BOOL:${ENABLE_FREETYPE}>:freetype::freetype>
          ${RPI_LIBS}
          ${OPENGL_LIBRARIES}
          ${GLEW_LIBRARIES}
          ${GLFW_LIBRARIES}
          ${DRM_LIBRARIES}
          ${GBM_LIBRARIES}
          ${EGL_LIBRARIES}
          ${SDL2_LIBRARIES}
          ${PNG_LIBRARIES})

set_target_properties(Simulator PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                           "${PROJECT_SOURCE_DIR}/build")
