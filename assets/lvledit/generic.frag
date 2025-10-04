#version 330

out vec4 COLOR;

uniform vec4 alb;

void main() {
    COLOR = alb;
}