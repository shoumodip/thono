#include <stdio.h>
#include <string.h>

#include "app.h"

void usage(FILE *f) {
    fprintf(f, "Usage:\n");
    fprintf(f, "    thono [FLAG] [IMAGES]...\n\n");
    fprintf(f, "Flags:\n");
    fprintf(f, "    -h   Show this help message\n");
    fprintf(f, "    -s   Take a screenshot and exit\n\n");
    fprintf(f, "Image:\n");
    fprintf(f, "    Thono can be used as an image viewer if an image path is provided\n");
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
            app_init(&app);
            app_save(&app);
            XCloseDisplay(app.display);
            return 0;
        }
    }

    app_init(&app);
    app_open(&app, argv + 1, argc - 1);
    app_loop(&app);
    app_exit(&app);
}
