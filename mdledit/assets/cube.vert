#version 320 es

precision mediump float;

layout (location = 0) in vec3 VERTEX;

uniform mat4 PROJECTION;

void main()
{
    gl_Position = PROJECTION * vec4(VERTEX, 1.0);
}
