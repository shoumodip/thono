#include <stdio.h>
#include <string.h>

#include "app.h"

void usage(FILE *f) {
    fprintf(f, "Usage:\n");
    fprintf(f, "    thono [FLAG|IMAGE]\n\n");
    fprintf(f, "Flags:\n");
    fprintf(f, "    -h   Show this help message\n");
    fprintf(f, "    -s   Take a screenshot and exit\n\n");
    fprintf(f, "Image:\n");
    fprintf(f, "    Thono can be used as an image viewer if an image path is provided\n");
}

int main(int argc, char **argv) {
    App app = {0};

    const char *path = NULL;
    if (argc >= 2) {
        path = argv[1];
        if (!strcmp(path, "-h")) {
            usage(stdout);
            return 0;
        }

        if (!strcmp(path, "-s")) {
            app_init(&app);
            app_save(&app);
            XCloseDisplay(app.display);
            return 0;
        }
    }

    app_init(&app);
    app_open(&app, path);
    app_loop(&app);
    app_exit(&app);
}
