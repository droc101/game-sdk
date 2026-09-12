#version 460

out vec4 COLOR;

void main() {
    const float a = float(gl_PrimitiveID) / 15.0;
    COLOR = vec4(a, a, a, 1);
}