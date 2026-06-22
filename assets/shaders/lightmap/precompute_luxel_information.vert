#version 460

layout(location = 0) in vec2 inLightmapUv;
layout(location = 1) in vec3 inPosition;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outNormal;

void main() {
    outPosition = inPosition;
    outNormal = inNormal;
    gl_Position = vec4(inLightmapUv * 2 - 1, 0, 1);
}