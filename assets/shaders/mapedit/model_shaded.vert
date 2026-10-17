#version 460

in vec3 VERTEX;
in vec2 VERTEX_UV;
in vec3 VERTEX_NORMAL;
in vec4 VERTEX_COLOR;

out vec2 UV;
out vec3 NORMAL;
out vec4 COLOR;

uniform mat4 VIEW_MATRIX;
uniform mat4 WORLD_MATRIX;

void main()
{
    gl_Position = VIEW_MATRIX * WORLD_MATRIX * vec4(VERTEX, 1.0);
    UV = VERTEX_UV;
    NORMAL = (WORLD_MATRIX * vec4(VERTEX_NORMAL, 0.0)).xyz;
    COLOR = VERTEX_COLOR;
}
