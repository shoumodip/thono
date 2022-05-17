# Thono
Zoomer in C.

## Quick Start
```console
$ ./build.sh
$ ./thono
```

## Usage
| Action           | Description                                        |
| ---------------- | -------------------------------------------------- |
| `q`              | Quit                                               |
| `s`              | Screenshot                                         |
| `s`              | Screenshot                                         |
| Right Click      | Toggle Lens Mode                                   |
| Scroll Up        | Zoom in (If in Lens Mode, increase the lens size)  |
| Scroll Down      | Zoom out (If in Lens Mode, decrease the lens size) |
| Left Click Drag  | Drag the zoom view                                 |

## Command Line Usage
```console
$ ./thono [FLAGS]
```

### `-snap`
Take a screenshot of the screen and exit.

```console
$ ./thono -snap
```

### `-zoom N`
Set the magnification zoom factor to N.

- Default: `0.1`

```console
$ ./thono -zoom 0.2
```

### `-lens-zoom N`
Set the lens size zoom factor to N.

- Default: `50`

```console
$ ./thono -lens-zoom 69
```

### `-lens-opacity N`
Set the lens opacity to N.

- Default: `0.8`

```console
$ ./thono -lens-opacity 0.7
```

### `-help`
Display the usage information.

```console
$ ./thono -help
```

### `-help-ui`
Display help message about the UI.

```console
$ ./thono -help-ui
```

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
