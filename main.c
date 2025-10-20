
/**
 * @file main
 *
 */

/*********************
 *      INCLUDES
 *********************/

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

int main(int argc, char** argv)
{
    /*Initialize LVGL*/
    lv_init();

#if LV_USE_PROFILER
    void my_profiler_init(void);
    my_profiler_init();
#endif

    /*Create a default group for keyboard navigation*/
    lv_group_set_default(lv_group_create());

    /*Initialize the HAL (display, input devices, tick) for LVGL*/
    hal_init(800, 600);

    extern int app_entry(int argc, char** argv);
    int ret = app_entry(argc, argv);
    if (ret < 0) {
        return ret;
    }

    while (1) {
        uint32_t time_till_next = lv_timer_handler();
        if (time_till_next) {
            usleep(time_till_next * 1000);
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
    lv_indev_t* mouse = lv_sdl_mouse_create();
    // lv_display_set_rotation(disp, LV_DISP_ROTATION_90);
    lv_indev_set_group(mouse, lv_group_get_default());
    lv_indev_set_disp(mouse, disp);

#if 0
     LV_IMG_DECLARE(mouse_cursor_icon); /*Declare the image file.*/
     lv_obj_t* cursor_obj;
     cursor_obj = lv_img_create(lv_scr_act()); /*Create an image object for the cursor */
     lv_img_set_src(cursor_obj, &mouse_cursor_icon); /*Set the image source*/
     lv_indev_set_cursor(mouse, cursor_obj); /*Connect the image  object to the driver*/
#endif
    // lv_indev_t* mousewheel = lv_sdl_mousewheel_create();
    // lv_indev_set_disp(mousewheel, disp);
    // lv_indev_set_group(mousewheel, lv_group_get_default());

    lv_indev_t* keyboard = lv_sdl_keyboard_create();
    lv_indev_set_disp(keyboard, disp);
    lv_indev_set_group(keyboard, lv_group_get_default());
#endif

#if LV_USE_LINUX_DRM
    /* Create a DRM display */
    lv_display_t *disp = lv_linux_drm_create();

    const char * device_path = lv_linux_drm_find_device_path();

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

#if LV_USE_DRAW_NANOVG
    lv_display_set_render_mode(disp, LV_DISPLAY_RENDER_MODE_FULL);
    // lv_display_set_matrix_rotation(disp, true);
    // lv_display_set_rotation(disp, LV_DISP_ROTATION_180);
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

    return NULL;
}
