
/**
 * @file main
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "external/argparse/argparse.h"
#include "external/rpi_port/rpi_port.h"
#include "lvgl/demos/lv_demos.h"
#include "lvgl/lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_disp_t* hal_init(int32_t w, int32_t h);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      VARIABLES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

int main(int argc, const char** argv)
{
    /*Initialize LVGL*/
    lv_init();

    int width = 480;
    int height = 480;
    const char* demo_name = "widgets";
    const char* touchdev = NULL;

    struct {
        const char* name;
        void (*func)(void);
    } demo_funcs[] = {
#if LV_USE_DEMO_WIDGETS
        { "widgets", lv_demo_widgets },
#endif
#if LV_USE_DEMO_BENCHMARK
        { "benchmark", lv_demo_benchmark },
#endif
#if LV_USE_DEMO_VECTOR_GRAPHIC
        { "vector_graphic", lv_demo_vector_graphic_buffered },
#endif
        { NULL, NULL }
    };

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_INTEGER(0, "width", &width, "Set display width", NULL, 0, 0),
        OPT_INTEGER(0, "height", &height, "Set display height", NULL, 0, 0),
        OPT_STRING('d', "demo", &demo_name, "Set demo name", NULL, 0, 0),
        OPT_STRING('t', "touch", &touchdev, "Set touch device", NULL, 0, 0),
        OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, NULL, 0);
    if (argparse_parse(&argparse, argc, argv) > 0) {
        LV_LOG_WARN("argparse failed");
        lv_deinit();
        return -1;
    }

    /*Create a default group for keyboard navigation*/
    lv_group_set_default(lv_group_create());

    /*Initialize the HAL (display, input devices, tick) for LVGL*/
    hal_init(width, height);

    if (touchdev) {
        lv_indev_t* indev = lv_evdev_create(LV_INDEV_TYPE_POINTER, touchdev);
        if (indev) {
            lv_indev_set_display(indev, lv_display_get_default());
        } else {
            LV_LOG_WARN("Failed to create evdev input device for %s", touchdev);
        }
    }

    for (int i = 0; i < sizeof(demo_funcs) / sizeof(demo_funcs[0]); i++) {
        if (!demo_funcs[i].name) {
            LV_LOG_WARN("Demo '%s' not found", demo_name);

            for (int j = 0; j < sizeof(demo_funcs) / sizeof(demo_funcs[0]); j++) {
                if (demo_funcs[j].name) {
                    LV_LOG_WARN("  Available demo: %s", demo_funcs[j].name);
                }
            }

            lv_deinit();
            return -1;
        }

        if (lv_strcmp(demo_name, demo_funcs[i].name) == 0) {
            demo_funcs[i].func();
            break;
        }
    }

    while (1) {
        uint32_t time_till_next = lv_timer_handler();

        if (time_till_next == LV_NO_TIMER_READY) {
            time_till_next = LV_DEF_REFR_PERIOD;
        }

        if (time_till_next) {
            if (usleep(time_till_next * 1000) != 0) {
                LV_LOG_ERROR("usleep error");
                break;
            }
        }
    }

    lv_deinit();

    return 0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * Initialize the Hardware Abstraction Layer (HAL) forLVGL
 */
static lv_disp_t* hal_init(int32_t w, int32_t h)
{
#if LV_USE_SDL
    lv_disp_t* disp = lv_sdl_window_create(w, h);
    // lv_indev_t* mouse = lv_sdl_mouse_create();
    // lv_indev_set_group(mouse, lv_group_get_default());
    // lv_indev_set_display(mouse, disp);

    lv_indev_t* keyboard = lv_sdl_keyboard_create();
    lv_indev_set_display(keyboard, disp);
    lv_indev_set_group(keyboard, lv_group_get_default());
#endif

#if LV_USE_LINUX_DRM
    /* Create a DRM display */
    lv_display_t* disp = lv_linux_drm_create();

    const char* device_path = lv_linux_drm_find_device_path();

    /* Set DRM device file and connector */
    /* The 2nd argument is the DRM device path */
    /* The 3rd argument is the connector_id (-1 = auto-select first available) */
    lv_linux_drm_set_file(disp, device_path, -1);
#endif

#if LV_USE_OPENGLES
#if LV_USE_GLFW
    /* create a window and initialize OpenGL */
    lv_opengles_window_t* window = lv_opengles_glfw_window_create(w, h, true);

    /* create a display that flushes to a texture */
    lv_display_t* disp = lv_opengles_window_display_create(window, w, h);
    lv_display_set_default(disp);
#endif

#if LV_USE_DRAW_OPENGLES
    /* add the texture to the window */
    unsigned int texture_id = lv_opengles_texture_get_texture_id(disp);
    lv_opengles_window_texture_t* window_texture = lv_opengles_window_add_texture(window, texture_id, w, h);

    /* get the mouse indev of the window texture */
    lv_indev_t* mouse = lv_opengles_window_texture_get_mouse_indev(window_texture);
    lv_indev_set_group(mouse, lv_group_get_default());
    lv_indev_set_display(mouse, disp);
#endif
#endif

#ifdef LV_USE_RPI_PORT
    rpi_port_init(w, h);
#endif

    return NULL;
}
