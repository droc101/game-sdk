#version 460

in vec2 UV;

out vec4 FRAG_COLOR;

uniform sampler2D TEXTURE;

void main() {
    FRAG_COLOR = texture(TEXTURE, UV);
}