# Thono
Zoomer in C

## Quick Start
Depends on Xlib and OpenGL

```console
$ ./build.sh
$ ./thono
```

## Usage
| Action          | Description                                         |
| --------------- | --------------------------------------------------- |
| `q`             | Quit                                                |
| `s`             | Take a screenshot                                   |
| `0`             | Reset the view                                      |
| Scroll Up       | Zoom in (If in Focus Mode, increase the lens size)  |
| Scroll Down     | Zoom out (If in Focus Mode, decrease the lens size) |
| Right Click     | Toggle Focus Mode                                   |
| Middle Click    | Quit                                                |
| Left Click Drag | Drag the zoom view                                  |

## Screenshot Utility
Thono can be used as a simple screenshot utility

```console
$ ./thono -s
```

## Image Viewer Utility
Thono can be used as a simple image viewer utility

```console
$ ./thono <path-to-image>
```
