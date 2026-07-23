#extension GL_EXT_ray_tracing : require
#extension GL_EXT_debug_printf : require
#extension GL_EXT_scalar_block_layout : require

/// Enable extra checks and debug printf logging
// #define DEBUG

#pragma region Constants

const float EPSILON = 1e-6;
const float PI = 3.141592653589793;
const float MIN_BRIGHTNESS = 1.0 / 256.0;
const float MIN_RAY_LENGTH = 0.0001;
const float MAX_RAY_LENGTH = 1776; // 2 * sqrt(3) * MAP_MAX_HALF_EXTENTS; Rounded up slightly

layout (constant_id = 0) const uint WIDTH = 8192;
layout (constant_id = 1) const uint HEIGHT = 4096;

#pragma endregion Constants

#pragma region Types

/// The format of the map's vertices. Interop with C++ must use scalar block layout
struct MapVertex {
    vec3 position;
    vec2 uv;
    vec2 lightmapUv;
    vec3 normal;
    uint textureIndex;
    float emissive;
};

#pragma region LightTypeEnum
const uint LIGHT_TYPE_POINT = 0u;
const uint LIGHT_TYPE_SPOT = 1u;
const uint LIGHT_TYPE_AREA = 2u;
const uint LIGHT_TYPE_DIRECTIONAL = 3u;
#pragma endregion LightTypeEnum

/// The format of the lights in the map. Interop with C++ must use scalar block layout
struct Light {
    uint type; // Maps to an enum in C++
    vec3 position;
    vec3 negativeForwardDirection;
    vec3 color;
    float brightness;
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
    float attenuationMultiplier;
    float brightAngle; // 0-90 degrees
    float fadingAngle;
};

#pragma endregion Types

#pragma region Pipeline Layout

layout (push_constant) uniform PushConstants {
    uint baseLuxelIndex;
} pushConstants;

layout (set = 0, binding = 0) uniform accelerationStructureEXT accelerationStructure;
layout (set = 0, binding = 1, rgba16f) restrict uniform imageBuffer outputLightmap;
layout (set = 0, binding = 2, rgba16f) readonly restrict uniform imageBuffer previousBounceLightmap;
layout (set = 0, binding = 3, rgba16f) writeonly restrict uniform imageBuffer currentBounceLightmap;
layout (set = 0, binding = 4, rgba32f) readonly restrict uniform image2D luxelPositions;
layout (set = 0, binding = 5, rgba32f) readonly restrict uniform image2D luxelNormals;
layout (set = 0, binding = 6, rgba16f) readonly restrict uniform image2D luxelAlbedos;
layout (scalar, set = 0, binding = 7) readonly restrict buffer LightsData {
    Light lights[];
} lightsData;
layout (set = 0, binding = 8) readonly restrict buffer EmissiveLuxelIndices {
    uint indexCount;
    int indices[];
} emissiveLuxelIndices;
layout (scalar, set = 0, binding = 9) readonly restrict buffer VertexData {
    MapVertex vertices[];
} vertexData;
layout (set = 0, binding = 10) readonly restrict buffer IndexData {
    uint indices[];
} indexData;

#pragma endregion Pipeline Layout
