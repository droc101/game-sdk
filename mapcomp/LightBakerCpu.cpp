//
// Created by NBT22 on 3/20/26.
//

#include "LightBakerCpu.hpp"

#include <glm/geometric.hpp>
#include <thread>

namespace converted_glsl
{
namespace
{
    using namespace glm;

    uint32_t width = 1;
    uint32_t height = 1;
    std::vector<Light> lights;
    std::vector<MapVertex> vertices;
    std::vector<uint32_t> indices;

    uvec3 gl_NumWorkGroups{};
    uvec3 gl_WorkGroupSize{};
    thread_local uvec3 gl_WorkGroupID{};
    thread_local uvec3 gl_LocalInvocationID{};
    thread_local uvec3 gl_GlobalInvocationID{};
    thread_local uint gl_LocalInvocationIndex{};

    constexpr float EPSILON = 1e-6f; // std::numeric_limits<float>::epsilon();
    constexpr float PI = 3.141592653589793f; //std::numbers::pi;

    struct MapTriangle
    {
            MapVertex a{};
            MapVertex b{};
            MapVertex c{};
    };

    struct Intersection
    {
            float distance = std::numeric_limits<float>::infinity();
            vec3 intersection{};
            MapTriangle triangle{};
            bool isBackface{};
            float lightStrength{};
    };

    constexpr Intersection intersectionNone{};
} // namespace

static inline Intersection calculateIntersection(vec3 origin, vec3 ray, MapTriangle triangle)
{
    vec3 edge1 = triangle.b.position - triangle.a.position;
    vec3 edge2 = triangle.c.position - triangle.a.position;

    vec3 pvec = cross(ray, edge2);

    float det = dot(edge1, pvec);
    if (abs(det) < EPSILON)
    {
        return {};
    }

    float inverse_det = 1.0f / det;

    vec3 tvec = origin - triangle.a.position;

    float u = dot(tvec, pvec) * inverse_det;
    if (u < -EPSILON || u - 1 > EPSILON)
    {
        return intersectionNone;
    }

    vec3 qvec = cross(tvec, edge1);

    float v = dot(ray, qvec) * inverse_det;
    if (v < -EPSILON || u + v - 1 > EPSILON)
    {
        return intersectionNone;
    }

    float t = dot(edge2, qvec) * inverse_det;
    if (t > EPSILON)
    {
        return Intersection(t, origin + ray * t, triangle, det < EPSILON, 0);
    } else
    {
        return intersectionNone;
    }
}

vec2 computeLuxelUv(MapTriangle triangle, vec3 hit)
{
    vec3 v0 = triangle.b.position - triangle.a.position;
    vec3 v1 = triangle.c.position - triangle.a.position;
    vec3 v2 = hit - triangle.a.position;

    float d00 = dot(v0, v0);
    float d01 = dot(v0, v1);
    float d11 = dot(v1, v1);
    float d20 = dot(v2, v0);
    float d21 = dot(v2, v1);
    float denom = d00 * d11 - d01 * d01;

    float b = (d11 * d20 - d01 * d21) / denom;
    float c = (d00 * d21 - d01 * d20) / denom;
    float a = 1.0f - b - c;

    return a * triangle.a.lightmapUv + b * triangle.b.lightmapUv + c * triangle.c.lightmapUv;
}

int computeLuxelIndex(MapTriangle triangle, vec3 hit)
{
    vec2 uv = computeLuxelUv(triangle, hit);
    return int(int(uv.x * width) + int(uv.y * height) * width);
}

float getLightStrength(float distance, Light light)
{
    float inverseRange = 1.0f / light.range;
    float nd = distance * inverseRange;
    nd *= nd;
    nd *= nd;
    nd = max(1.0f - nd, 0.0f);
    nd *= nd;
    return nd * pow(max(distance, 0.0001f), -light.attenuation);
}

static constexpr uint8_t getLowByte(const _Float16 val)
{
    return (std::bit_cast<uint8_t *>(&val))[0];
}

static constexpr uint8_t getHighByte(const _Float16 val)
{
    return (std::bit_cast<uint8_t *>(&val))[1];
}

static inline void imageStore(std::vector<uint8_t> &image, const int index, const vec4 &color)
{
    image.at(index * 4 * 2) = getLowByte(color.r);
    image.at(index * 4 * 2 + 1) = getHighByte(color.r);
    image.at(index * 4 * 2 + 2) = getLowByte(color.g);
    image.at(index * 4 * 2 + 3) = getHighByte(color.g);
    image.at(index * 4 * 2 + 4) = getLowByte(color.b);
    image.at(index * 4 * 2 + 5) = getHighByte(color.b);
    image.at(index * 4 * 2 + 6) = getLowByte(color.a);
    image.at(index * 4 * 2 + 7) = getHighByte(color.a);
}

static inline void mainFunction(std::vector<uint8_t> &outputLightmap)
{
    Light light = lights[gl_WorkGroupID.x];

    vec2 percents = ((vec2(gl_LocalInvocationID.x, gl_LocalInvocationID.y) /
                      vec2(gl_WorkGroupSize.x, gl_WorkGroupSize.y)) *
                     (float(gl_WorkGroupID.y + 1) / gl_NumWorkGroups.y)) *
                    float((gl_WorkGroupID.z + 1) / gl_WorkGroupSize.z);
    float theta = percents.x * 2 * PI;
    float phi = percents.y * 2 * PI;
    vec3 ray = vec3(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));

    Intersection closestIntersection = intersectionNone;
    for (uint i = 0; i < indices.size(); i += 3)
    {
        MapTriangle triangle = MapTriangle(vertices[indices[i]], vertices[indices[i + 1]], vertices[indices[i + 2]]);
        Intersection intersection = calculateIntersection(light.position, ray, triangle);
        if (isinf(intersection.distance))
        {
            continue; // Slight optimization, I hope
        }
        intersection.lightStrength = getLightStrength(intersection.distance, light);
        if (intersection.lightStrength < EPSILON)
        {
            continue;
        }
        if (intersection.distance < closestIntersection.distance)
        {
            closestIntersection = intersection;
        }
    }

    if (isinf(closestIntersection.distance))
    {
        return;
    }

    imageStore(outputLightmap,
               computeLuxelIndex(closestIntersection.triangle, closestIntersection.intersection),
               closestIntersection.lightStrength * vec4(light.color, 1));
}
} // namespace converted_glsl

void LightBakerCpu::bake(const std::unordered_map<std::string, LevelMeshBuilder> &meshBuilders,
                         const std::vector<Light> &lights,
                         std::vector<uint8_t> &pixelData,
                         const glm::ivec2 &lightmapSize)
{
    pixelData.clear();
    pixelData.resize(lightmapSize.x * lightmapSize.y * sizeof(_Float16) * 4);

    size_t indexOffset = 0;
    std::vector<MapVertex> vertices{};
    std::vector<uint32_t> indices{};
    for (const LevelMeshBuilder &builder: meshBuilders | std::views::values)
    {
        vertices.insert(vertices.end(), builder.GetVertices().begin(), builder.GetVertices().end());
        for (const uint32_t index: builder.GetIndices())
        {
            indices.emplace_back(index + indexOffset);
        }
        indexOffset += builder.GetVertices().size();
    }

    converted_glsl::width = lightmapSize.x;
    converted_glsl::height = lightmapSize.y;
    converted_glsl::lights = lights;
    converted_glsl::vertices = vertices;
    converted_glsl::indices = indices;

    converted_glsl::gl_NumWorkGroups = {lights.size(), 1024, 4};
    converted_glsl::gl_WorkGroupSize = {32, 32, 1};

    for (uint32_t gl_WorkGroupIDx = 0; gl_WorkGroupIDx < converted_glsl::gl_NumWorkGroups.x; gl_WorkGroupIDx++)
    {
        std::vector<std::thread> threads;
        for (uint32_t gl_WorkGroupIDy = 0; gl_WorkGroupIDy < converted_glsl::gl_NumWorkGroups.y; gl_WorkGroupIDy++)
        {
            threads.emplace_back([gl_WorkGroupIDx, gl_WorkGroupIDy, &pixelData]() -> void {
                for (uint32_t gl_WorkGroupIDz = 0; gl_WorkGroupIDz < converted_glsl::gl_NumWorkGroups.z;
                     gl_WorkGroupIDz++)
                {
                    converted_glsl::gl_WorkGroupID = {gl_WorkGroupIDx, gl_WorkGroupIDy, gl_WorkGroupIDz};
                    for (uint32_t gl_LocalInvocationIDx = 0; gl_LocalInvocationIDx < converted_glsl::gl_WorkGroupSize.x;
                         gl_LocalInvocationIDx++)
                    {
                        for (uint32_t gl_LocalInvocationIDy = 0;
                             gl_LocalInvocationIDy < converted_glsl::gl_WorkGroupSize.y;
                             gl_LocalInvocationIDy++)
                        {
                            for (uint32_t gl_LocalInvocationIDz = 0;
                                 gl_LocalInvocationIDz < converted_glsl::gl_WorkGroupSize.z;
                                 gl_LocalInvocationIDz++)
                            {
                                converted_glsl::gl_LocalInvocationID = {gl_LocalInvocationIDx,
                                                                        gl_LocalInvocationIDy,
                                                                        gl_LocalInvocationIDz};
                                converted_glsl::gl_GlobalInvocationID = converted_glsl::gl_WorkGroupID *
                                                                                converted_glsl::gl_WorkGroupSize +
                                                                        converted_glsl::gl_LocalInvocationID;
                                converted_glsl::gl_LocalInvocationIndex = converted_glsl::gl_LocalInvocationID.z *
                                                                                  converted_glsl::gl_WorkGroupSize.x *
                                                                                  converted_glsl::gl_WorkGroupSize.y +
                                                                          converted_glsl::gl_LocalInvocationID.y *
                                                                                  converted_glsl::gl_WorkGroupSize.x +
                                                                          converted_glsl::gl_LocalInvocationID.x;
                                converted_glsl::mainFunction(pixelData);
                            }
                        }
                    }
                }
            });
        }
        for (std::thread &thread: threads)
        {
            thread.join();
        }
    }
}
