#version 460

out vec4 FRAG_COLOR;

uniform vec4 COLOR;

void main() {
    FRAG_COLOR = COLOR;
}