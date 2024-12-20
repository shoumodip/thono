#include <sys/time.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "app.h"
#include "config.h"

static const char *vs_source = //
    "#version 330 core\n"
    "layout (location = 0) in vec2 pos;\n"
    "layout (location = 1) in vec2 uv;\n"
    "\n"
    "out vec2 texcoord;\n"
    "\n"
    "uniform float zoom;\n"
    "uniform vec2 offset;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    gl_Position = vec4(pos * zoom + offset, 0.0, 1.0);\n"
    "    texcoord = uv;\n"
    "}\n";

static const char *fs_source = //
    "#version 330 core\n"
    "\n"
    "in vec2 texcoord;\n"
    "out vec4 color;\n"
    "\n"
    "uniform float lens;\n"
    "uniform float zoom;\n"
    "uniform vec4 flash;\n"
    "uniform vec2 mouse;\n"
    "uniform vec2 offset;\n"
    "uniform float aspect;\n"
    "uniform sampler2D image;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    color = texture(image, texcoord);\n"
    "    if (length((mouse - texcoord) * vec2(aspect, 1.0)) >= lens / zoom) {\n"
    "        color *= flash;\n"
    "    }\n"
    "}\n";

static double get_time(void) {
    struct timeval time = {0};
    if (gettimeofday(&time, NULL) < 0) {
        fprintf(stderr, "Error: could not get time\n");
        exit(1);
    }
    return time.tv_sec + time.tv_usec / 1e6;
}

static void app_zero(App *a) {
    a->focus = False;
    a->final.lens = THONO_LENS_SIZE;
    a->final.zoom = 1.0;
    a->final.flash = (Vec4){1.0, 1.0, 1.0, 1.0};
    a->final.offset = vec2_scale(a->size, 0.5);
}

static void app_zoom(App *a, float factor) {
    const Vec2 world = camera_world(&a->final, a->mouse);
    a->final.zoom *= factor;
    a->final.offset = vec2_sub(a->mouse, vec2_scale(world, a->final.zoom));
}

typedef struct {
    GLubyte r;
    GLubyte g;
    GLubyte b;
    GLubyte a;
} Pixel;

static Pixel *app_snap(App *a) {
    const uint width = a->size.x;
    const uint height = a->size.y;
    const Window root = DefaultRootWindow(a->display);

    XImage *image = XGetImage(a->display, root, 0, 0, width, height, AllPlanes, ZPixmap);
    if (!image) {
        fprintf(stderr, "Error: could not capture screenshot\n");
        exit(1);
    }

    Pixel *pixels = malloc(width * height * sizeof(Pixel));
    if (!pixels) {
        fprintf(stderr, "Error: could not allocate screenshot buffer\n");
        exit(1);
    }

    for (uint y = 0; y < height; ++y) {
        for (uint x = 0; x < width; ++x) {
            const ulong p = XGetPixel(image, x, y);
            Pixel *it = &pixels[y * width + x];
            it->r = (p & image->red_mask) >> 16;
            it->g = (p & image->green_mask) >> 8;
            it->b = (p & image->blue_mask);
            it->a = 0xFF;
        }
    }

    XDestroyImage(image);
    return pixels;
}

void app_init(App *a) {
    a->display = XOpenDisplay(NULL);
    if (!a->display) {
        fprintf(stderr, "Error: could not open display\n");
        exit(1);
    }

    XWindowAttributes wa = {0};
    XGetWindowAttributes(a->display, DefaultRootWindow(a->display), &wa);
    a->size = (Vec2){wa.width, wa.height};
}

typedef struct {
    Vec2 pos;
    Vec2 uv;
} Vertex;

void app_open(App *a, const char *path) {
    app_zero(a);
    a->camera = a->final;

    {
        int x = 0, y = 0;
        unsigned int mask;

        Window root = DefaultRootWindow(a->display);
        XQueryPointer(a->display, root, &root, &root, &x, &y, &x, &y, &mask);

        a->mouse = (Vec2){x, y};
    }

    Pixel *image = NULL;
    size_t image_width = a->size.x;
    size_t image_height = a->size.y;

    if (path) {
        int w, h;
        Pixel *data = (Pixel *)stbi_load(path, &w, &h, NULL, 4);
        if (!data) {
            fprintf(stderr, "Error: could not load image '%s'\n", path);
            exit(1);
        }

        if (w > image_width || h > image_height) {
            const float ax = (float)w / image_width;
            const float ay = (float)h / image_height;
            const float scale = ax > ay ? ax : ay;

            image_width = floor(image_width * scale);
            image_height = floor(image_height * scale);
        }

        image = malloc(image_width * image_height * sizeof(Pixel));
        if (!image) {
            fprintf(stderr, "Error: could not allocate image buffer\n");
            exit(1);
        }

        const size_t x = (image_width - w) / 2;
        const size_t y = (image_height - h) / 2;
        for (size_t j = 0; j < image_height; ++j) {
            for (size_t i = 0; i < image_width; ++i) {
                if (i >= x && i < x + w && j >= y && j < y + h) {
                    image[j * (size_t)image_width + i] = data[(j - y) * w + (i - x)];
                } else {
                    const Vec4 c = vec4_scale((Vec4){THONO_BACKGROUND_COLOR}, 0xFF);
                    image[j * (size_t)image_width + i] = (Pixel){c.x, c.y, c.z, c.w};
                }
            }
        }

        stbi_image_free(data);
    } else {
        image = app_snap(a);
    }

    GLint glx_attribs[] = {GLX_RGBA, GLX_DOUBLEBUFFER, GLX_DEPTH_SIZE, 24, None};
    XVisualInfo *vi = glXChooseVisual(a->display, 0, glx_attribs);
    if (!vi) {
        fprintf(stderr, "Error: no appropriate visual found\n");
        exit(1);
    }

    const Window root = DefaultRootWindow(a->display);

    XSetWindowAttributes wa;
    wa.colormap = XCreateColormap(a->display, root, vi->visual, AllocNone);
    wa.event_mask = ButtonPressMask | ButtonReleaseMask | KeyPressMask | PointerMotionMask |
                    ExposureMask | VisibilityChangeMask | FocusChangeMask;

    wa.override_redirect = True;
    wa.save_under = True;

    a->window = XCreateWindow(
        a->display,
        root,
        0,
        0,
        a->size.x,
        a->size.y,
        0,
        vi->depth,
        InputOutput,
        vi->visual,
        CWColormap | CWEventMask | CWOverrideRedirect | CWSaveUnder,
        &wa);

    a->glx_context = glXCreateContext(a->display, vi, NULL, GL_TRUE);
    glXMakeCurrent(a->display, a->window, a->glx_context);

    XGrabKeyboard(a->display, root, True, GrabModeAsync, GrabModeAsync, CurrentTime);
    XGrabPointer(
        a->display, a->window, True, 0, GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
    XMapRaised(a->display, a->window);

    XGetInputFocus(a->display, &a->revert_window, &a->revert_return);
    XSetInputFocus(a->display, a->window, RevertToParent, CurrentTime);
    XSelectInput(a->display, root, SubstructureNotifyMask);

    a->program = compile_program(vs_source, fs_source);
    glUseProgram(a->program);

    glGenVertexArrays(1, &a->vao);
    glGenBuffers(1, &a->vbo);
    glGenBuffers(1, &a->ebo);

    glBindVertexArray(a->vao);

    static const Vertex vertices[] = {
        {{-1.0f, -1.0f}, {0.0f, 1.0f}},
        {{1.0f, -1.0f}, {1.0f, 1.0f}},
        {{1.0f, 1.0f}, {1.0f, 0.0f}},
        {{-1.0f, 1.0f}, {0.0f, 0.0f}},
    };
    glBindBuffer(GL_ARRAY_BUFFER, a->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    static const GLuint indices[] = {0, 1, 2, 2, 3, 0};
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, a->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, uv));
    glEnableVertexAttribArray(1);

    glGenTextures(1, &a->texture);
    glBindTexture(GL_TEXTURE_2D, a->texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);
    free(image);

    a->uniform_lens = get_uniform(a->program, "lens");
    a->uniform_zoom = get_uniform(a->program, "zoom");
    a->uniform_flash = get_uniform(a->program, "flash");
    a->uniform_mouse = get_uniform(a->program, "mouse");
    a->uniform_offset = get_uniform(a->program, "offset");
    a->uniform_aspect = get_uniform(a->program, "aspect");
}

void app_draw(App *a) {
    const Vec4 bg = vec4_mul(a->camera.flash, (Vec4){THONO_BACKGROUND_COLOR});
    glClearColor(bg.x, bg.y, bg.z, bg.w);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindTexture(GL_TEXTURE_2D, a->texture);

    glUniform1f(a->uniform_lens, a->camera.lens);
    glUniform1f(a->uniform_zoom, a->camera.zoom);
    glUniform4f(
        a->uniform_flash,
        a->camera.flash.x,
        a->camera.flash.y,
        a->camera.flash.z,
        a->camera.flash.w);

    const Vec2 mouse =
        vec2_div(vec2_add(camera_world(&a->camera, a->mouse), vec2_scale(a->size, 0.5)), a->size);
    glUniform2f(a->uniform_mouse, mouse.x, mouse.y);

    glUniform2f(
        a->uniform_offset,
        2.0 * a->camera.offset.x / a->size.x - 1.0,
        1.0 - 2.0 * a->camera.offset.y / a->size.y);

    glUniform1f(a->uniform_aspect, a->size.x / a->size.y);

    glBindVertexArray(a->vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glXSwapBuffers(a->display, a->window);
}

void app_save(App *a) {
    Pixel *image = app_snap(a);
    const long long since = get_time() * 1000;

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "thono-%lld.png", since);

    if (!stbi_write_png(buffer, a->size.x, a->size.y, 4, image, a->size.x * sizeof(*image))) {
        fprintf(stderr, "Error: could not save screenshot to png\n");
        exit(1);
    }

    free(image);
}

void app_loop(App *a) {
    double pt = get_time();
    while (True) {
        const double dt = get_time() - pt;
        camera_update(&a->camera, &a->final, dt);
        pt += dt;

        app_draw(a);
        while (XPending(a->display)) {
            XEvent e;
            XNextEvent(a->display, &e);

            switch (e.type) {
            case Expose:
                app_draw(a);
                break;

            case FocusOut:
                XSetInputFocus(a->display, a->window, RevertToParent, CurrentTime);
                break;

            case VisibilityNotify:
                if (((XVisibilityEvent *)&e)->state != VisibilityUnobscured) {
                    XRaiseWindow(a->display, a->window);
                }
                break;

            case ConfigureNotify:
                if (((XConfigureEvent *)&e)->window != a->window) {
                    XRaiseWindow(a->display, a->window);
                }
                break;

            case ButtonPress:
                switch (e.xbutton.button) {
                case Button1:
                    a->dragging = True;
                    a->mouse = (Vec2){e.xbutton.x, e.xbutton.y};
                    break;

                case Button2:
                    return;

                case Button3:
                    a->focus = !a->focus;
                    if (a->focus) {
                        a->final.flash = (Vec4){THONO_FLASHLIGHT_COLOR};
                    } else {
                        a->final.flash = (Vec4){1.0, 1.0, 1.0, 1.0};
                    }
                    break;

                case Button4:
                    if (a->focus) {
                        a->final.lens *= THONO_LENS_FACTOR;
                    } else {
                        app_zoom(a, THONO_ZOOM_FACTOR);
                    }
                    break;

                case Button5:
                    if (a->focus) {
                        a->final.lens /= THONO_LENS_FACTOR;
                    } else {
                        app_zoom(a, 1.0 / THONO_ZOOM_FACTOR);
                    }
                    break;
                }
                break;

            case ButtonRelease:
                if (e.xbutton.button == Button1) {
                    a->dragging = False;
                }
                break;

            case MotionNotify: {
                const Vec2 pos = (Vec2){e.xmotion.x, e.xmotion.y};
                if (a->dragging) {
                    a->final.offset = vec2_add(a->final.offset, vec2_sub(pos, a->mouse));
                }
                a->mouse = pos;
            } break;

            case KeyPress:
                switch (XLookupKeysym(&e.xkey, 0)) {
                case 'q':
                    return;

                case 's':
                    app_save(a);
                    break;

                case '0':
                    app_zero(a);
                    break;
                }
                break;
            }
        }
    }
}

void app_exit(App *a) {
    glDeleteVertexArrays(1, &a->vao);
    glDeleteBuffers(1, &a->vbo);
    glDeleteBuffers(1, &a->ebo);
    glDeleteProgram(a->program);
    glDeleteTextures(1, &a->texture);

    glXMakeCurrent(a->display, None, NULL);
    glXDestroyContext(a->display, a->glx_context);

    XSetInputFocus(a->display, a->revert_window, a->revert_return, CurrentTime);
    XUngrabKeyboard(a->display, CurrentTime);
    XUngrabPointer(a->display, CurrentTime);
    XDestroyWindow(a->display, a->window);
    XCloseDisplay(a->display);
}
