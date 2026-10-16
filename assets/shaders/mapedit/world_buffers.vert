#version 460

in vec3 VERTEX;
in vec3 VERTEX_NORMAL;

out vec3 POSITION;
out vec3 NORMAL;

uniform mat4 WORLD_MATRIX;
uniform mat4 VIEW_MATRIX;

void main()
{
    POSITION = (WORLD_MATRIX * vec4(VERTEX, 0.0)).xyz;
    NORMAL = (WORLD_MATRIX * vec4(VERTEX_NORMAL, 0.0)).xyz;

    gl_Position = VIEW_MATRIX * WORLD_MATRIX * vec4(VERTEX, 1.0);
}
