#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec4 outLuxelPosition;
layout(location = 1) out vec4 outLuxelNormal;

void main() {
    outLuxelPosition = vec4(inPosition, 1);
    outLuxelNormal = vec4(inNormal, 1);
}