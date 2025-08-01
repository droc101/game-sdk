#version 330

in vec3 VERTEX;
in vec2 VERTEX_UV;
in vec3 VERTEX_NORMAL;
in vec4 VERTEX_COLOR;

out vec2 UV;
out vec3 NORMAL;
out vec4 COLOR;

uniform mat4 PROJECTION;
uniform mat4 VIEW;

void main()
{
    gl_Position = PROJECTION * VIEW * vec4(VERTEX, 1.0);
    UV = VERTEX_UV;
    NORMAL = VERTEX_NORMAL;
    COLOR = VERTEX_COLOR;
}
