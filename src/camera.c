#include "camera.h"
#include "config.h"

Vec2 camera_world(const Camera *c, Vec2 v) {
    return vec2_scale(vec2_sub(v, c->offset), 1.0 / c->zoom);
}

void camera_update(Camera *c, const Camera *final, float dt) {
    const float ds = THONO_SPEED * dt;
    c->lens += (final->lens - c->lens) * ds;
    c->zoom += (final->zoom - c->zoom) * ds;
    c->flash = vec4_add(c->flash, vec4_scale(vec4_sub(final->flash, c->flash), ds));
    c->offset = vec2_add(c->offset, vec2_scale(vec2_sub(final->offset, c->offset), ds));
}
