#version 460

in vec3 VERTEX;
in vec3 VERTEX_COLOR;

uniform mat4 VIEW;

out vec3 COLOR;

void main()
{
    gl_Position = VIEW * vec4(VERTEX, 1.0);
    COLOR = VERTEX_COLOR;
}
