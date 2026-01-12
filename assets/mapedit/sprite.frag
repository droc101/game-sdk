#version 330

out vec4 COLOR;

uniform vec4 color;
uniform sampler2D sprite;

void main() {
    COLOR = texture(sprite, gl_PointCoord) * color;
}