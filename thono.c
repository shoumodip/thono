#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrender.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

// Vector
typedef struct {
    long x;
    long y;
} Vec;

Vec vec_new(long x, long y) {
    return (Vec) {.x = x, .y = y};
}

Vec vec_add(Vec a, Vec b) {
    return (Vec) {.x = a.x + b.x, .y = a.y + b.y};
}

Vec vec_sub(Vec a, Vec b) {
    return (Vec) {.x = a.x - b.x, .y = a.y - b.y};
}

// App
typedef struct {
    Display *display;
    XImage *image;
    Window window;
    Vec size;

    Picture source;
    Picture buffer;
    Picture output;
    Pixmap pixmaps[2];

    Vec drag;
    Vec mouse;
    Vec start;
    Vec origin;
    XTransform transform;

    long lens;
    double zoom;
    bool clicked;
    bool focused;
    bool running;
} App;

void app_init(App *app) {
    app->display = XOpenDisplay(NULL);
    if (!app->display) {
        fprintf(stderr, "error: could not open display\n");
        exit(1);
    }

    Window root = DefaultRootWindow(app->display);
    XWindowAttributes attr = {0};
    XGetWindowAttributes(app->display, root, &attr);
    app->size = vec_new(attr.width, attr.height);

    app->image = XGetImage(app->display, root, 0, 0, app->size.x, app->size.y, AllPlanes, ZPixmap);
    if (!app->image) {
        fprintf(stderr, "error: could not capture screenshot\n");
        exit(1);
    }

    GC gc = DefaultGC(app->display, DefaultScreen(app->display));
    app->window = XCreateSimpleWindow(app->display, root, 0, 0, app->size.x, app->size.y, 0, 0, 0);

    Atom window_state = XInternAtom(app->display, "_NET_WM_STATE", true);
    Atom window_fullscreen = XInternAtom(app->display, "_NET_WM_STATE_FULLSCREEN", true);
    XChangeProperty(
        app->display, app->window, window_state, XA_ATOM, 32,
        PropModeReplace, (unsigned char *) &window_fullscreen, 1);

    XMapWindow(app->display, app->window);
    XSelectInput(app->display, app->window, ButtonPressMask | ButtonReleaseMask | PointerMotionMask | KeyPressMask | ExposureMask);

    XRenderPictFormat *format = XRenderFindStandardFormat(app->display, PictStandardRGB24);
    if (!format) {
        fprintf(stderr, "error: could not find standard rendering format\n");
        exit(1);
    }

    app->pixmaps[0] = XCreatePixmap(app->display, app->window, app->size.x, app->size.y, app->image->depth);
    app->pixmaps[1] = XCreatePixmap(app->display, app->window, app->size.x, app->size.y, app->image->depth);
    XPutImage(app->display, app->pixmaps[0], gc, app->image, 0, 0, 0, 0, app->size.x, app->size.y);

    app->source = XRenderCreatePicture(app->display, app->pixmaps[0], format, 0, NULL);
    app->buffer = XRenderCreatePicture(app->display, app->pixmaps[1], format, 0, NULL);
    app->output = XRenderCreatePicture(app->display, app->window, format, 0, NULL);

    app->transform.matrix[0][0] = XDoubleToFixed(1.0);
    app->transform.matrix[1][1] = XDoubleToFixed(1.0);
    app->transform.matrix[2][2] = XDoubleToFixed(1.0);

    app->lens = 100;
    app->zoom = 1.0;
    app->running = true;
}

void app_exit(App *app) {
    XFreePixmap(app->display, app->pixmaps[0]);
    XFreePixmap(app->display, app->pixmaps[1]);

    XRenderFreePicture(app->display, app->source);
    XRenderFreePicture(app->display, app->buffer);
    XRenderFreePicture(app->display, app->output);

    XDestroyImage(app->image);
    XDestroyWindow(app->display, app->window);
    XCloseDisplay(app->display);
}

void app_zoom(App *app, double factor) {
    double dx = (app->mouse.x - app->origin.x) / app->zoom;
    double dy = (app->mouse.y - app->origin.y) / app->zoom;

    app->zoom += factor;
    app->transform.matrix[2][2] = XDoubleToFixed(app->zoom);

    app->origin.x = app->mouse.x - dx * app->zoom;
    app->origin.y = app->mouse.y - dy * app->zoom;
}

void app_draw(App *app) {
    XRenderColor black = {
        .red = UINT16_MAX * 0.09375,
        .green = UINT16_MAX * 0.09375,
        .blue = UINT16_MAX * 0.09375,
        .alpha = UINT16_MAX
    };
    XRenderFillRectangle(app->display, PictOpSrc, app->buffer, &black, 0, 0, app->size.x, app->size.y);

    XRenderSetPictureTransform(app->display, app->source, &app->transform);
    XRenderComposite(
        app->display, PictOpOver, app->source, 0, app->buffer, 0, 0, 0, 0,
        app->origin.x, app->origin.y, app->size.x - app->origin.x, app->size.y - app->origin.y);

    if (app->focused) {
        XRenderColor focus = {0};
        focus.alpha = UINT16_MAX * 0.8;

        Vec start = {
            .x = MAX(app->mouse.x - app->lens, 0),
            .y = MAX(app->mouse.y - app->lens, 0),
        };

        Vec end = {
            .x = app->mouse.x + app->lens,
            .y = app->mouse.y + app->lens
        };

        XRenderFillRectangle(app->display, PictOpOver, app->buffer, &focus,
                             0, 0, start.x, app->size.y);

        XRenderFillRectangle(app->display, PictOpOver, app->buffer, &focus,
                             end.x, 0, app->size.x - start.x, app->size.y);

        XRenderFillRectangle(app->display, PictOpOver, app->buffer, &focus,
                             start.x, 0, end.x, start.y);

        XRenderFillRectangle(app->display, PictOpOver, app->buffer, &focus,
                             start.x, end.y, end.x, app->size.y - start.y);

        for (long y = start.y; y < end.y; ++y) {
            for (long x = start.x; x < end.x; ++x) {
                long dx = x - app->mouse.x;
                long dy = y - app->mouse.y;
                size_t length = dx * dx + dy * dy;

                if (length > app->lens * app->lens) {
                    XRenderFillRectangle(app->display, PictOpOver, app->buffer, &focus, x, y, 1, 1);
                }
            }
        }
    }

    XRenderComposite(app->display, PictOpSrc, app->buffer, 0, app->output, 0, 0, 0, 0, 0, 0, app->size.x, app->size.y);
}

void app_next(App *app) {
    XEvent event;
    XNextEvent(app->display, &event);
    switch (event.type) {
    case Expose:
        app_draw(app);
        break;

    case KeyPress:
        switch (XLookupKeysym(&event.xkey, 0)) {
        case 'q':
            app->running = false;
            break;

        case '0':
            app->zoom = 1.0;
            app->lens = 100;
            app->origin.x = 0;
            app->origin.y = 0;
            app->focused = false;
            app->transform.matrix[2][2] = XDoubleToFixed(app->zoom);
        }
        break;

    case ButtonPress:
        app->mouse = vec_new(event.xbutton.x, event.xbutton.y);
        switch (event.xbutton.button) {
        case Button1:
            app->drag = app->origin;
            app->start = app->mouse;
            app->clicked = true;
            break;

        case Button2:
            app->running = false;
            break;

        case Button3:
            app->focused = !app->focused;
            break;

        case Button4:
            if (app->focused) {
                app->lens += 20;
            } else {
                app_zoom(app, 0.2);
            }
            break;

        case Button5:
            if (app->focused) {
                if (app->lens >= 20) {
                    app->lens -= 20;
                }
            } else {
                if (app->zoom >= 0.21) {
                    app_zoom(app, -0.2);
                }
            }
            break;
        }
        break;

    case ButtonRelease:
        switch (event.xbutton.button) {
        case Button1:
            app->clicked = false;
            break;
        }
        break;

    case MotionNotify:
        app->mouse = vec_new(event.xmotion.x, event.xmotion.y);
        if (app->clicked) {
            app->origin = vec_add(app->drag, vec_sub(app->mouse, app->start));
        }
        break;
    }
}

// Main
int main(void) {
    App app = {0};
    app_init(&app);

    while (app.running) {
        app_draw(&app);
        app_next(&app);
    }

    app_exit(&app);
}
