#version 330

in vec3 lineColor;

out mediump vec4 COLOR;

void main() {
    COLOR.rgb = lineColor;
    COLOR.a = 1.0;
}