#ifndef BASIC_H
#define BASIC_H

#define return_defer(value)                                                                        \
    do {                                                                                           \
        result = (value);                                                                          \
        goto defer;                                                                                \
    } while (0)

char *read_file(const char *path);

#endif // BASIC_H
