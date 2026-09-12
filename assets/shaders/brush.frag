#version 460

in vec2 UV;

out vec4 COLOR;

uniform sampler2D TEXTURE;

void main() {
    COLOR = texture(TEXTURE, UV);
}