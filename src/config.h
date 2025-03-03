#ifndef CONFIG_H
#define CONFIG_H

#define FPS         60
#define SPEED       8.0
#define LENS_SIZE   0.1
#define ZOOM_FACTOR 1.1
#define LENS_FACTOR 1.2

#define SELECTION_COLOR  0.6, 0.6, 0.6, 1.0
#define FLASHLIGHT_COLOR 0.0, 0.0, 0.0, 0.8
#define BACKGROUND_COLOR (0x20 / 255.0), (0x20 / 255.0), (0x20 / 255.0), 1.0

#define IPC_LOCK_PATH    "/tmp/thono_ipc_lock"
#define IPC_WINDOW_PATH  "/tmp/thono_ipc_window"
#define IPC_SOCKET_PATH  "/tmp/thono_ipc_socket"
#define IPC_MESSAGE_LOAD "THONO_LOAD"

#define WALLPAPER_RESTORE_PATH_DEFAULT ".local/share/wallpaper"

#define SELECTION_PENDING_FRAMES_SKIP 5

#endif // CONFIG_H
