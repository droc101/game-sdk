#version 460

out vec4 FRAG_COLOR;

in vec3 COLOR;

void main()
{
    FRAG_COLOR.rgb = COLOR;
    FRAG_COLOR.a = 1.0;
}
