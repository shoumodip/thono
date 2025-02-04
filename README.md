# Thono
Image Utility in C

## Quick Start
Depends on Xlib and OpenGL

```console
$ ./release.sh
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
$ ./thono -s 5
```

A region of the screen can also be selected

```console
$ ./thono -r
$ ./thono -r 5 # With optional delay
```

A screenshot can also be taken in "normal mode"

| Action          | Description                                         |
| --------------- | --------------------------------------------------- |
| `s`             | Take a screenshot                                   |
| `r`             | Select a region and take a screenshot               |

Press `r` or `Escape` to get out of "selection mode"

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

Thono can load entire directories as well. If you want to open a directory
recursively, pass the `-R` flag as the first argument before the directory
paths

```console
$ ./thono -R [PATHS]...
```

## Wallpaper Setter Utility
Thono can be used as a simple wallpaper setter utility

```console
$ ./thono -w <image>
```

The wallpaper can also be set in "normal mode"

| Action          | Description                                         |
| --------------- | --------------------------------------------------- |
| `w`             | Take screenshot and set wallpaper                   |

The wallpaper can also be restored later if the `-W` (capital) flag is used

```console
$ ./thono -W <image>
Created wallpaper restore script '/home/<user>/.local/share/wallpaper'
$ ~/.local/share/wallpaper
```

The path of the restore script can be customized from the CLI

```console
$ ./thono -W <image> ~/.thonobg
Created wallpaper restore script '/home/<user>/.thonobg'
$ ~/.thonobg
```
