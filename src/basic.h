#ifndef BASIC_H
#define BASIC_H

#include <stdbool.h>
#include <stddef.h>

#define return_defer(value)                                                                        \
    do {                                                                                           \
        result = (value);                                                                          \
        goto defer;                                                                                \
    } while (0)

char *read_file(const char *path);

typedef struct {
    char  *data;
    size_t size;
} SV;

SV sv_from_cstr(char *data);
SV sv_split(SV *s, char ch);

bool wait_till_file_exists(const char *dirpath, const char *basepath, const char *fullpath);

#endif // BASIC_H
