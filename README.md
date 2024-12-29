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

A screenshot can also be taken in "normal mode"

| Action          | Description                                         |
| --------------- | --------------------------------------------------- |
| `s`             | Take a screenshot                                   |

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

## Wallpaper Setter Utility
Thono can be used as a simple wallpaper setter utility

```console
$ ./thono -w <image>
```

The wallpaper can also be set in "normal mode"

| Action          | Description                                         |
| --------------- | --------------------------------------------------- |
| `w`             | Take screenshot and set wallpaper                   |
