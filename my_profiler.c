#include "lvgl/lvgl.h"

#if LV_USE_PROFILER && LV_USE_PROFILER_BUILTIN

#include "lvgl/lvgl_private.h"
#include <sys/syscall.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

static uint64_t my_get_tick_us_cb(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000 + ts.tv_nsec;
}

static int my_get_tid_cb(void)
{
    return (int)syscall(SYS_gettid);
}

static int my_get_cpu_cb(void)
{
    return (int)syscall(SYS_getcpu);
}

void my_profiler_init(void)
{
    lv_profiler_builtin_config_t config;
    lv_profiler_builtin_config_init(&config);
    config.buf_size = 2048 * 1024;
    config.tick_per_sec = 1000000000; /* One second is equal to 1000000000 nanoseconds */
    config.tick_get_cb = my_get_tick_us_cb;
    config.tid_get_cb = my_get_tid_cb;
    config.cpu_get_cb = my_get_cpu_cb;
    lv_profiler_builtin_init(&config);
}

#endif
