#ifndef THONO_CAMERA_H
#define THONO_CAMERA_H

#include "la.h"

typedef struct {
    Vec4 flash;
    Vec2 offset;
    float lens;
    float zoom;
} Camera;

Vec2 camera_world(const Camera *c, Vec2 v);
void camera_update(Camera *c, const Camera *final, float dt);

#endif // THONO_CAMERA_H
