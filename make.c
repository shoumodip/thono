#include "make.h"

#define CC "cc"
#define CFLAGS "-Wall", "-Wextra", "-std=c11", "-pedantic"
#define LIBS "-lXrender", "-lX11", "-ljpeg"

int main(int argc, char **argv)
{
    make_rebuild(argc, argv);
    make_assert(argc > 1, "no subcommand provided");

    if (strcmp(argv[1], "build") == 0) {
        make_cc("thono", "thono.c", CFLAGS, LIBS);
    } else if (strcmp(argv[1], "run") == 0) {
        make_cc("thono", "thono.c", CFLAGS, LIBS);
        make_cmd("./thono");
    } else {
        make_error("unknown subcommand `%s`", argv[1]);
    }
}
