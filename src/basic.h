#ifndef BASIC_H
#define BASIC_H

#include <stdbool.h>
#include <stddef.h>

#define return_defer(value)                                                                        \
    do {                                                                                           \
        result = (value);                                                                          \
        goto defer;                                                                                \
    } while (0)

// TODO: return a SV
char *read_file(const char *path);

typedef struct {
    char  *data;
    size_t size;
} SV;

#define SVFmt    "%.*s"
#define SVArg(s) (int) (s).size, (s).data

SV sv_from_cstr(char *data);
SV sv_split(SV *s, char ch);

#endif // BASIC_H
