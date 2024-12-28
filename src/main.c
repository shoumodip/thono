#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "app.h"

void usage_and_exit(FILE *f) {
    fprintf(f, "Usage:\n");
    fprintf(f, "    thono [FLAG] [IMAGES]...\n\n");
    fprintf(f, "Flags:\n");
    fprintf(f, "    -h          Show this help message\n");
    fprintf(f, "    -s [delay]  Take a screenshot and exit, with optional delay\n\n");
    fprintf(f, "Image:\n");
    fprintf(f, "    Thono can be used as an image viewer if an image path is provided\n");
    exit(0);
}

void screenshot_and_exit(App *a) {
    app_init(a);
    app_save(a);
    XCloseDisplay(a->display);
    exit(0);
}

int main(int argc, const char **argv) {
    App app = {0};

    if (argc >= 2) {
        const char *flag = argv[1];
        if (!strcmp(flag, "-h")) {
            usage_and_exit(stdout);
        }

        if (!strcmp(flag, "-s")) {
            if (argc > 2) {
                char *endptr;
                const size_t delay = strtoul(argv[2], &endptr, 10);

                if (*endptr != '\0' || (delay == ULONG_MAX && errno == ERANGE)) {
                    fprintf(stderr, "ERROR: Invalid delay '%s'\n", argv[2]);
                    exit(1);
                }

                sleep(delay);
            }

            screenshot_and_exit(&app);
        }
    }

    app_init(&app);
    app_open(&app, argv + 1, argc - 1);
    app_loop(&app);
    app_exit(&app);
}
