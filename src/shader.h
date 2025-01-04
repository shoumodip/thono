#ifndef SHADER_H
#define SHADER_H

#include <stdbool.h>

#define return_defer(value)                                                                        \
    do {                                                                                           \
        result = (value);                                                                          \
        goto defer;                                                                                \
    } while (0)

bool shader_init(void);
void shader_free(void);

extern const char *image_fs;
extern const char *image_vs;

extern const char *overlay_fs;
extern const char *overlay_vs;

#endif // SHADER_H
