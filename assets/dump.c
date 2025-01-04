#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/basic.h"

#define COLS 13

static void guard(const char *p) {
    while (*p) {
        printf("%c", toupper(*p++));
    }
    printf("_C\n");
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "ERROR: Insufficient arguments\n");
        fprintf(stderr, "Usage: %s <file> <name>\n", *argv);
        return 1;
    }

    const char *path = argv[1];
    const char *name = argv[2];

    char *contents = read_file(path);
    if (!contents) {
        return 1;
    }
    const size_t length = strlen(contents);

    printf("#ifndef ");
    guard(name);

    printf("#define ");
    guard(name);
    printf("\n");

    printf("static const char %s_data[] = {\n", name);
    for (size_t i = 0; i <= length;) {
        printf("   ");
        for (size_t j = 0; i <= length && j < COLS; i++, j++) {
            printf(" 0x%02X,", contents[i]);
        }
        printf("\n");
    }
    printf("};\n\n");
    printf("const char *%s = %s_data;\n\n", name, name);

    printf("#endif // ");
    guard(name);

    free(contents);
}
