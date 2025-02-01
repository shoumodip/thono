#include <dirent.h>
#include <errno.h>

#include <math.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <X11/Xatom.h>
#include <X11/cursorfont.h>

#include "app.h"
#include "basic.h"
#include "config.h"

#include "stb_image.h"
#include "stb_image_write.h"

static double get_time(void) {
    struct timeval time = {0};
    if (gettimeofday(&time, NULL) < 0) {
        fprintf(stderr, "ERROR: Could not get time\n");
        exit(1);
    }
    return time.tv_sec + time.tv_usec / 1e6;
}

static void app_zero(App *a) {
    a->focus = false;
    a->final.lens_size = LENS_SIZE;
    a->final.lens_color = (Vec4) {0};

    a->final.zoom = 1.0;
    a->final.offset = vec2_scale(a->size, 0.5);
}

static void app_zoom(App *a, float factor) {
    const Vec2 world = camera_world(&a->final, a->mouse);
    a->final.zoom *= factor;
    a->final.offset = vec2_sub(a->mouse, vec2_scale(world, a->final.zoom));
}

static XImage *app_snap_ximage(App *a, Vec2 start, Vec2 size) {
    const Window root = DefaultRootWindow(a->display);

    XImage *image =
        XGetImage(a->display, root, start.x, start.y, size.x, size.y, AllPlanes, ZPixmap);

    if (!image) {
        fprintf(stderr, "ERROR: Could not capture screenshot\n");
        exit(1);
    }

    return image;
}

static Pixel *app_snap(App *a, Vec2 start, Vec2 size) {
    const uint width = size.x;
    const uint height = size.y;

    XImage *image = app_snap_ximage(a, start, size);
    Pixel  *pixels = malloc(width * height * sizeof(Pixel));
    if (!pixels) {
        fprintf(stderr, "ERROR: Could not allocate screenshot buffer\n");
        exit(1);
    }

    for (uint y = 0; y < height; ++y) {
        for (uint x = 0; x < width; ++x) {
            const ulong p = XGetPixel(image, x, y);
            Pixel      *it = &pixels[y * width + x];
            it->r = (p & image->red_mask) >> 16;
            it->g = (p & image->green_mask) >> 8;
            it->b = (p & image->blue_mask);
            it->a = 0xFF;
        }
    }

    XDestroyImage(image);
    return pixels;
}

static void app_save_image(App *a, Vec2 start, Vec2 size) {
    Pixel          *image = app_snap(a, start, size);
    const long long since = get_time() * 1000;

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "thono-%lld.png", since);

    if (!stbi_write_png(buffer, size.x, size.y, 4, image, size.x * sizeof(*image))) {
        fprintf(stderr, "ERROR: Could not save screenshot to png\n");
        exit(1);
    }

    free(image);
}

static void app_load_image(App *a, bool next_if_failed) {
    while (true) {
        Image *image = &a->images.data[a->current];
        if (image->path) {
            int    w, h;
            Pixel *data = (Pixel *) stbi_load(image->path, &w, &h, NULL, 4);
            if (!data) {
                fprintf(stderr, "ERROR: Could not load image '%s'\n", image->path);

                da_remove(&a->images, a->current);
                if (a->images.count == 0) {
                    fprintf(
                        stderr, "ERROR: Could not load any of the requested images! Exiting...\n");
                    exit(1);
                }

                if (next_if_failed) {
                    if (a->current == a->images.count) {
                        a->current = 0;
                    }
                } else {
                    if (a->current) {
                        a->current--;
                    } else {
                        a->current = a->images.count - 1;
                    }
                }
                continue;
            }

            image->width = a->size.x;
            image->height = a->size.y;
            if (w > image->width || h > image->height) {
                const float ax = (float) w / image->width;
                const float ay = (float) h / image->height;
                const float scale = ax > ay ? ax : ay;

                image->width = floor(image->width * scale);
                image->height = floor(image->height * scale);
            }

            image->data = malloc(image->width * image->height * sizeof(Pixel));
            if (!image->data) {
                fprintf(stderr, "ERROR: Could not allocate image buffer\n");
                exit(1);
            }

            const size_t x = (image->width - w) / 2;
            const size_t y = (image->height - h) / 2;
            for (size_t j = 0; j < image->height; ++j) {
                for (size_t i = 0; i < image->width; ++i) {
                    if (i >= x && i < x + w && j >= y && j < y + h) {
                        image->data[j * (size_t) image->width + i] = data[(j - y) * w + (i - x)];
                    } else {
                        const Vec4 c = vec4_scale((Vec4) {BACKGROUND_COLOR}, 0xFF);
                        image->data[j * (size_t) image->width + i] = (Pixel) {c.x, c.y, c.z, c.w};
                    }
                }
            }

            stbi_image_free(data);
            image->path = NULL;
        }

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            image->width,
            image->height,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            image->data);

        glGenerateMipmap(GL_TEXTURE_2D);
        break;
    }

    app_zero(a);
    a->camera = a->final;
}

static int compare_images(const void *a, const void *b) {
    const Image *ia = a;
    const Image *ib = b;
    return strcmp(ia->path, ib->path);
}

static_assert(sizeof(size_t) == sizeof(const char *), "Use a proper computer bruh");
static bool app_load_dir(App *a, const char *path, bool top) {
    errno = 0;
    DIR *dir = NULL;
    bool result = true;

    dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "ERROR: Could not read directory '%s'\n", path);
        return_defer(false);
    }

    const size_t   start = a->images.count;
    struct dirent *e;
    while ((e = readdir(dir))) {
        if (!strcmp(e->d_name, "..") || !strcmp(e->d_name, ".")) {
            continue;
        }

        if (e->d_type == DT_DIR && !a->recursive) {
            continue;
        }

        const size_t copied = a->paths.count;
        da_append_cstr(&a->paths, path);
        da_append(&a->paths, '/');
        da_append_cstr(&a->paths, e->d_name);
        da_append(&a->paths, '\0');

        if (e->d_type == DT_DIR) {
            app_load_dir(a, a->paths.data + copied, false);
        } else {
            da_append(&a->images, (Image) {.path = (const char *) copied});
        }
    }

    if (errno) {
        fprintf(stderr, "ERROR: Could not read directory '%s'\n", path);
        return_defer(false);
    }

    if (top) {
        for (size_t i = start; i < a->images.count; i++) {
            a->images.data[i].path = a->paths.data + (size_t) a->images.data[i].path;
        }

        qsort(
            a->images.data + start,
            a->images.count - start,
            sizeof(*a->images.data),
            compare_images);
    }

defer:
    closedir(dir);
    return result;
}

static void app_load_path(App *a, const char *path) {
    struct stat statbuf = {0};
    if (stat(path, &statbuf) < 0) {
        fprintf(stderr, "ERROR: Could not stat file '%s'\n", path);
        return;
    }

    if (S_ISDIR(statbuf.st_mode)) {
        if (!app_load_dir(a, path, true)) {
            return;
        }
    } else {
        da_append(&a->images, (Image) {.path = path});
    }
}

void app_init(App *a) {
    a->display = XOpenDisplay(NULL);
    if (!a->display) {
        fprintf(stderr, "ERROR: Could not open display\n");
        exit(1);
    }

    XWindowAttributes wa = {0};
    XGetWindowAttributes(a->display, DefaultRootWindow(a->display), &wa);
    a->size = (Vec2) {wa.width, wa.height};
}

typedef struct {
    Vec2 pos;
    Vec2 uv;
} Vertex;

void app_open(App *a, const char **paths, size_t count) {
    app_zero(a);
    a->camera = a->final;

    if (!shader_init()) {
        exit(1);
    }

    {
        int  x = 0, y = 0;
        uint mask;

        Window root = DefaultRootWindow(a->display);
        XQueryPointer(a->display, root, &root, &root, &x, &y, &x, &y, &mask);

        a->mouse = (Vec2) {x, y};
        a->select_cursor = XCreateFontCursor(a->display, XC_crosshair);
    }

    if (count) {
        for (size_t i = 0; i < count; i++) {
            app_load_path(a, paths[i]);
        }
    } else {
        const Image image = {
            .data = app_snap(a, (Vec2) {0}, a->size),
            .width = a->size.x,
            .height = a->size.y,
        };
        da_append(&a->images, image);
    }

    GLint        glx_attribs[] = {GLX_RGBA, GLX_DOUBLEBUFFER, GLX_DEPTH_SIZE, 24, None};
    XVisualInfo *vi = glXChooseVisual(a->display, 0, glx_attribs);
    if (!vi) {
        fprintf(stderr, "ERROR: No appropriate visual found\n");
        exit(1);
    }

    const Window root = DefaultRootWindow(a->display);

    XSetWindowAttributes wa;
    wa.colormap = XCreateColormap(a->display, root, vi->visual, AllocNone);
    wa.event_mask = ButtonPressMask | ButtonReleaseMask | KeyPressMask | PointerMotionMask |
                    ExposureMask | VisibilityChangeMask | FocusChangeMask;

    wa.override_redirect = true;
    wa.save_under = true;

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

    XMapRaised(a->display, a->window);
    XGrabKeyboard(a->display, root, true, GrabModeAsync, GrabModeAsync, CurrentTime);
    XGrabPointer(
        a->display,
        a->window,
        true,
        0,
        GrabModeAsync,
        GrabModeAsync,
        None,
        a->select_on ? a->select_cursor : None,
        CurrentTime);

    XGetInputFocus(a->display, &a->revert_window, &a->revert_return);
    XSetInputFocus(a->display, a->window, RevertToParent, CurrentTime);
    XSelectInput(a->display, root, SubstructureNotifyMask);

    a->image_program = compile_program(image_vs, image_fs);
    a->image_uniform_zoom = get_uniform(a->image_program, "zoom");
    a->image_uniform_offset = get_uniform(a->image_program, "offset");

    a->overlay_program = compile_program(overlay_vs, overlay_fs);
    a->overlay_uniform_mouse = get_uniform(a->overlay_program, "mouse");
    a->overlay_uniform_aspect = get_uniform(a->overlay_program, "aspect");

    a->overlay_uniform_lens_size = get_uniform(a->overlay_program, "lens_size");
    a->overlay_uniform_lens_color = get_uniform(a->overlay_program, "lens_color");

    a->overlay_uniform_select_began = get_uniform(a->overlay_program, "select_began");
    a->overlay_uniform_select_mouse = get_uniform(a->overlay_program, "select_mouse");
    a->overlay_uniform_select_start = get_uniform(a->overlay_program, "select_start");

    glUseProgram(a->overlay_program);
    glUniform4f(get_uniform(a->overlay_program, "select_color"), SELECTION_COLOR);

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

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, uv));
    glEnableVertexAttribArray(1);

    glGenTextures(1, &a->texture);
    glBindTexture(GL_TEXTURE_2D, a->texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    app_load_image(a, true);
}

void app_draw(App *a) {
    glClearColor(BACKGROUND_COLOR);
    glClear(GL_COLOR_BUFFER_BIT);

    {
        glUseProgram(a->image_program);
        glUniform1f(a->image_uniform_zoom, a->camera.zoom);
        glUniform2f(
            a->image_uniform_offset,
            2.0 * a->camera.offset.x / a->size.x - 1.0,
            1.0 - 2.0 * a->camera.offset.y / a->size.y);

        glBindTexture(GL_TEXTURE_2D, a->texture);
        glBindVertexArray(a->vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    {
        glUseProgram(a->overlay_program);

        glUniform1f(a->overlay_uniform_lens_size, a->camera.lens_size);
        glUniform4f(
            a->overlay_uniform_lens_color,
            a->camera.lens_color.x,
            a->camera.lens_color.y,
            a->camera.lens_color.z,
            a->camera.lens_color.w);

        glUniform1f(a->overlay_uniform_aspect, a->size.x / a->size.y);
        glUniform1i(a->overlay_uniform_select_began, a->select_began);

        if (a->select_on || a->select_snap_pending) {
            glUniform2f(
                a->overlay_uniform_select_mouse, a->mouse.x / a->size.x, a->mouse.y / a->size.y);
        } else {
            glUniform2f(a->overlay_uniform_mouse, a->mouse.x / a->size.x, a->mouse.y / a->size.y);
        }

        glUniform2f(
            a->overlay_uniform_select_start,
            a->select_start.x / a->size.x,
            a->select_start.y / a->size.y);

        glBindVertexArray(a->vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    glXSwapBuffers(a->display, a->window);
}

void app_loop(App *a) {
    double pt = get_time();
    while (true) {
        const double dt = get_time() - pt;
        if (!a->select_snap_pending) camera_update(&a->camera, &a->final, dt);
        pt += dt;

        app_draw(a);
        if (a->select_snap_pending) {
            a->select_snap_pending--;
            if (!a->select_snap_pending) {
                const Vec2 start = {
                    min(a->select_start.x, a->mouse.x),
                    min(a->select_start.y, a->mouse.y),
                };

                const Vec2 size = vec2_sub(
                    (Vec2) {
                        max(a->select_start.x, a->mouse.x),
                        max(a->select_start.y, a->mouse.y),
                    },
                    start);

                if (size.x != 0 && size.y != 0) {
                    app_save_image(a, start, size);
                    if (a->select_exit) return;
                }
            }
        }

        while (XPending(a->display)) {
            XEvent e;
            XNextEvent(a->display, &e);
            if (e.type != Expose && a->select_snap_pending) continue;

            switch (e.type) {
            case Expose:
                app_draw(a);
                break;

            case FocusOut:
                XSetInputFocus(a->display, a->window, RevertToParent, CurrentTime);
                break;

            case VisibilityNotify:
                if (((XVisibilityEvent *) &e)->state != VisibilityUnobscured) {
                    XRaiseWindow(a->display, a->window);
                }
                break;

            case ConfigureNotify:
                if (((XConfigureEvent *) &e)->window != a->window) {
                    XRaiseWindow(a->display, a->window);
                }
                break;

            case ButtonPress:
                switch (e.xbutton.button) {
                case Button1:
                    a->mouse = (Vec2) {e.xbutton.x, e.xbutton.y};
                    if (a->select_on) {
                        a->select_start = a->mouse;
                        a->select_began = true;
                    } else {
                        a->dragging = true;
                    }
                    break;

                case Button2:
                    return;

                case Button3:
                    if (!a->select_on) {
                        a->focus = !a->focus;
                        if (a->focus) {
                            a->final.lens_color = (Vec4) {FLASHLIGHT_COLOR};
                        } else {
                            a->final.lens_color = (Vec4) {0};
                        }
                    }
                    break;

                case Button4:
                    if (!a->select_on) {
                        if (a->focus) {
                            a->final.lens_size *= LENS_FACTOR;
                        } else {
                            app_zoom(a, ZOOM_FACTOR);
                        }
                    }
                    break;

                case Button5:
                    if (!a->select_on) {
                        if (a->focus) {
                            a->final.lens_size /= LENS_FACTOR;
                        } else {
                            app_zoom(a, 1.0 / ZOOM_FACTOR);
                        }
                    }
                    break;
                }
                break;

            case ButtonRelease:
                if (e.xbutton.button == Button1) {
                    if (a->select_on) {
                        a->select_on = false;
                        a->select_began = false;

                        XUngrabPointer(a->display, CurrentTime);
                        XGrabPointer(
                            a->display,
                            a->window,
                            true,
                            0,
                            GrabModeAsync,
                            GrabModeAsync,
                            None,
                            None,
                            CurrentTime);

                        a->select_snap_pending = SELECTION_PENDING_FRAMES_SKIP;
                    } else {
                        a->dragging = false;
                    }
                }
                break;

            case MotionNotify: {
                const Vec2 pos = (Vec2) {e.xmotion.x, e.xmotion.y};
                if (a->dragging) {
                    a->final.offset = vec2_add(a->final.offset, vec2_sub(pos, a->mouse));
                    a->camera.offset = a->final.offset;
                }
                a->mouse = pos;
            } break;

            case KeyPress:
                switch (XLookupKeysym(&e.xkey, 0)) {
                case 'q':
                    return;

                case 's':
                    app_screenshot(a);
                    break;

                case '0':
                    app_zero(a);
                    break;

                case 'n':
                case 'j':
                    a->current++;
                    if (a->current >= a->images.count) {
                        a->current = 0;
                    }
                    app_load_image(a, true);
                    break;

                case 'p':
                case 'k':
                    if (a->current) {
                        a->current--;
                    } else {
                        a->current = a->images.count - 1;
                    }
                    app_load_image(a, false);
                    break;

                case 'w': {
                    XImage *wallpaper = app_snap_ximage(a, (Vec2) {0}, a->size);
                    if (a->wallpaper) {
                        XDestroyImage(a->wallpaper);
                    }
                    a->wallpaper = wallpaper;
                } break;

                case 'r':
                    a->select_on = !a->select_on;
                    XUngrabPointer(a->display, CurrentTime);
                    XGrabPointer(
                        a->display,
                        a->window,
                        true,
                        0,
                        GrabModeAsync,
                        GrabModeAsync,
                        None,
                        a->select_on ? a->select_cursor : None,
                        CurrentTime);

                    if (!a->select_on) {
                        a->select_began = false;
                    }
                    break;

                case XK_Escape:
                    if (a->select_on) {
                        a->select_on = false;
                        a->select_began = false;

                        XUngrabPointer(a->display, CurrentTime);
                        XGrabPointer(
                            a->display,
                            a->window,
                            true,
                            0,
                            GrabModeAsync,
                            GrabModeAsync,
                            None,
                            None,
                            CurrentTime);
                    }
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
    glDeleteProgram(a->image_program);
    glDeleteProgram(a->overlay_program);
    glDeleteTextures(1, &a->texture);

    glXMakeCurrent(a->display, None, NULL);
    glXDestroyContext(a->display, a->glx_context);

    XSetInputFocus(a->display, a->revert_window, a->revert_return, CurrentTime);
    XUngrabKeyboard(a->display, CurrentTime);
    XUngrabPointer(a->display, CurrentTime);
    XDestroyWindow(a->display, a->window);
    XFreeCursor(a->display, a->select_cursor);

    app_wallpaper(a);
    XCloseDisplay(a->display);

    for (size_t i = 0; i < a->images.count; i++) {
        free(a->images.data[i].data);
    }
    da_free(&a->images);
    da_free(&a->paths);

    shader_free();
}

void app_wallpaper(App *a) {
    if (!a->wallpaper) {
        return;
    }

    // Because X11 is a pile of dogshit, the OFFICIAL way to set the wallpaper is to create a Pixmap
    // and then leak the resources. Yes. I'm not kidding.
    Display *display = XOpenDisplay(NULL);

    const Window root = DefaultRootWindow(display);
    const size_t width = a->size.x;
    const size_t height = a->size.y;

    Pixmap pixmap = XCreatePixmap(display, root, width, height, a->wallpaper->depth);
    GC     gc = XCreateGC(display, root, 0, NULL);

    XPutImage(display, pixmap, gc, a->wallpaper, 0, 0, 0, 0, width, height);

    {
        int  screen = DefaultScreen(display);
        Atom atom_root = XInternAtom(display, "_XROOTMAP_ID", true);
        Atom atom_eroot = XInternAtom(display, "ESETROOT_PMAP_ID", true);

        if (atom_root != None && atom_eroot != None) {
            Atom           type;
            int            format;
            unsigned long  length, after;
            unsigned char *data_root, *data_eroot;

            XGetWindowProperty(
                display,
                root,
                atom_root,
                0L,
                1L,
                false,
                AnyPropertyType,
                &type,
                &format,
                &length,
                &after,
                &data_root);

            if (type == XA_PIXMAP) {
                XGetWindowProperty(
                    display,
                    root,
                    atom_eroot,
                    0L,
                    1L,
                    false,
                    AnyPropertyType,
                    &type,
                    &format,
                    &length,
                    &after,
                    &data_eroot);

                if (data_root && data_eroot && type == XA_PIXMAP &&
                    *((Pixmap *) data_root) == *((Pixmap *) data_eroot))
                    XKillClient(display, *((Pixmap *) data_root));
            }
        }

        atom_root = XInternAtom(display, "_XROOTPMAP_ID", false);
        atom_eroot = XInternAtom(display, "ESETROOT_PMAP_ID", false);
        if (atom_root != None && atom_eroot != None) {
            XChangeProperty(
                display,
                root,
                atom_root,
                XA_PIXMAP,
                32,
                PropModeReplace,
                (unsigned char *) &pixmap,
                1);

            XChangeProperty(
                display,
                root,
                atom_eroot,
                XA_PIXMAP,
                32,
                PropModeReplace,
                (unsigned char *) &pixmap,
                1);
        }
    }

    XSetWindowBackgroundPixmap(display, root, pixmap);
    XClearWindow(display, root);
    XFlush(display);
    XSync(display, false);
    XKillClient(display, AllTemporary);
    XSetCloseDownMode(display, RetainTemporary);
    XDestroyImage(a->wallpaper);
    XFreeGC(display, gc);
    XCloseDisplay(display);

    a->wallpaper = NULL;
}

void app_screenshot(App *a) {
    app_save_image(a, (Vec2) {0}, a->size);
}
