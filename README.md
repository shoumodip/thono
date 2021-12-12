# Thono
Zoomer in C.

## Quick Start
```console
$ make
$ ./thono
```

## Usage
|Action|Description|
|------|-----------|
|`q`|Quit|
|`j` or Scroll Up|Zoom in|
|`k` or Scroll Down|Zoom out|

## Dependencies
- [Xlib](https://www.x.org/releases/current/doc/libX11/libX11/libX11.html)
- [XRender](https://www.x.org/releases/X11R7.7/doc/libXrender/libXrender.txt)
- [LibJPEG](http://libjpeg.sourceforge.net/)

## References
- [Read current X background image to a JPEG file](https://hamberg.no/erlend/posts/2011-01-06-read-current-x-background-to-jpeg.html)
- [Xlib Fullscreen window](https://stackoverflow.com/questions/9065669/x11-glx-fullscreen-mode)
- [Render a Scaled Pixel Buffer with XRender](https://stackoverflow.com/questions/66885643/how-to-render-a-scaled-pixel-buffer-with-xlib-xrender)
- [Xlib Pixmap demo](https://github.com/QMonkey/Xlib-demo/blob/master/src/draw-pixmap.c)
- [Xlib Keypress demo](https://gist.github.com/javiercantero/7753445)
- [Xlib Mouse Wheel demo](https://stackoverflow.com/questions/39296311/how-to-react-to-mouse-wheel-in-xlib)
