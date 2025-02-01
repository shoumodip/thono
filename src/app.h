#ifndef APP_H
#define APP_H

#include "da.h"
#include "gl.h"

#include "camera.h"
#include "shader.h"

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
    Window   window;

    int    revert_return;
    Window revert_window;

    GLuint     vao;
    GLuint     vbo;
    GLuint     ebo;
    GLuint     texture;
    GLXContext glx_context;

    GLuint image_program;
    GLint  image_uniform_zoom;
    GLint  image_uniform_offset;

    GLuint overlay_program;
    GLint  overlay_uniform_mouse;
    GLint  overlay_uniform_aspect;

    GLint overlay_uniform_lens_size;
    GLint overlay_uniform_lens_color;

    GLint overlay_uniform_select_began;
    GLint overlay_uniform_select_mouse;
    GLint overlay_uniform_select_start;

    bool focus;
    bool dragging;

    Vec2 size;
    Vec2 mouse;

    Camera final;
    Camera camera;

    size_t current;
    DynamicArray(Image) images;

    bool recursive;
    DynamicArray(char) paths;

    XImage *wallpaper; // Owned

    bool   select_on;    // Whether the selection mode is on
    bool   select_exit;  // Whether the app should immediately exit after end of selection
    bool   select_began; // Whether the actual selection has started yet
    Vec2   select_start;
    Cursor select_cursor;
    size_t select_snap_pending;
} App;

void app_init(App *a);
void app_open(App *a, const char **paths, size_t count);
void app_loop(App *a);
void app_exit(App *a);

void app_wallpaper(App *a);
void app_screenshot(App *a);

#endif // APP_H
