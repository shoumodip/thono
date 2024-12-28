# Thono
Zoomer in C

## Quick Start
Depends on Xlib and OpenGL

```console
$ make
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

An optional delay in seconds can be provided

```console
$ ./thono 5
```

## Image Viewer Utility
Thono can be used as a simple image viewer utility

```console
$ ./thono [IMAGES]...
```

| Action          | Description                                         |
| --------------- | --------------------------------------------------- |
| `n`             | Next image                                          |
| `p`             | Previous image                                      |
| `j`             | Next image                                          |
| `k`             | Previous image                                      |
