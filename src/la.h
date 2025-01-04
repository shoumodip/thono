#ifndef LA_H
#define LA_H

typedef struct {
    float x;
    float y;
} Vec2;

Vec2 vec2_add(Vec2 a, Vec2 b);
Vec2 vec2_sub(Vec2 a, Vec2 b);
Vec2 vec2_div(Vec2 a, Vec2 b);
Vec2 vec2_scale(Vec2 a, float b);

typedef struct {
    float x;
    float y;
    float z;
    float w;
} Vec4;

Vec4 vec4_add(Vec4 a, Vec4 b);
Vec4 vec4_sub(Vec4 a, Vec4 b);
Vec4 vec4_mul(Vec4 a, Vec4 b);
Vec4 vec4_scale(Vec4 a, float b);

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#endif // LA_H
