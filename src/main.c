#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <unistd.h>
#include <jpeglib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrender.h>

#define SHOT_PATH_FMT "shot-%zu.jpeg"

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
    Vec2D pos;
    double zoom;
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

    if (view->pos.x != 0 || view->pos.y != 0) {
        XRenderColor back = {0};
        XRenderFillRectangle(display, PictOpSrc, buffer_picture, &back, 0, 0, width, height);
    }

    XRenderComposite(display, PictOpOver, pixmap_picture, 0, buffer_picture, 0, 0, 0, 0, view->pos.x, view->pos.y, width - view->pos.x, height - view->pos.y);
    XRenderComposite(display, PictOpSrc, buffer_picture, 0, window_picture, 0, 0, 0, 0, 0, 0, width, height);
}

void save_screen(Display *display, Window root, size_t *image_count)
{
    XImage *current = snap_screen(display, root);
    save_image(current, image_count);
    XDestroyImage(current);
}

int main(void)
{
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
            ButtonPressMask | ButtonReleaseMask | ButtonMotionMask | KeyPressMask);

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

    const double x_scale = snap->width / window_attr.width;
    const double y_scale = snap->height / window_attr.height;

    View view = {0};

    view.transform.matrix[0][0] = XDoubleToFixed(x_scale);
    view.transform.matrix[1][1] = XDoubleToFixed(y_scale);
    view.zoom = 1.0;

    bool view_changed = true;
    bool mouse_left_down = false;
    Vec2D mouse_drag_start = {0};
    Vec2D view_pos_start = {0};

    XEvent event;
    bool running = true;

    size_t image_count = 0;
    while (running) {
        if (view_changed) {
            render_pixmap(display, window_attr.width, window_attr.height, pixmap_picture, window_picture, buffer_picture, &view);
            view_changed = false;
        }

        XNextEvent(display, &event);

        if (event.type == KeyPress) {
            switch (XLookupKeysym(&event.xkey, 0)) {
                case 'q':
                    running = false;
                    break;
                case 's':
                    save_screen(display, root, &image_count);
                    break;
            }
        } else if (event.type == ButtonPress) {
            switch (event.xbutton.button) {
                case Button4:
                    view.zoom += 0.05;
                    view_changed = true;
                    break;

                case Button5:
                    view.zoom -= 0.05;
                    view_changed = true;
                    break;

                case Button1:
                    mouse_drag_start = vec2d(event.xbutton.x, event.xbutton.y);
                    view_pos_start = view.pos;
                    mouse_left_down = true;
                    break;
            }
        } else if (event.type == ButtonRelease) {
            switch (event.xbutton.button) {
                case Button1:
                    mouse_left_down = false;
                    break;
            }
        } else if (event.type == MotionNotify) {
            if (mouse_left_down) {
                Vec2D dm = vec2d_sub(vec2d(event.xmotion.x, event.xmotion.y), mouse_drag_start);
                view.pos = vec2d_add(view_pos_start, dm);
                view_changed = true;
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
