#version 330

in vec3 VERTEX;
in vec3 VERTEX_COLOR;

uniform mat4 VIEW;

out vec3 FSVCOL;

void main()
{
    gl_Position = VIEW * vec4(VERTEX, 1.0);
    FSVCOL = VERTEX_COLOR;
}
