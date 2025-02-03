#ifndef BASIC_H
#define BASIC_H

#include <stdbool.h>

#define return_defer(value)                                                                        \
    do {                                                                                           \
        result = (value);                                                                          \
        goto defer;                                                                                \
    } while (0)

char *read_file(const char *path);

bool wait_till_file_exists(const char *dirpath, const char *basepath, const char *fullpath);

#endif // BASIC_H
