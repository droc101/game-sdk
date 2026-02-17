#version 330

in vec3 VERTEX;

uniform mat4 PROJECTION;
uniform mat4 VIEW;

void main()
{
    gl_Position = PROJECTION * VIEW * vec4(VERTEX, 1.0);
}
