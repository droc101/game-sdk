#version 330

in vec3 VERTEX;
in vec2 VERTEX_UV;
in vec3 VERTEX_NORMAL;

out vec2 UV;
out vec3 NORMAL;

uniform mat4 PROJECTION;
uniform mat4 VIEW;

void main()
{
    gl_Position = PROJECTION * VIEW * vec4(VERTEX, 1.0);
    UV = VERTEX_UV;
    NORMAL = VERTEX_NORMAL;
}
