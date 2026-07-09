#version 460

#extension GL_EXT_debug_printf : require
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUv;
layout(location = 3) flat in uint inTextureIndex;
layout(location = 4) in float inEmissive;

layout(location = 0) out vec4 outLuxelPosition;
layout(location = 1) out vec4 outLuxelNormal;
layout(location = 2) out vec4 outLuxelAlbedo;

layout(binding = 0) uniform sampler2D textureSampler[];

void main() {
    outLuxelPosition = vec4(inPosition, 1);
    outLuxelNormal = vec4(inNormal, inEmissive);
    outLuxelAlbedo = texture(textureSampler[nonuniformEXT(inTextureIndex)], inUv);
}
