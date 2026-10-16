#version 460

out mediump vec4 COLOR;

in vec3 FSVCOL;

void main()
{
    COLOR.rgb = FSVCOL;
    COLOR.a = 1.0;
}
