#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <unistd.h>

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

bool wait_till_file_exists(const char *dirpath, const char *basepath, const char *fullpath) {
    if (access(fullpath, F_OK) == 0) {
        return true;
    }

    const int fd = inotify_init();
    if (fd < 0) {
        return false;
    }

    const int wd = inotify_add_watch(fd, dirpath, IN_CREATE);
    if (wd < 0) {
        return false;
    }

    static char buffer[(1024 * (sizeof(struct inotify_event) + 16))];
    while (1) {
        const long n = read(fd, buffer, sizeof(buffer));
        if (n < 0) {
            return false;
        }

        size_t i = 0;
        while (i < n) {
            struct inotify_event *e = (struct inotify_event *) &buffer[i];
            if (e->len && e->mask & IN_CREATE) {
                if (strcmp(e->name, basepath) == 0) {
                    close(fd);
                    return true;
                }
            }
            i += sizeof(struct inotify_event) + e->len;
        }
    }
}
