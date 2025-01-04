#version 330 core

in vec2 texcoord;
out vec4 color;

uniform vec2 mouse;
uniform float aspect;

uniform float lens_size;
uniform vec4 lens_color;

uniform bool select_began;
uniform vec2 select_mouse;
uniform vec2 select_start;
uniform vec4 select_color;

void main()
{
    color = vec4(1.0, 1.0, 1.0, 0.0);
    if (length((mouse - texcoord) * vec2(aspect, 1.0)) >= lens_size) {
        color = lens_color;
    }

    if (select_began) {
        vec2 a = vec2(
            min(select_mouse.x, select_start.x),
            min(select_mouse.y, select_start.y));

        vec2 b = vec2(
            max(select_mouse.x, select_start.x),
            max(select_mouse.y, select_start.y));

        if (a.x <= texcoord.x && texcoord.x <= b.x) {
            if (abs(texcoord.y - a.y) < 0.001 || abs(texcoord.y - b.y) < 0.001) {
                color = select_color;
            }
        }

        if (a.y <= texcoord.y && texcoord.y <= b.y) {
            if (aspect * abs(texcoord.x - a.x) < 0.001 ||
                aspect * abs(texcoord.x - b.x) < 0.001) {
                color = select_color;
            }
        }
    }
}
