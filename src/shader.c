#include <stdio.h>
#include <stdlib.h>

#include "shader.h"

#ifdef RELEASE

const char *image_fs =
#    include "../.build/image_fs"
    ;

const char *image_vs =
#    include "../.build/image_vs"
    ;

const char *overlay_fs =
#    include "../.build/overlay_fs"
    ;

const char *overlay_vs =
#    include "../.build/overlay_vs"
    ;

bool shader_init(void) {
    return true;
}

void shader_free(void) {}
#else

#    define return_defer(value)                                                                    \
        do {                                                                                       \
            result = (value);                                                                      \
            goto defer;                                                                            \
        } while (0)

static char *read_file(const char *path) {
    char *result = NULL;

    FILE *f = fopen(path, "r");
    if (!f) {
        return_defer(NULL);
    }

    if (fseek(f, 0, SEEK_END) == -1) {
        return_defer(NULL);
    }

    const long offset = ftell(f);
    if (offset == -1) {
        return_defer(NULL);
    }

    if (fseek(f, 0, SEEK_SET) == -1) {
        return_defer(NULL);
    }

    result = malloc(offset + 1);
    if (!result) {
        return_defer(NULL);
    }

    const size_t count = fread(result, 1, offset, f);
    if (ferror(f)) {
        free(result);
        return_defer(NULL);
    }
    result[count] = '\0';

defer:
    if (f) fclose(f);
    if (!result) fprintf(stderr, "ERROR: Could not read file '%s'\n", path);
    return result;
}

const char *image_fs;
const char *image_vs;

const char *overlay_fs;
const char *overlay_vs;

bool shader_init(void) {
    image_fs = read_file("shaders/image.fs");
    image_vs = read_file("shaders/image.vs");

    overlay_fs = read_file("shaders/overlay.fs");
    overlay_vs = read_file("shaders/overlay.vs");

    return image_fs && image_vs && overlay_fs && overlay_vs;
}

void shader_free(void) {
    free((char *)image_fs);
    free((char *)image_vs);

    free((char *)overlay_fs);
    free((char *)overlay_vs);
}

#endif
