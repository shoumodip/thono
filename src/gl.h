#ifndef GL_H
#define GL_H

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

GLint get_uniform(GLuint program, const char *name);
GLuint compile_shader(const char *source, GLenum type);
GLuint compile_program(const char *vs_source, const char *fs_source);

#endif // GL_H
