#ifndef SHADER_H
#define SHADER_H

#include <stdbool.h>

bool shader_init(void);
void shader_free(void);

extern const char *image_fs;
extern const char *image_vs;

extern const char *overlay_fs;
extern const char *overlay_vs;

#endif // SHADER_H
