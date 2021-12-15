#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <unistd.h>
#include <jpeglib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrender.h>

#define SHOT_PATH_FMT "shot-%zu.jpeg"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

void save_image(XImage *image, size_t *image_count)
{
    const size_t length = snprintf(NULL, 0, SHOT_PATH_FMT, *image_count) + 1;
    char file_path[length];
    snprintf(file_path, length, SHOT_PATH_FMT, *image_count);

    FILE* f = fopen(file_path, "wb");
    if (f == NULL) return;

    char *buffer = malloc(3 * image->width * image->height);
    for (int y = 0; y < image->height; y++) {
        for (int x = 0; x < image->width; x++) {
            const size_t pixel = XGetPixel(image, x, y);
            buffer[y * image->width * 3 + x * 3 + 0] = (char) (pixel >> 16);
            buffer[y * image->width * 3 + x * 3 + 1] = (char) ((pixel & 0x00ff00) >> 8);
            buffer[y * image->width * 3 + x * 3 + 2] = (char) (pixel & 0x0000ff);
        }
    }

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, f);

    cinfo.image_width = image->width;
    cinfo.image_height = image->height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 85, TRUE);
    jpeg_start_compress(&cinfo, TRUE);

    const size_t stride = (image->depth >> 3) * image->width;
    while (cinfo.next_scanline < cinfo.image_height) {
        JSAMPROW row_pointer = (JSAMPROW) &buffer[cinfo.next_scanline * stride];
        jpeg_write_scanlines(&cinfo, &row_pointer, 1);
    }

    free(buffer);
    jpeg_finish_compress(&cinfo);
    fclose(f);

    *image_count += 1;
}

XImage *snap_screen(Display *display, Window root)
{
    XWindowAttributes attr = {0};
    XGetWindowAttributes(display, root, &attr);
    return XGetImage(display, root, 0, 0, attr.width, attr.height, AllPlanes, ZPixmap);
}

typedef struct {
    int x, y;
} Vec2D;

Vec2D vec2d(int x, int y)
{
    return (Vec2D) {.x = x, .y = y};
}

Vec2D vec2d_add(Vec2D a, Vec2D b)
{
    return (Vec2D) {.x = a.x + b.x, .y = a.y + b.y};
}

Vec2D vec2d_sub(Vec2D a, Vec2D b)
{
    return (Vec2D) {.x = a.x - b.x, .y = a.y - b.y};
}

Vec2D vec2d_div(Vec2D a, Vec2D b)
{
    return (Vec2D) {.x = a.x / b.x, .y = a.y / b.y};
}

typedef struct {
    XTransform transform;
    Vec2D move_offset;

    double zoom;
    Vec2D zoom_offset;

    bool lens_mode;
    size_t lens_size;

    Vec2D mouse;
} View;

void render_pixmap(Display *display,
        size_t width,
        size_t height,
        Picture pixmap_picture,
        Picture window_picture,
        Picture buffer_picture,
        View *view)
{
    view->transform.matrix[2][2] = XDoubleToFixed(view->zoom);
    XRenderSetPictureTransform(display, pixmap_picture, &view->transform);

    Vec2D offset = vec2d_add(view->move_offset, view->zoom_offset);
    if (offset.x != 0 || offset.y != 0) {
        XRenderColor back = {0};
        XRenderFillRectangle(display, PictOpSrc, buffer_picture, &back, 0, 0, width, height);
    }

    XRenderComposite(display, PictOpOver, pixmap_picture, 0, buffer_picture, 0, 0, 0, 0, offset.x, offset.y, width - offset.x, height - offset.y);

    if (view->lens_mode) {
        XRenderColor focus = {0};

        Vec2D start = {
            .x = view->mouse.x - view->lens_size,
            .y = view->mouse.y - view->lens_size
        };

        Vec2D end = {
            .x = view->mouse.x + view->lens_size,
            .y = view->mouse.y + view->lens_size
        };

        start.x = MAX(start.x, 0);
        start.y = MAX(start.y, 0);

        XRenderFillRectangle(display, PictOpSrc, buffer_picture, &focus,
                0, 0, start.x, height);

        XRenderFillRectangle(display, PictOpSrc, buffer_picture, &focus,
                end.x, 0, width - start.x, height);

        XRenderFillRectangle(display, PictOpSrc, buffer_picture, &focus,
                start.x, 0, end.x, start.y);

        XRenderFillRectangle(display, PictOpSrc, buffer_picture, &focus,
                start.x, end.y, end.x, height - start.y);

        for (int y = start.y; y < end.y; ++y) {
            for (int x = start.x; x < end.x; ++x) {
                const int dx = x - view->mouse.x;
                const int dy = y - view->mouse.y;
                const size_t length = dx * dx + dy * dy;

                if (length > view->lens_size * view->lens_size) {
                    XRenderFillRectangle(display, PictOpSrc, buffer_picture, &focus,
                            x, y, 1, 1);
                }
            }
        }
    }

    XRenderComposite(display, PictOpSrc, buffer_picture, 0, window_picture, 0, 0, 0, 0, 0, 0, width, height);
}

void save_screen(Display *display, Window root, size_t *image_count)
{
    XImage *current = snap_screen(display, root);
    save_image(current, image_count);
    XDestroyImage(current);
}

void usage(FILE *stream)
{
    fprintf(stream, "USAGE:\n");
    fprintf(stream, "  thono [FLAGS]\n");
    fprintf(stream, "FLAGS:\n");
    fprintf(stream, "  -help         Display this help message\n");
    fprintf(stream, "  -help-ui      Display help message about the UI\n");
    fprintf(stream, "  -zoom N       Set the magnification zoom factor to N (Default: 0.1)\n");
    fprintf(stream, "  -lens-zoom N  Set the lens size zoom factor to N (Default: 50)\n");
}

int main(int argc, char **argv)
{
    double zoom_factor = 0.1;
    size_t lens_zoom_factor = 50;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-zoom") == 0) {
            i++;

            if (i == argc) {
                usage(stderr);
                fprintf(stderr, "ERROR: zoom factor not provided\n");
                exit(1);
            }

            char *endp = NULL;
            zoom_factor = strtod(argv[i], &endp);

            if (endp != argv[i] + strlen(argv[i])) {
                usage(stderr);
                fprintf(stderr, "ERROR: invalid zoom factor `%s`. Expected number\n", argv[i]);
                exit(1);
            }
        } else if (strcmp(argv[i], "-lens-zoom") == 0) {
            i++;

            if (i == argc) {
                usage(stderr);
                fprintf(stderr, "ERROR: lens zoom factor not provided\n");
                exit(1);
            }

            char *endp = NULL;
            lens_zoom_factor = strtoul(argv[i], &endp, 10);

            if (endp != argv[i] + strlen(argv[i])) {
                usage(stderr);
                fprintf(stderr, "ERROR: invalid lens zoom factor `%s`. Expected integer\n", argv[i]);
                exit(1);
            }
        } else if (strcmp(argv[i], "-help") == 0) {
            usage(stdout);
            exit(0);
        } else if (strcmp(argv[i], "-help-ui") == 0) {
            printf("Action           Description\n");
            printf("----------------------------\n");
            printf("`q`              Quit\n");
            printf("`s`              Screenshot\n");
            printf("`s`              Screenshot\n");
            printf("Right Click      Toggle Lens Mode\n");
            printf("Scroll Up        Zoom in (If in Lens Mode, increase the lens size) \n");
            printf("Scroll Down      Zoom out (If in Lens Mode, decrease the lens size)\n");
            printf("Left Click Drag  Drag the zoom view\n");
            exit(0);
        } else {
            usage(stderr);
            fprintf(stderr, "ERROR: unknown flag `%s`\n", argv[i]);
            exit(1);
        }
    }

    Display *display = XOpenDisplay(NULL);
    Window root = RootWindow(display, DefaultScreen(display));
    XImage *snap = snap_screen(display, root);

    Window window = XCreateSimpleWindow(display, root, 0, 0, 800, 600, 0, 0, 0);

    Atom wm_state = XInternAtom(display, "_NET_WM_STATE", true);
    Atom wm_fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", true);

    XChangeProperty(display, window, wm_state, XA_ATOM, 32,
            PropModeReplace, (unsigned char *) &wm_fullscreen, 1);

    XMapWindow(display, window);
    XSelectInput(display, window,
            ButtonPressMask | ButtonReleaseMask | ButtonMotionMask | PointerMotionMask | KeyPressMask);

    Pixmap pixmap = XCreatePixmap(display, window, snap->width, snap->height, snap->depth);
    XPutImage(display, pixmap, DefaultGC(display, 0), snap, 0, 0, 0, 0, snap->width, snap->height);

    XRenderPictureAttributes picture_attr = {0};
    XRenderPictFormat *picture_format = XRenderFindStandardFormat(display, PictStandardRGB24);

    Pixmap buffer = XCreatePixmap(display, window, snap->width, snap->height, snap->depth);
    Picture buffer_picture = XRenderCreatePicture(display, buffer, picture_format, 0, &picture_attr);

    Picture pixmap_picture = XRenderCreatePicture(display, pixmap, picture_format, 0, &picture_attr);
    Picture window_picture = XRenderCreatePicture(display, window, picture_format, 0, &picture_attr);

    XWindowAttributes window_attr = {0};
    XGetWindowAttributes(display, window, &window_attr);

    View view = {0};
    view.lens_size = 50;

    view.transform.matrix[0][0] = XDoubleToFixed(snap->width / window_attr.width);
    view.transform.matrix[1][1] = XDoubleToFixed(snap->height / window_attr.height);
    view.zoom = 1.0;

    bool view_changed = true;
    bool mouse_left_down = false;
    Vec2D mouse_drag_start = {0};
    Vec2D view_move_start = {0};

    XEvent event;
    bool running = true;

    size_t image_count = 0;
    while (running) {
        if (view_changed) {
            render_pixmap(display, window_attr.width, window_attr.height, pixmap_picture, window_picture, buffer_picture, &view);
            view_changed = false;
        }

        while (XPending(display) > 0) {
            XNextEvent(display, &event);

            switch (event.type) {
                case KeyPress:
                    switch (XLookupKeysym(&event.xkey, 0)) {
                        case 'q':
                            running = false;
                            break;
                        case 's':
                            save_screen(display, root, &image_count);
                            break;
                    }
                    break;

                case ButtonPress:
                    switch (event.xbutton.button) {
                        case Button1:
                            mouse_drag_start = vec2d(event.xbutton.x, event.xbutton.y);
                            view_move_start = view.move_offset;
                            mouse_left_down = true;
                            break;

                        case Button4:
                            if (view.lens_mode) {
                                view.lens_size += lens_zoom_factor;
                            } else {
                                view.zoom += zoom_factor;
                                view.zoom_offset.x -= (event.xbutton.x - view.move_offset.x) * zoom_factor;
                                view.zoom_offset.y -= (event.xbutton.y - view.move_offset.y) * zoom_factor;
                            }
                            view_changed = true;
                            break;

                        case Button5:
                            if (view.lens_mode) {
                                view.lens_size -= lens_zoom_factor;
                            } else {
                                view.zoom -= zoom_factor;
                                view.zoom_offset.x += (event.xbutton.x - view.move_offset.x) * zoom_factor;
                                view.zoom_offset.y += (event.xbutton.y - view.move_offset.y) * zoom_factor;
                            }
                            view_changed = true;
                            break;
                    }
                    break;

                case ButtonRelease:
                    switch (event.xbutton.button) {
                        case Button1:
                            view_move_start = view.move_offset;
                            mouse_left_down = false;
                            break;

                        case Button3:
                            view.mouse.x = event.xmotion.x;
                            view.mouse.y = event.xmotion.y;

                            view.lens_mode = !view.lens_mode;
                            view_changed = true;
                            break;
                    }
                    break;

                case MotionNotify:
                    view.mouse.x = event.xmotion.x;
                    view.mouse.y = event.xmotion.y;

                    if (mouse_left_down) {
                        Vec2D dm = vec2d_sub(view.mouse, mouse_drag_start);
                        view.move_offset = vec2d_add(view_move_start, dm);
                        view_changed = true;
                    }

                    if (view.lens_mode) {
                        view_changed = true;
                    }
                    break;
            }
        }
    }

    XRenderFreePicture(display, pixmap_picture);
    XRenderFreePicture(display, buffer_picture);
    XRenderFreePicture(display, window_picture);

    XFreePixmap(display, pixmap);
    XFreePixmap(display, buffer);

    XDestroyImage(snap);
    XCloseDisplay(display);
    return 0;
}
