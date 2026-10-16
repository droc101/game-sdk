#version 460

in vec3 VERTEX;
in vec2 VERTEX_UV;

out vec2 UV;

uniform mat4 VIEW_MATRIX;
uniform mat4 WORLD_MATRIX;

void main() {
    UV = VERTEX_UV;
    gl_Position = VIEW_MATRIX * WORLD_MATRIX * vec4(VERTEX, 1.0);
}