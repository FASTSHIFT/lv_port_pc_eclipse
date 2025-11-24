/**
 * @file rpi_port.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#ifdef LV_USE_RPI_PORT

#include "lvgl/lvgl.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <bcm_host.h>
#include <stdio.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static uint32_t get_tick_ms(void);
static int init_egl(uint32_t width, uint32_t height);

/**********************
 *  STATIC VARIABLES
 **********************/

static EGLDisplay display;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

int rpi_port_init(uint32_t width, uint32_t height)
{
    LV_LOG_USER("width: %" LV_PRIu32 ", height: %" LV_PRIu32, width, height);
    lv_tick_set_cb(get_tick_ms);
    return init_egl(width, height);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static uint32_t get_tick_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + (float)ts.tv_nsec / 1000000;
}

static int init_egl(uint32_t width, uint32_t height)
{
    static EGL_DISPMANX_WINDOW_T nativewindow;
    static EGLSurface surface;
    static EGLContext context;

    bcm_host_init();

    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        LV_LOG_ERROR("Failed to get EGL display: %d", eglGetError());
        return -1;
    }

    LV_LOG_USER("EGL display: %p", display);

    if (!eglInitialize(display, NULL, NULL)) {
        LV_LOG_ERROR("Failed to initialize EGL: %d", eglGetError());
        return -1;
    }

    EGLint attribs[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_STENCIL_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    EGLConfig config;
    EGLint num_configs;
    if (!eglChooseConfig(display, attribs, &config, 1, &num_configs)) {
        LV_LOG_ERROR("Failed to choose EGL config: %d", eglGetError());
        return -1;
    }

    EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attribs);
    if (context == EGL_NO_CONTEXT) {
        LV_LOG_ERROR("Failed to create EGL context: %d", eglGetError());
        return -1;
    }

    LV_LOG_USER("EGL context: %p", context);

    // Create dispmanx window
    VC_RECT_T dst_rect = { 0, 0, width, height };
    VC_RECT_T src_rect = { 0, 0, width << 16, height << 16 };

    DISPMANX_DISPLAY_HANDLE_T dispman_display = vc_dispmanx_display_open(0);
    DISPMANX_UPDATE_HANDLE_T dispman_update = vc_dispmanx_update_start(0);
    DISPMANX_ELEMENT_HANDLE_T dispman_element = vc_dispmanx_element_add(
        dispman_update, dispman_display,
        0, &dst_rect, 0,
        &src_rect, DISPMANX_PROTECTION_NONE,
        0, 0, DISPMANX_NO_ROTATE);

    nativewindow.element = dispman_element;
    nativewindow.width = width;
    nativewindow.height = height;
    vc_dispmanx_update_submit_sync(dispman_update);

    surface = eglCreateWindowSurface(display, config, &nativewindow, NULL);
    if (surface == EGL_NO_SURFACE) {
        LV_LOG_ERROR("Failed to create EGL surface: %d", eglGetError());
        return -1;
    }

    LV_LOG_USER("EGL surface: %p", surface);

    if (!eglMakeCurrent(display, surface, surface, context)) {
        LV_LOG_ERROR("Failed to make EGL context current: %d", eglGetError());
        return -1;
    }

    LV_LOG_USER("EGL initialized successfully");

    return 0;
}

void rpi_port_deinit(void)
{
    eglTerminate(display);
    bcm_host_deinit();
}

#endif /* LV_USE_RPI_PORT */
