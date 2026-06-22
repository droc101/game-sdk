#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_debug_printf : require

/// The format of the map's vertices. Interop with C++ must use std140
struct MapVertex {
    vec3 position;
    vec2 uv;
    vec2 lightmapUv;
    vec3 normal;
};

const float MIN_BRIGHTNESS = 1.0 / 256.0;

layout (constant_id = 0) const uint WIDTH = 8192;
layout (constant_id = 1) const uint HEIGHT = 4096;

hitAttributeEXT vec2 barycentric;

layout(location = 0) rayPayloadInEXT vec3 hitColor;

// layout (set = 0, binding = 0) uniform accelerationStructureEXT accelerationStructure;
layout (set = 0, binding = 1, rgba16f) readonly restrict uniform imageBuffer inputLightmap;
// layout (set = 0, binding = 2, rgba16f) writeonly restrict uniform imageBuffer outputLightmap;
// layout (set = 0, binding = 3, rgba32f) readonly restrict uniform image2D luxelPositions;
// layout (set = 0, binding = 4, rgba32f) readonly restrict uniform image2D luxelNormals;
layout (set = 0, binding = 5, rgba8) readonly restrict uniform image2D luxelAlbedos;
// layout (std140, set = 0, binding = 6) readonly restrict buffer LightsData {
//     Light lights[];
// } lightsData;
layout (std140, set = 0, binding = 7) readonly restrict buffer VertexData {
    MapVertex vertices[];
} vertexData;
layout (set = 0, binding = 8) readonly restrict buffer IndexData {
    uint indices[];
} indexData;

vec2 computeLuxelUv() {
    return (1.0 - barycentric.x - barycentric.y) * vertexData.vertices[indexData.indices[3 * gl_PrimitiveID + 0]].lightmapUv +
                   barycentric.x * vertexData.vertices[indexData.indices[3 * gl_PrimitiveID + 1]].lightmapUv +
                   barycentric.y * vertexData.vertices[indexData.indices[3 * gl_PrimitiveID + 2]].lightmapUv;
}
int computeLuxelIndex() {
    vec2 uv = clamp(computeLuxelUv(), vec2(0), vec2(1));
    int x = int(uv.x * WIDTH);
    int y = int(uv.y * HEIGHT);
    return int(x + y * WIDTH);
}

void main() {
    if (gl_HitKindEXT == gl_HitKindBackFacingTriangleEXT) {
        return;
    }
    hitColor = imageLoad(inputLightmap, computeLuxelIndex()).rgb;
}