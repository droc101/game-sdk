#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_debug_printf : require
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
    float brightness;
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
    float attenuationMultiplier;
    float brightAngle;
    float fadingAngle;
};

struct Payload {
    Light sourceLight;
    vec3 rayOrigin;
    vec3 rayDirection;
    int targetLuxelIndex;
    float distanceTraveled;
};

layout (constant_id = 0) const uint WIDTH = 8192;
layout (constant_id = 1) const uint HEIGHT = 4096;

hitAttributeEXT vec2 barycentric;

layout(location = 0) rayPayloadInEXT Payload payload;

layout(set = 0, binding = 0) uniform accelerationStructureEXT accelerationStructure;
/// The buffer containing the map vertex data
layout (std140, set = 0, binding = 4) readonly buffer VertexData {
    MapVertex vertices[];
} vertexData;
/// The buffer containing the map index data
layout (set = 0, binding = 5) readonly buffer IndexData {
    uint indices[];
} indexData;
// The lightmap from which data is both read and written to
layout (set = 0, binding = 6, rgba16f) coherent uniform imageBuffer lightmap;
// layout (set = 0, binding = 6) coherent buffer LightmapLuxels {
//     uint luxels[];
// } lightmap;

// void atomicAddColorToLuxelChannel(uint luxel, int channel, float val) {
//     // lightmap.luxels[luxel + channel] = floatBitsToUint((uintBitsToFloat(lightmap.luxels[luxel + channel]) + val));
//     float val1 = uintBitsToFloat(lightmap.luxels[luxel + channel]);
//     float val2 = uintBitsToFloat(lightmap.luxels[luxel + channel]);
//     do {
//         val1 = val2;
//         val2 = uintBitsToFloat(atomicCompSwap(lightmap.luxels[luxel + channel], floatBitsToUint(val1), floatBitsToUint(val1 + val)));
//     } while (val1 != val2);
// }
// void atomicAddColorToLightmap(vec3 color, uint index) {
//     atomicAddColorToLuxelChannel(3 * index, 0, color.r);
//     atomicAddColorToLuxelChannel(3 * index, 1, color.g);
//     atomicAddColorToLuxelChannel(3 * index, 2, color.b);
// }

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

float getLightBrightness(const Light light, const float distance) {
    float multiplierSquared = light.attenuationMultiplier * light.attenuationMultiplier;
    return (multiplierSquared * light.brightness) /
           (multiplierSquared * light.constantAttenuation +
            light.attenuationMultiplier * light.linearAttenuation * distance +
            light.quadraticAttenuation * distance * distance);
}

vec3 getLightColor() {
    const float brightness = getLightBrightness(payload.sourceLight, payload.distanceTraveled);
    if (brightness < 0.00390625) {
        return vec3(0);
    }
    return payload.sourceLight.color * brightness;
}

void main() {
    if (gl_HitKindEXT == gl_HitKindBackFacingTriangleEXT) {
        return;
    }
    const int luxelIndex = computeLuxelIndex();
    if (luxelIndex != payload.targetLuxelIndex) {
        return;
    }
    payload.distanceTraveled += gl_HitTEXT;
    imageStore(lightmap, luxelIndex, imageLoad(lightmap, luxelIndex) + vec4(getLightColor(), 1));
    // atomicAddColorToLightmap(payload.sourceLight.color, 2 * payload.targetLuxelIndex);
    // atomicAddColorToLightmap(getLightColor(), luxelIndex);
    // imageStore(lightmap, payload.targetLuxelIndex, vec4(1));
}
