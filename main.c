
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
#include <string.h>
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
    int enable_profiler = 0;
    int enable_sysmon = 0;

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

#if LV_USE_DEMO_GLTF || LV_USE_DEMO_NANOVG_GLTF

#if LV_USE_DEMO_NANOVG_GLTF
#include "lvgl/demos/gltf/lv_demo_nanovg_3d.h"
#endif

#define GLTF_MAX_PATH_LEN 512

typedef struct {
    char name[256];
    bool is_dir;
} gltf_file_entry_t;

typedef struct {
    lv_obj_t* browser_cont;
    lv_obj_t* path_label;
    lv_obj_t* file_list;
    lv_obj_t* gltf_obj;
    lv_obj_t* back_btn;
    char current_path[GLTF_MAX_PATH_LEN];
} gltf_ctx_t;

static void gltf_browser_update(gltf_ctx_t* ctx);

static void gltf_back_btn_event_cb(lv_event_t* e)
{
    gltf_ctx_t* ctx = lv_event_get_user_data(e);
    if (ctx->gltf_obj) {
        lv_obj_delete(ctx->gltf_obj);
        ctx->gltf_obj = NULL;
    }
    if (ctx->back_btn) {
        lv_obj_delete(ctx->back_btn);
        ctx->back_btn = NULL;
    }
    if (ctx->browser_cont) {
        lv_obj_remove_flag(ctx->browser_cont, LV_OBJ_FLAG_HIDDEN);
    }
}

static void gltf_load_model(gltf_ctx_t* ctx, const char* path)
{
    LV_LOG_USER("Loading GLTF model: %s", path);

    /* Hide browser */
    if (ctx->browser_cont) {
        lv_obj_add_flag(ctx->browser_cont, LV_OBJ_FLAG_HIDDEN);
    }

    /* Load GLTF - use NanoVG 3D if available, otherwise use original */
#if LV_USE_DEMO_NANOVG_GLTF
    ctx->gltf_obj = lv_demo_nanovg_3d(path);
#elif LV_USE_DEMO_GLTF
    ctx->gltf_obj = lv_demo_gltf(path);
#endif

    /* Create back button */
    ctx->back_btn = lv_button_create(lv_screen_active());
    lv_obj_set_size(ctx->back_btn, 80, 40);
    lv_obj_align(ctx->back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_add_event_cb(ctx->back_btn, gltf_back_btn_event_cb, LV_EVENT_CLICKED, ctx);

    lv_obj_t* label = lv_label_create(ctx->back_btn);
    lv_label_set_text(label, LV_SYMBOL_LEFT " Back");
    lv_obj_center(label);
}

static void gltf_list_item_event_cb(lv_event_t* e)
{
    gltf_file_entry_t* entry = lv_obj_get_user_data(lv_event_get_current_target(e));
    gltf_ctx_t* ctx = lv_event_get_user_data(e);
    if (!entry)
        return;

    if (entry->is_dir) {
        /* Navigate into directory */
        size_t cur_len = lv_strlen(ctx->current_path);
        if (cur_len > 0 && ctx->current_path[cur_len - 1] != '/') {
            lv_strcat(ctx->current_path, "/");
        }
        lv_strcat(ctx->current_path, entry->name);
        gltf_browser_update(ctx);
    } else {
        /* Load GLTF file */
        char full_path[GLTF_MAX_PATH_LEN];
        lv_snprintf(full_path, sizeof(full_path), "%s/%s", ctx->current_path, entry->name);
        gltf_load_model(ctx, full_path);
    }
}

static void gltf_parent_dir_event_cb(lv_event_t* e)
{
    gltf_ctx_t* ctx = lv_event_get_user_data(e);
    /* Go to parent directory */
    /* Path format: A:/xxx/yyy */
    char* path_start = ctx->current_path + 2; /* Skip "A:" */
    char* last_slash = strrchr(path_start, '/');

    if (last_slash && last_slash != path_start) {
        /* Not at root, go up one level */
        *last_slash = '\0';
    } else {
        /* At root or one level deep, go to root */
        lv_strlcpy(ctx->current_path, "A:.", sizeof(ctx->current_path));
    }
    gltf_browser_update(ctx);
}

static const char* get_file_icon(const char* filename, bool is_dir)
{
    if (is_dir) {
        return LV_SYMBOL_DIRECTORY;
    }

    /* Get extension using LVGL API */
    const char* ext = lv_fs_get_ext(filename);
    if (ext && (lv_strcmp(ext, "gltf") == 0 || lv_strcmp(ext, "glb") == 0)) {
        return LV_SYMBOL_IMAGE;
    }
    return LV_SYMBOL_FILE;
}

static void gltf_browser_update(gltf_ctx_t* ctx)
{
    lv_fs_dir_t dir;
    lv_fs_res_t res;
    char fn[256];

    /* Update path label - show path without 'A:' prefix */
    lv_label_set_text(ctx->path_label, ctx->current_path);

    /* Clear existing list items (skip the first item which is "..") */
    uint32_t child_cnt = lv_obj_get_child_count(ctx->file_list);
    for (int i = child_cnt - 1; i >= 1; i--) {
        lv_obj_t* child = lv_obj_get_child(ctx->file_list, i);
        lv_obj_delete(child);
    }

    /* Open directory using LVGL FS API */
    res = lv_fs_dir_open(&dir, ctx->current_path);
    if (res != LV_FS_RES_OK) {
        LV_LOG_ERROR("Failed to open directory: %s (res=%d)", ctx->current_path, res);
        lv_list_add_text(ctx->file_list, "Failed to open directory");
        return;
    }

    static gltf_file_entry_t entries[256];
    int file_count = 0;

    /* Read directory entries */
    while (file_count < sizeof(entries) / sizeof(entries[0])) {
        res = lv_fs_dir_read(&dir, fn, sizeof(fn));
        if (res != LV_FS_RES_OK || fn[0] == '\0') {
            break;
        }

        /* Check if it's a directory (first char is '/') */
        bool is_dir = (fn[0] == '/');
        const char* name = is_dir ? (fn + 1) : fn; /* Skip leading '/' for directories */

        /* Skip . and .. */
        if (lv_strcmp(name, ".") == 0 || lv_strcmp(name, "..") == 0) {
            continue;
        }

        lv_strlcpy(entries[file_count].name, name, sizeof(entries[file_count].name));
        entries[file_count].is_dir = is_dir;

        const char* icon = get_file_icon(entries[file_count].name, is_dir);
        lv_obj_t* btn = lv_list_add_button(ctx->file_list, icon, entries[file_count].name);
        lv_obj_set_user_data(btn, &entries[file_count]);
        lv_obj_add_event_cb(btn, gltf_list_item_event_cb, LV_EVENT_CLICKED, ctx);

        file_count++;
    }

    lv_fs_dir_close(&dir);

    if (file_count == 0) {
        lv_list_add_text(ctx->file_list, "Empty directory");
    }
}

static void demo_gltf(void)
{
    static gltf_ctx_t gltf_ctx;
    lv_memzero(&gltf_ctx, sizeof(gltf_ctx));
    gltf_ctx_t* ctx = &gltf_ctx;

    /* Start from root */
    lv_strlcpy(ctx->current_path, "A:gltfs", sizeof(ctx->current_path));

    /* Create browser container */
    ctx->browser_cont = lv_obj_create(lv_screen_active());
    lv_obj_set_size(ctx->browser_cont, LV_PCT(90), LV_PCT(90));
    lv_obj_center(ctx->browser_cont);
    lv_obj_set_flex_flow(ctx->browser_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(ctx->browser_cont, 10, 0);
    lv_obj_set_style_pad_gap(ctx->browser_cont, 5, 0);

    /* Title */
    lv_obj_t* title = lv_label_create(ctx->browser_cont);
    lv_label_set_text(title, "GLTF File Browser");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);

    /* Current path label */
    ctx->path_label = lv_label_create(ctx->browser_cont);
    lv_label_set_long_mode(ctx->path_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(ctx->path_label, LV_PCT(100));
    lv_obj_set_style_text_color(ctx->path_label, lv_palette_main(LV_PALETTE_BLUE), 0);

    /* File list */
    ctx->file_list = lv_list_create(ctx->browser_cont);
    lv_obj_set_width(ctx->file_list, LV_PCT(100));
    lv_obj_set_flex_grow(ctx->file_list, 1);

    /* Add parent directory button */
    lv_obj_t* parent_btn = lv_list_add_button(ctx->file_list, LV_SYMBOL_UP, "..");
    lv_obj_add_event_cb(parent_btn, gltf_parent_dir_event_cb, LV_EVENT_CLICKED, &gltf_ctx);

    /* Populate file list */
    gltf_browser_update(&gltf_ctx);
}
#endif

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
#if LV_USE_DEMO_GLTF || LV_USE_DEMO_NANOVG_GLTF
        { "gltf", demo_gltf },
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
