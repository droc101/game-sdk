#version 330

in vec3 VERTEX;

uniform mat4 VIEW_MATRIX;
uniform mat4 WORLD_MATRIX;

void main() {
    gl_Position = VIEW_MATRIX * WORLD_MATRIX * vec4(VERTEX, 1.0);
}