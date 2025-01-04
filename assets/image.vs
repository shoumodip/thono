#version 330 core

layout (location = 0) in vec2 pos;
layout (location = 1) in vec2 uv;

out vec2 texcoord;

uniform float zoom;
uniform vec2 offset;

void main()
{
    gl_Position = vec4(pos * zoom + offset, 0.0, 1.0);
    texcoord = uv;
}
