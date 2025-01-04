#ifndef CAMERA_H
#define CAMERA_H

#include "la.h"

typedef struct {
    float lens_size;
    Vec4 lens_color;

    Vec2 offset;
    float zoom;
} Camera;

Vec2 camera_world(const Camera *c, Vec2 v);
void camera_update(Camera *c, const Camera *final, float dt);

#endif // CAMERA_H
