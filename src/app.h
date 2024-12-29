#ifndef THONO_APP_H
#define THONO_APP_H

#include "camera.h"
#include "da.h"
#include "gl.h"

#include <GL/glx.h>

typedef struct {
    GLubyte r;
    GLubyte g;
    GLubyte b;
    GLubyte a;
} Pixel;

typedef struct {
    Pixel *data;
    size_t width;
    size_t height;

    const char *path;
} Image;

typedef struct {
    Display *display;
    Window window;

    int revert_return;
    Window revert_window;

    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLuint program;
    GLuint texture;
    GLXContext glx_context;

    GLint uniform_lens;
    GLint uniform_zoom;
    GLint uniform_flash;
    GLint uniform_mouse;
    GLint uniform_offset;
    GLint uniform_aspect;

    Bool focus;
    Bool dragging;

    Vec2 size;
    Vec2 mouse;

    Camera final;
    Camera camera;

    size_t current;
    DynamicArray(Image) images;

    XImage *wallpaper; // Owned
} App;

void app_init(App *a);
void app_open(App *a, const char **paths, size_t count);
void app_loop(App *a);
void app_exit(App *a);

void app_wallpaper(App *a);
void app_screenshot(App *a);

#endif // THONO_APP_H
