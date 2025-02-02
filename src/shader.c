#include "shader.h"

#ifdef RELEASE

#    include "../.build/image_fs.c"
#    include "../.build/image_vs.c"
#    include "../.build/overlay_fs.c"
#    include "../.build/overlay_vs.c"

bool shader_init(void) {
    return true;
}

void shader_free(void) {}

#else

#    include <stdlib.h>

#    include "basic.h"

const char *image_fs;
const char *image_vs;

const char *overlay_fs;
const char *overlay_vs;

bool shader_init(void) {
    image_fs = read_file("assets/image.fs");
    image_vs = read_file("assets/image.vs");

    overlay_fs = read_file("assets/overlay.fs");
    overlay_vs = read_file("assets/overlay.vs");

    return image_fs && image_vs && overlay_fs && overlay_vs;
}

void shader_free(void) {
    free((char *) image_fs);
    free((char *) image_vs);

    free((char *) overlay_fs);
    free((char *) overlay_vs);
}

#endif // RELEASE
