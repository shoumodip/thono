#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <limits.h>
#include <sys/stat.h>

#include "app.h"
#include "config.h"
#include "stb_image.h"
#include "stb_image_resize2.h"

#define return_defer(value)                                                                        \
    do {                                                                                           \
        result = (value);                                                                          \
        goto defer;                                                                                \
    } while (0)

static void usage(FILE *f) {
    fprintf(f, "Usage:\n");
    fprintf(f, "    thono [FLAG] [IMAGES]...\n\n");
    fprintf(f, "Flags:\n");
    fprintf(f, "    -h          Show this help message\n");
    fprintf(f, "    -w <image>  Set the wallpaper\n");
    fprintf(f, "    -W <image>  Set the wallpaper and create a restore script\n");
    fprintf(f, "    -s [delay]  Take a screenshot and exit, with optional delay\n\n");
    fprintf(f, "Image:\n");
    fprintf(f, "    Thono can be used as an image viewer if an image path is provided\n");
}

static int wallpaper(App *a, const char *path) {
    int result = 0;
    uint8_t *image = NULL;

    int w, h;
    uint8_t *src = stbi_load(path, &w, &h, NULL, sizeof(uint32_t));
    if (!src) {
        fprintf(stderr, "ERROR: Could not load image '%s'\n", path);
        return_defer(1);
    }
    app_init(a);

    size_t width = a->size.x;
    size_t height = a->size.y;
    image = stbir_resize_uint8_linear(
        src, w, h, w * sizeof(uint32_t), NULL, width, height, width * sizeof(uint32_t), STBIR_RGBA);

    stbi_image_free(src);
    if (!image) {
        fprintf(stderr, "ERROR: Failed to resize image to %zux%zu\n", width, height);
        return_defer(1);
    }

    a->wallpaper = XCreateImage(
        a->display,
        DefaultVisual(a->display, DefaultScreen(a->display)),
        DefaultDepth(a->display, DefaultScreen(a->display)),
        ZPixmap,
        0,
        NULL,
        width,
        height,
        32,
        0);

    if (!a->wallpaper) {
        fprintf(stderr, "ERROR: Could not create XImage\n");
        return_defer(1);
    }

    a->wallpaper->data = malloc(a->wallpaper->bytes_per_line * height);
    if (!a->wallpaper->data) {
        fprintf(stderr, "ERROR: Could not create XImage\n");
        return_defer(1);
    }

    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            const uint8_t r = image[(y * width + x) * sizeof(uint32_t) + 0];
            const uint8_t g = image[(y * width + x) * sizeof(uint32_t) + 1];
            const uint8_t b = image[(y * width + x) * sizeof(uint32_t) + 2];
            const uint32_t pixel = ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff);
            XPutPixel(a->wallpaper, x, y, pixel);
        }
    }

    app_wallpaper(a);

defer:
    if (image) stbi_image_free(image);
    if (a->display) XCloseDisplay(a->display);
    if (a->wallpaper) XDestroyImage(a->wallpaper);
    return result;
}

static int wallpaper_restore(App *a, const char *path, const char *program) {
    int result = 0;
    DynamicArray(char) restore = {0};

    result = wallpaper(a, path);
    if (result) return_defer(result);

    const char *env_restore_path = getenv("THONO_WALLPAPER_RESTORE_PATH");
    if (env_restore_path) {
        da_append_cstr(&restore, env_restore_path);
        da_append(&restore, '\0');
    } else {
        const char *env_home = getenv("HOME");
        if (!env_home) return_defer(result);

        da_append_cstr(&restore, env_home);
        da_append(&restore, '/');
        da_append_cstr(&restore, THONO_WALLPAPER_RESTORE_PATH);
        da_append(&restore, '\0');
    }

    const size_t cwd = restore.count;

    da_append_many(&restore, NULL, DA_INIT_CAP);
    while (!getcwd(restore.data + cwd, restore.capacity - cwd)) {
        if (errno != ERANGE) {
            fprintf(
                stderr, "ERROR: Could not create wallpaper restore script '%s'\n", restore.data);
            return_defer(1);
        }

        restore.count = restore.capacity;
        da_append_many(&restore, NULL, DA_INIT_CAP);
    }
    restore.count = cwd + strlen(restore.data + cwd);

    FILE *f = fopen(restore.data, "w");
    if (!f) {
        fprintf(stderr, "ERROR: Could not create wallpaper restore script '%s'\n", restore.data);
        return_defer(result);
    }

    fprintf(
        f,
        "#!/bin/sh\n"
        "%s/%s -w %s\n",
        restore.data + cwd,
        program,
        path);

    fclose(f);

    if (chmod(restore.data, 0755) == -1) {
        fprintf(
            stderr,
            "ERROR: Could not mark wallpaper restore script '%s' as executable\n",
            restore.data);
        return_defer(result);
    }

    printf("Created wallpaper restore script '%s'\n", restore.data);

defer:
    da_free(&restore);
    return result;
}

int main(int argc, const char **argv) {
    App app = {0};

    if (argc >= 2) {
        const char *flag = argv[1];
        if (!strcmp(flag, "-h")) {
            usage(stdout);
            return 0;
        }

        if (!strcmp(flag, "-s")) {
            if (argc > 2) {
                char *endptr;
                const size_t delay = strtoul(argv[2], &endptr, 10);

                if (*endptr != '\0' || (delay == ULONG_MAX && errno == ERANGE)) {
                    fprintf(stderr, "ERROR: Invalid delay '%s'\n", argv[2]);
                    return 1;
                }

                sleep(delay);
            }

            app_init(&app);
            app_screenshot(&app);
            XCloseDisplay(app.display);
            return 0;
        }

        if (!strcmp(flag, "-w")) {
            if (argc == 2) {
                fprintf(stderr, "ERROR: Wallpaper image not provided\n");
                fprintf(stderr, "Usage: thono -w <image>\n");
                return 1;
            }

            return wallpaper(&app, argv[2]);
        }

        if (!strcmp(flag, "-W")) {
            if (argc == 2) {
                fprintf(stderr, "ERROR: Wallpaper image not provided\n");
                fprintf(stderr, "Usage: thono -W <image>\n");
                return 1;
            }

            return wallpaper_restore(&app, argv[2], argv[0]);
        }
    }

    app_init(&app);
    app_open(&app, argv + 1, argc - 1);
    app_loop(&app);
    app_exit(&app);
    return 0;
}
