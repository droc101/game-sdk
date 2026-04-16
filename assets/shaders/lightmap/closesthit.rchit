#version 460

#extension GL_EXT_ray_tracing : require
// #extension GL_EXT_debug_printf : require
#extension GL_KHR_memory_scope_semantics : require

/// The format of the map's vertices. Interop with C++ must use std140
struct MapVertex {
    vec3 position;
    vec2 uv;
    vec2 lightmapUv;
    vec3 normal;
};

/// The format of the lights in the map. Interop with C++ must use std140
struct Light {
    uint type; // Maps to an enum in C++
    vec3 position;
    vec3 rotation;
    vec3 color;
    float brightnessScale;
    float range;
    float attenuation;
    float coneAngle;
    float angularAttenuation;
};

struct Payload {
    uint sourceLightIndex;
    Light sourceLight;
    vec3 rayOrigin;
    vec3 rayDirection;
    float distanceTraveled;
};

layout (constant_id = 0) const uint WIDTH = 8192;
layout (constant_id = 1) const uint HEIGHT = 4096;
layout (constant_id = 2) const uint LIGHT_COUNT = 1;

hitAttributeEXT vec2 barycentric;

layout(location = 0) rayPayloadInEXT Payload payload;

layout(set = 0, binding = 0) uniform accelerationStructureEXT accelerationStructure;
/// The buffer containing the map vertex data
layout (std140, set = 0, binding = 2) readonly buffer VertexData {
    MapVertex vertices[];
} vertexData;
/// The buffer containing the map index data
layout (set = 0, binding = 3) readonly buffer IndexData {
    uint indices[];
} indexData;
layout (set = 0, binding = 4) coherent buffer LightHitIndicesData {
    uint indices[]; // WIDTH * HEIGHT * LIGHT_COUNT / 32
} lightHitIndicesData;
// The lightmap from which data is both read and written to
layout (set = 0, binding = 5) coherent buffer LightmapLuxels {
    uint luxels[];
} lightmap;

void atomicAddColorToLuxelChannel(uint luxel, int channel, float val) {
    // lightmap.luxels[luxel + channel] = floatBitsToUint((uintBitsToFloat(lightmap.luxels[luxel + channel]) + val));
    float val1 = uintBitsToFloat(lightmap.luxels[luxel + channel]);
    float val2 = uintBitsToFloat(lightmap.luxels[luxel + channel]);
    do {
        val1 = val2;
        val2 = uintBitsToFloat(atomicCompSwap(lightmap.luxels[luxel + channel], floatBitsToUint(val1), floatBitsToUint(val1 + val)));
    } while (val1 != val2);
}
void atomicAddColorToLightmap(vec3 color, uint index) {
    atomicAddColorToLuxelChannel(3 * index, 0, color.r);
    atomicAddColorToLuxelChannel(3 * index, 1, color.g);
    atomicAddColorToLuxelChannel(3 * index, 2, color.b);
}

vec2 computeLuxelUv() {
    return (1.0 - barycentric.x - barycentric.y) * vertexData.vertices[indexData.indices[3 * gl_PrimitiveID + 0]].lightmapUv +
                   barycentric.x * vertexData.vertices[indexData.indices[3 * gl_PrimitiveID + 1]].lightmapUv +
                   barycentric.y * vertexData.vertices[indexData.indices[3 * gl_PrimitiveID + 2]].lightmapUv;
}
int computeLuxelIndex() {
    vec2 uv = clamp(computeLuxelUv(), vec2(0), vec2(1));
    int x = int(round(uv.x * WIDTH));
    int y = int(round(uv.y * HEIGHT));
    return int(x + y * WIDTH);
}

vec3 getLightColor() {
    const Light light = payload.sourceLight;
    payload.distanceTraveled += gl_HitTEXT;
    if (light.range < payload.distanceTraveled) {
        return vec3(0);
    }
    float inverseRange = 1.0 / light.range;
    float nd = payload.distanceTraveled * inverseRange;
    nd *= nd;
    nd *= nd;
    nd = max(1.0 - nd, 0.0);
    nd *= nd;
    return nd * pow(payload.distanceTraveled, -light.attenuation) * light.brightnessScale * light.color;
}

bool atomicLightHitLuxelIndex(uint luxelIndex) {
    const uint index = luxelIndex * LIGHT_COUNT + payload.sourceLightIndex;
    const uint bit = 1u << (index % 32);
    return (atomicOr(lightHitIndicesData.indices[index / 32], bit) & bit) == 0;
}

void main() {
    if (gl_HitKindEXT == gl_HitKindBackFacingTriangleEXT) {
        return;
    }
    const uint luxelIndex = computeLuxelIndex();
    // if (payload.distanceTraveled == 0) {
        if (!atomicLightHitLuxelIndex(luxelIndex)) {
            return;
        }
    // }
    atomicAddColorToLightmap(vec3(getLightColor()), luxelIndex);
    payload.rayOrigin = (1.0 - barycentric.x - barycentric.y) * vertexData.vertices[indexData.indices[3 * gl_PrimitiveID + 0]].position +
                                barycentric.x * vertexData.vertices[indexData.indices[3 * gl_PrimitiveID + 1]].position +
                                barycentric.y * vertexData.vertices[indexData.indices[3 * gl_PrimitiveID + 2]].position;
    payload.rayDirection = reflect(payload.rayDirection,
                                   (1.0 - barycentric.x - barycentric.y) * vertexData.vertices[indexData.indices[3 * gl_PrimitiveID + 0]].normal +
                                           barycentric.x * vertexData.vertices[indexData.indices[3 * gl_PrimitiveID + 1]].normal +
                                           barycentric.y * vertexData.vertices[indexData.indices[3 * gl_PrimitiveID + 2]].normal);
}
