#version 460

in vec3 VERTEX;
in vec3 VERTEX_COLOR;

uniform mat4 VIEW_MATRIX;

out vec3 COLOR;

void main()
{
    gl_Position = VIEW_MATRIX * vec4(VERTEX, 1.0);
    COLOR = VERTEX_COLOR;
}
