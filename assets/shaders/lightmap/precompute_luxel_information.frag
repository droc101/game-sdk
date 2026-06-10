#version 460

layout (constant_id = 0) const uint WIDTH = 8192;
layout (constant_id = 1) const uint HEIGHT = 4096;

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec4 outLuxelInformation;

void main() {
    outLuxelInformation = vec4(inPosition, 1);
}