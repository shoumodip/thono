#include <ctype.h>

#define NOB_IMPLEMENTATION
#include "src/nob.h"

static bool build_stb_library(Nob_Cmd *cmd, const char *name) {
    const char *header_file = nob_temp_sprintf("src/stb_%s.h", name);
    const char *object_file = nob_temp_sprintf("build/stb_%s.o", name);
    if (!nob_needs_rebuild1(object_file, header_file)) return true;

    char *macro = nob_temp_sprintf("-DSTB_%s_IMPLEMENTATION", name);
    for (char *p = macro; *p; p++) {
        if (islower(*p)) {
            *p = toupper(*p);
        }
    }

    nob_cmd_append(cmd, "cc", "-O3", "-o", object_file, macro, "-x", "c", "-c", header_file);
    return nob_cmd_run_sync_and_reset(cmd);
}

#define SHADER_COLS        13
#define SHADER_INPUT_DIR   "assets/"
#define SHADER_OUTPUT_FILE "build/shader.c"

static char *array_name_from_path(const char *path_cstr) {
    Nob_String_View path = nob_sv_from_cstr(path_cstr);
    while (path.count) {
        const Nob_String_View left = nob_sv_chop_by_delim(&path, '/');
        if (!path.count) {
            path = left;
            break;
        }
    }

    char *result = nob_temp_strdup(path.data);
    for (size_t i = 0; i < path.count; i++) {
        if (result[i] == '.') {
            result[i] = '_';
        }
    }

    return result;
}

static bool build_shader_file(void) {
    bool result = true;

    FILE *f = fopen(SHADER_OUTPUT_FILE, "w");
    if (!f) {
        fprintf(stderr, "ERROR: Could not create file '%s'\n", SHADER_OUTPUT_FILE);
        nob_return_defer(false);
    }

    Nob_File_Paths paths = {0};
    if (!nob_read_entire_dir(SHADER_INPUT_DIR, &paths)) nob_return_defer(false);

    Nob_String_Builder sb = {0};
    for (size_t i = 0; i < paths.count; i++) {
        const char *input = paths.items[i];
        if (!strcmp(input, ".") || !strcmp(input, "..")) continue;

        const char *path = nob_temp_sprintf(SHADER_INPUT_DIR "%s", input);
        if (!nob_read_entire_file(path, &sb)) nob_return_defer(false);
        nob_sb_append_null(&sb);

        const char *name = array_name_from_path(path);
        fprintf(f, "const char %s[] = {\n", name);
        for (size_t i = 0; i <= sb.count;) {
            fprintf(f, "   ");
            for (size_t j = 0; i <= sb.count && j < SHADER_COLS; i++, j++) {
                fprintf(f, " 0x%02X,", sb.items[i]);
            }
            fprintf(f, "\n");
        }
        fprintf(f, "};\n");

        sb.count = 0;
    }

defer:
    if (f) fclose(f);
    nob_sb_free(sb);
    nob_da_free(paths);
    return result;
}

static bool push_matches_into_cmd(Nob_Cmd *cmd, const char *dir, const char *extension) {
    bool result = true;

    Nob_File_Paths paths = {0};
    if (!nob_read_entire_dir(dir, &paths)) nob_return_defer(false);

    for (size_t i = 0; i < paths.count; i++) {
        const char *it = paths.items[i];
        if (!strcmp(it, ".") || !strcmp(it, "..")) continue;

        if (nob_sv_end_with(nob_sv_from_cstr(it), extension)) {
            nob_cmd_append(cmd, nob_temp_sprintf("%s/%s", dir, it));
        }
    }

defer:
    nob_da_free(paths);
    return result;
}

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);
    if (!nob_mkdir_if_not_exists("build")) return 1;

    Nob_Cmd cmd = {0};
    if (!build_stb_library(&cmd, "image")) return 1;
    if (!build_stb_library(&cmd, "image_write")) return 1;
    if (!build_stb_library(&cmd, "image_resize2")) return 1;
    if (!build_shader_file()) return 1;

    nob_cmd_append(&cmd, "cc", "-O3", "-o", "build/thono");
    if (!push_matches_into_cmd(&cmd, "src", ".c")) return 1;
    if (!push_matches_into_cmd(&cmd, "build", ".o")) return 1;
    nob_cmd_append(&cmd, "build/shader.c", "-lm", "-lGL", "-lX11");

    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;

    nob_log(NOB_INFO, "Build executable `thono`");
    return 0;
}
