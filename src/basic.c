#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "basic.h"

char *read_file(const char *path) {
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

SV sv_from_cstr(char *data) {
    return (SV) {.data = data, .size = strlen(data)};
}

SV sv_split(SV *s, char ch) {
    char *p = memchr(s->data, ch, s->size);
    if (!p) {
        const SV result = *s;
        s->data += s->size;
        s->size = 0;
        return result;
    }

    const SV result = (SV) {.data = s->data, .size = p - s->data};
    s->data = p + 1;
    s->size -= result.size + 1;
    return result;
}
