#version 330

in vec3 VERTEX;

uniform mat4 MATRIX;

void main() {
    gl_Position = MATRIX * vec4(VERTEX, 1.0);
}