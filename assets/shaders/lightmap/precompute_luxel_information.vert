#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUv;
layout(location = 2) in vec2 inLightmapUv;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in uint inTextureIndex;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUv;
layout(location = 3) flat out uint outTextureIndex;

void main() {
    outPosition = inPosition;
    outNormal = inNormal;
    outUv = inUv;
    outTextureIndex = inTextureIndex;
    gl_Position = vec4(inLightmapUv * 2 - 1, 0, 1);
}
