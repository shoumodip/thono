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

#define IPC_LOCK_FILE    "/tmp/thono.lock"
#define IPC_WINDOW_NAME  "Thono"
#define IPC_MESSAGE_LOAD "THONO_LOAD"

#define IPC_READ_DELAY_MS  100
#define IPC_READ_MAX_TRIES 50

#define IPC_TEMPORARY_FILE "/tmp/thono_lm_XXXXXX"

#define WALLPAPER_RESTORE_PATH_DEFAULT ".local/share/wallpaper"

#define SELECTION_PENDING_FRAMES_SKIP 5

#endif // CONFIG_H
