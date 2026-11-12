#version 460

out vec4 FRAG_COLOR;

uniform vec4 COLOR;
uniform sampler2D SPRITE;

void main() {
    FRAG_COLOR = texture(SPRITE, gl_PointCoord) * COLOR;
}