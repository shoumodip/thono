#include "la.h"

Vec2 vec2_add(Vec2 a, Vec2 b) {
    a.x += b.x;
    a.y += b.y;
    return a;
}

Vec2 vec2_sub(Vec2 a, Vec2 b) {
    a.x -= b.x;
    a.y -= b.y;
    return a;
}

Vec2 vec2_div(Vec2 a, Vec2 b) {
    a.x /= b.x;
    a.y /= b.y;
    return a;
}

Vec2 vec2_scale(Vec2 a, float b) {
    a.x *= b;
    a.y *= b;
    return a;
}

Vec4 vec4_add(Vec4 a, Vec4 b) {
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    a.w += b.w;
    return a;
}

Vec4 vec4_sub(Vec4 a, Vec4 b) {
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    a.w -= b.w;
    return a;
}

Vec4 vec4_mul(Vec4 a, Vec4 b) {
    a.x *= b.x;
    a.y *= b.y;
    a.z *= b.z;
    a.w *= b.w;
    return a;
}

Vec4 vec4_scale(Vec4 a, float b) {
    a.x *= b;
    a.y *= b;
    a.z *= b;
    a.w *= b;
    return a;
}
