#version 460

#include "shared.glsl"

layout (constant_id = 0) const uint WIDTH = 8192;
layout (constant_id = 1) const uint HEIGHT = 4096;

hitAttributeEXT vec2 barycentric;

layout(location = 0) rayPayloadInEXT vec3 hitColor;

vec2 computeLuxelUv() {
    return (1.0 - barycentric.x - barycentric.y) * vertexData.vertices[indexData.indices[3 * gl_PrimitiveID + 0]].lightmapUv +
                   barycentric.x * vertexData.vertices[indexData.indices[3 * gl_PrimitiveID + 1]].lightmapUv +
                   barycentric.y * vertexData.vertices[indexData.indices[3 * gl_PrimitiveID + 2]].lightmapUv;
}
int computeLuxelIndex(const vec2 uv) {
    int x = int(uv.x * WIDTH);
    int y = int(uv.y * HEIGHT);
    return int(x + y * WIDTH);
}

void main() {
    if (gl_HitKindEXT == gl_HitKindBackFacingTriangleEXT) {
        return;
    }
    const vec2 luxelUv = computeLuxelUv();
    const int luxelIndex = computeLuxelIndex(luxelUv);
    hitColor = imageLoad(luxelAlbedos, ivec2(luxelUv * vec2(WIDTH, HEIGHT))).rgb * imageLoad(inputLightmap, luxelIndex).rgb;
}
