#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <limits.h>
#include <sys/stat.h>

#include "app.h"
#include "basic.h"
#include "config.h"

#include "stb_image.h"
#include "stb_image_resize2.h"

static void usage(FILE *f) {
    fprintf(f, "Usage:\n");
    fprintf(f, "    thono [FLAG] [PATHS]...\n\n");
    fprintf(f, "Flags:\n");
    fprintf(f, "    -h          Show this help message\n");
    fprintf(f, "    -w <image>  Set the image as wallpaper\n");
    fprintf(f, "    -W <image>  Set the image as wallpaper and create a restore script\n");
    fprintf(f, "    -s [delay]  Take a screenshot and exit, with optional delay\n");
    fprintf(f, "    -r [delay]  Select a region, screenshot and exit, with optional delay\n");
    fprintf(f, "    -R          Open images recursively in the image viewer\n\n");
    fprintf(f, "Image:\n");
    fprintf(f, "    Thono can be used as an image viewer if image/directory paths are provided\n");
}

static int wallpaper(App *a, const char *path) {
    int      result = 0;
    uint8_t *image = NULL;

    int      w, h;
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
            const uint8_t  r = image[(y * width + x) * sizeof(uint32_t) + 0];
            const uint8_t  g = image[(y * width + x) * sizeof(uint32_t) + 1];
            const uint8_t  b = image[(y * width + x) * sizeof(uint32_t) + 2];
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

static int wallpaper_restore(App *a, const char *path) {
    int result = 0;
    DynamicArray(char) b = {0};

    result = wallpaper(a, path);
    if (result) return_defer(result);

    const char *env_restore_path = getenv("THONO_WALLPAPER_RESTORE_PATH");
    if (env_restore_path) {
        da_append_cstr(&b, env_restore_path);
        da_append(&b, '\0');
    } else {
        const char *env_home = getenv("HOME");
        if (!env_home) return_defer(result);

        da_append_cstr(&b, env_home);
        da_append(&b, '/');
        da_append_cstr(&b, WALLPAPER_RESTORE_PATH_DEFAULT);
        da_append(&b, '\0');
    }

    const size_t program = b.count;
    while (true) {
        da_append_many(&b, NULL, DA_INIT_CAP);

        const size_t capacity = b.capacity - program;
        const long   count = readlink("/proc/self/exe", b.data + program, capacity);

        if (count < 0) {
            fprintf(stderr, "ERROR: Could not get current program path\n");
            return_defer(1);
        }

        if (count < capacity) {
            b.count += count;
            break;
        }

        b.count = b.capacity;
    }
    da_append(&b, '\0');

    const size_t wallpaper = b.count;
    if (*path != '/') {
        da_append_many(&b, NULL, DA_INIT_CAP);
        while (!getcwd(b.data + wallpaper, b.capacity - wallpaper)) {
            if (errno != ERANGE) {
                fprintf(stderr, "ERROR: Could not get current working directory\n");
                return_defer(1);
            }

            b.count = b.capacity;
            da_append_many(&b, NULL, DA_INIT_CAP);
        }

        b.count = wallpaper + strlen(b.data + wallpaper);
        da_append(&b, '/');
    }

    da_append_cstr(&b, path);
    da_append(&b, '\0');

    FILE *f = fopen(b.data, "w");
    if (!f) {
        fprintf(stderr, "ERROR: Could not create wallpaper restore script '%s'\n", b.data);
        return_defer(result);
    }

    fprintf(
        f,
        "#!/bin/sh\n"
        "%s -w %s\n",
        b.data + program,
        b.data + wallpaper);

    fclose(f);

    if (chmod(b.data, 0755) == -1) {
        fprintf(
            stderr, "ERROR: Could not mark wallpaper restore script '%s' as executable\n", b.data);
        return_defer(result);
    }

    printf("Created wallpaper restore script '%s'\n", b.data);

defer:
    da_free(&b);
    return result;
}

int main(int argc, const char **argv) {
    App app = {0};
    if (argc >= 2) {
        const char *flag = argv[1];
        if (!strcmp(flag, "-h")) {
            usage(stdout);
            return 0;
        } else if (!strcmp(flag, "-s")) {
            if (argc > 2) {
                char        *endptr;
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
        } else if (!strcmp(flag, "-r")) {
            if (argc > 2) {
                char        *endptr;
                const size_t delay = strtoul(argv[2], &endptr, 10);

                if (*endptr != '\0' || (delay == ULONG_MAX && errno == ERANGE)) {
                    fprintf(stderr, "ERROR: Invalid delay '%s'\n", argv[2]);
                    return 1;
                }

                sleep(delay);
            }

            app_init(&app);

            app.select_on = true;
            app.select_exit = true;
            app_open(&app, NULL, 0);
            app_loop(&app);
            app_exit(&app);
            return 0;
        } else if (!strcmp(flag, "-w")) {
            if (argc == 2) {
                fprintf(stderr, "ERROR: Wallpaper image not provided\n");
                fprintf(stderr, "Usage: thono -w <image>\n");
                return 1;
            }

            return wallpaper(&app, argv[2]);
        } else if (!strcmp(flag, "-W")) {
            if (argc == 2) {
                fprintf(stderr, "ERROR: Wallpaper image not provided\n");
                fprintf(stderr, "Usage: thono -W <image>\n");
                return 1;
            }

            return wallpaper_restore(&app, argv[2]);
        } else if (!strcmp(flag, "-R")) {
            app.recursive = true;
            argv++;
            argc--;
        }
    }

    app_init(&app);
    app_open(&app, argv + 1, argc - 1);
    app_loop(&app);
    app_exit(&app);
    return 0;
}
