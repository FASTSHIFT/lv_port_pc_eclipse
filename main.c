
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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    const char* name;
    const char* fbdev;
    const char* evdev;
    int width;
    int height;
    int rotation;
} hal_cfg_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void sigint_handler(int sig);
static bool hal_init(const hal_cfg_t* cfg);
static bool demo_create(const char* demo_name);

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
    signal(SIGINT, sigint_handler);

    /*Initialize LVGL*/
    lv_init();

    hal_cfg_t hal_cfg = { 0 };
    hal_cfg.width = 640;
    hal_cfg.height = 480;

    const char* demo_name = "widgets";
    bool enable_profiler = false;
    bool enable_sysmon = false;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_INTEGER(0, "width", &hal_cfg.width, "Set display width", NULL, 0, 0),
        OPT_INTEGER(0, "height", &hal_cfg.height, "Set display height", NULL, 0, 0),
        OPT_INTEGER('r', "rotation", &hal_cfg.rotation, "Set display rotation", NULL, 0, 0),
        OPT_STRING('d', "demo", &demo_name, "Set demo name", NULL, 0, 0),
        OPT_STRING(0, "hal", &hal_cfg.name, "Set HAL name", NULL, 0, 0),
        OPT_STRING(0, "fbdev", &hal_cfg.fbdev, "Set framebuffer device", NULL, 0, 0),
        OPT_STRING(0, "evdev", &hal_cfg.evdev, "Set evdev device", NULL, 0, 0),
        OPT_BOOLEAN(0, "profiler", &enable_profiler, "Enable profiler", NULL, 0, 0),
        OPT_BOOLEAN(0, "sysmon", &enable_sysmon, "Enable system monitor", NULL, 0, 0),
        OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, NULL, 0);
    if (argparse_parse(&argparse, argc, argv) > 0) {
        LV_LOG_WARN("argparse failed");
        lv_deinit();
        return -1;
    }

#if LV_USE_PROFILER
    lv_profiler_builtin_set_enable(enable_profiler);
#endif

    if (!hal_init(&hal_cfg)) {
        lv_deinit();
        return -1;
    }

#if LV_USE_SYSMON
    enable_sysmon ? lv_sysmon_show_performance(NULL) : lv_sysmon_hide_performance(NULL);
#endif

    if (!demo_create(demo_name)) {
        lv_deinit();
        return -1;
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

static void sigint_handler(int sig)
{
    LV_UNUSED(sig);
    /* Trigger gprof data write */
    exit(0);
}

#if LV_USE_SDL
static bool hal_sdl_init(const hal_cfg_t* cfg)
{
    lv_disp_t* disp = lv_sdl_window_create(cfg->width, cfg->height);
    if (!disp) {
        return false;
    }

    lv_indev_t* mouse = lv_sdl_mouse_create();
    lv_indev_set_group(mouse, lv_group_get_default());
    lv_indev_set_display(mouse, disp);

    lv_indev_t* keyboard = lv_sdl_keyboard_create();
    lv_indev_set_display(keyboard, disp);
    lv_indev_set_group(keyboard, lv_group_get_default());
    return true;
}
#endif

#if LV_USE_LINUX_DRM
static bool hal_drm_init(const hal_cfg_t* cfg)
{
    /* Create a DRM display */
    lv_display_t* disp = lv_linux_drm_create();

    const char* device_path = lv_linux_drm_find_device_path();

    if (!disp || !device_path) {
        return false;
    }

    /* Set DRM device file and connector */
    /* The 2nd argument is the DRM device path */
    /* The 3rd argument is the connector_id (-1 = auto-select first available) */
    lv_linux_drm_set_file(disp, device_path, -1);
    return true;
}
#endif

#if LV_USE_GLFW
static bool hal_glfw_init(const hal_cfg_t* cfg)
{
#if LV_USE_GLFW
    /* create a window and initialize OpenGL */
    lv_opengles_window_t* window = lv_opengles_glfw_window_create(cfg->width, cfg->height, true);

    /* create a display that flushes to a texture */
    lv_display_t* disp = lv_opengles_window_display_create(window, cfg->width, cfg->height);
    lv_display_set_default(disp);
#endif

#if LV_USE_DRAW_OPENGLES
    /* add the texture to the window */
    unsigned int texture_id = lv_opengles_texture_get_texture_id(disp);
    lv_opengles_window_texture_t* window_texture = lv_opengles_window_add_texture(window, texture_id, cfg->width, cfg->height);

    /* get the mouse indev of the window texture */
    lv_indev_t* mouse = lv_opengles_window_texture_get_mouse_indev(window_texture);
    lv_indev_set_group(mouse, lv_group_get_default());
    lv_indev_set_display(mouse, disp);
#endif
    return true;
}
#endif

#ifdef LV_USE_RPI_PORT
static bool hal_rpi_port_init(const hal_cfg_t* cfg)
{
    return rpi_port_init(cfg->width, cfg->height) == 0;
}
#endif

static bool hal_dummy_init(const hal_cfg_t* cfg)
{
    LV_LOG_WARN("No HAL selected, using dummy display");
    return true;
}

static bool hal_init(const hal_cfg_t* cfg)
{
    /*Create a default group for keyboard navigation*/
    lv_group_set_default(lv_group_create());

    struct {
        const char* name;
        bool (*func)(const hal_cfg_t* cfg);
    } hal_init_funcs[] = {
#ifdef LV_USE_RPI_PORT
        { "rpi", hal_rpi_port_init },
#endif
#if LV_USE_SDL
        { "sdl", hal_sdl_init },
#endif
#if LV_USE_LINUX_DRM
        { "drm", hal_drm_init },
#endif
#if LV_USE_GLFW
        { "glfw", hal_glfw_init },
#endif
        { NULL, hal_dummy_init }
    };

    if (cfg->fbdev) {
        lv_display_t* disp = lv_linux_fbdev_create();
        lv_linux_fbdev_set_file(disp, cfg->fbdev);
    } else {
        if (!cfg->name) {
            hal_init_funcs[0].func(cfg);
        } else {
            for (int i = 0; i < sizeof(hal_init_funcs) / sizeof(hal_init_funcs[0]); i++) {
                if (lv_strcmp(cfg->name, hal_init_funcs[i].name) == 0) {
                    hal_init_funcs[i].func(cfg);
                    break;
                }
            }
        }
    }

    if (cfg->rotation) {
        lv_display_set_matrix_rotation(NULL, true);
        lv_display_set_rotation(NULL, (lv_display_rotation_t)(cfg->rotation % 4));
    }

    if (cfg->evdev) {
        lv_indev_t* indev = lv_evdev_create(LV_INDEV_TYPE_POINTER, cfg->evdev);
        if (indev) {
            lv_indev_set_display(indev, lv_display_get_default());
        } else {
            LV_LOG_WARN("Failed to create evdev input device for %s", cfg->evdev);
        }
    }

    return true;
}

static bool demo_create(const char* demo_name)
{
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
        { "vector_graphic", lv_demo_vector_graphic_not_buffered },
#endif
        { NULL, NULL }
    };

    for (int i = 0; i < sizeof(demo_funcs) / sizeof(demo_funcs[0]); i++) {
        if (!demo_funcs[i].name) {
            LV_LOG_WARN("Demo '%s' not found", demo_name);

            for (int j = 0; j < sizeof(demo_funcs) / sizeof(demo_funcs[0]); j++) {
                if (demo_funcs[j].name) {
                    LV_LOG_WARN("  Available demo: %s", demo_funcs[j].name);
                }
            }

            return false;
        }

        if (lv_strcmp(demo_name, demo_funcs[i].name) == 0) {
            demo_funcs[i].func();
            break;
        }
    }

    return true;
}
