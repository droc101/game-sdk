#version 320 es

precision mediump float;

layout (location = 0) in vec3 VERTEX;
layout (location = 1) in vec2 VERTEX_UV;
layout (location = 2) in vec3 VERTEX_NORMAL;

layout (location = 4) out vec2 UV;
layout (location = 5) out vec3 NORMAL;

uniform mat4 PROJECTION;

void main()
{
    gl_Position = PROJECTION * vec4(VERTEX, 1.0);
    UV = VERTEX_UV;
    NORMAL = VERTEX_NORMAL;
}
