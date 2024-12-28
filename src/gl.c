#include <stdio.h>
#include <stdlib.h>

#include "gl.h"

GLint get_uniform(GLuint program, const char *name) {
    const GLint uniform = glGetUniformLocation(program, name);
    if (uniform < 0) {
        fprintf(stderr, "ERROR: Could not get uniform '%s'\n", name);
        exit(1);
    }
    return uniform;
}

GLuint compile_shader(const char *source, GLenum type) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        fprintf(stderr, "ERROR: Could not compile shader\n%s\n", log);
        exit(1);
    }

    return shader;
}

GLuint compile_program(const char *vs_source, const char *fs_source) {
    const GLuint vs = compile_shader(vs_source, GL_VERTEX_SHADER);
    const GLuint fs = compile_shader(fs_source, GL_FRAGMENT_SHADER);

    const GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    int ok;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(program, sizeof(log), NULL, log);
        fprintf(stderr, "ERROR: Could not link program\n%s\n", log);
        exit(1);
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}
