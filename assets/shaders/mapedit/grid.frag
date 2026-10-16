#version 460

in vec3 LINE_COLOR;

out mediump vec4 FRAG_COLOR;

void main() {
    FRAG_COLOR.rgb = LINE_COLOR;
    FRAG_COLOR.a = 1.0;
}