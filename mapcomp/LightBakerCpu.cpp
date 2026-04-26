//
// Created by NBT22 on 3/20/26.
//

#include "LightBakerCpu.hpp"
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <glm/geometric.hpp>
#include <libassets/type/MapVertex.h>
#include <limits>
#include <numbers>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include "LevelMeshBuilder.h"
#include "Light.h"

namespace ConvertedGlsl
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
    thread_local uint32_t gl_LocalInvocationIndex{};

    constexpr float EPSILON = 1e-6f; // std::numeric_limits<float>::epsilon();
    constexpr float PI = std::numbers::pi_v<float>; // 3.141592653589793f;

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

    constexpr Intersection INTERSECTION_NONE{};
} // namespace

static Intersection CalculateIntersection(vec3 origin, vec3 ray, MapTriangle triangle)
{
    const vec3 edge1 = triangle.b.position - triangle.a.position;
    const vec3 edge2 = triangle.c.position - triangle.a.position;

    const vec3 pvec = cross(ray, edge2);

    const float det = dot(edge1, pvec);
    if (abs(det) < EPSILON)
    {
        return {};
    }

    const float inverseDet = 1.0f / det;

    const vec3 tvec = origin - triangle.a.position;

    const float u = dot(tvec, pvec) * inverseDet;
    if (u < -EPSILON || u - 1 > EPSILON)
    {
        return INTERSECTION_NONE;
    }

    const vec3 qvec = cross(tvec, edge1);

    const float v = dot(ray, qvec) * inverseDet;
    if (v < -EPSILON || u + v - 1 > EPSILON)
    {
        return INTERSECTION_NONE;
    }

    const float t = dot(edge2, qvec) * inverseDet;
    if (t > EPSILON)
    {
        return Intersection(t, origin + ray * t, triangle, det < EPSILON, 0);
    }
    return INTERSECTION_NONE;
}

static vec2 ComputeLuxelUv(const MapTriangle &triangle, const vec3 hit)
{
    const vec3 v0 = triangle.b.position - triangle.a.position;
    const vec3 v1 = triangle.c.position - triangle.a.position;
    const vec3 v2 = hit - triangle.a.position;

    const float d00 = dot(v0, v0);
    const float d01 = dot(v0, v1);
    const float d11 = dot(v1, v1);
    const float d20 = dot(v2, v0);
    const float d21 = dot(v2, v1);
    const float denom = d00 * d11 - d01 * d01;

    const float b = (d11 * d20 - d01 * d21) / denom;
    const float c = (d00 * d21 - d01 * d20) / denom;
    const float a = 1.0f - b - c;

    return a * triangle.a.lightmapUv + b * triangle.b.lightmapUv + c * triangle.c.lightmapUv;
}

static int ComputeLuxelIndex(const MapTriangle &triangle, const vec3 hit)
{
    const vec2 uv = ComputeLuxelUv(triangle, hit);
    return static_cast<int>(static_cast<int>(uv.x * static_cast<float>(width)) +
                            static_cast<int>(uv.y * static_cast<float>(height)) * width);
}

static float GetLightStrength(const float distance, const Light &light)
{
    const float inverseRange = 1.0f / light.range;
    float nd = distance * inverseRange;
    nd *= nd;
    nd *= nd;
    nd = max(1.0f - nd, 0.0f);
    nd *= nd;
    return nd * pow(max(distance, 0.0001f), -light.attenuation);
}

static constexpr uint8_t GetLowByte(const _Float16 val)
{
    return (std::bit_cast<uint8_t *>(&val))[0];
}

static constexpr uint8_t GetHighByte(const _Float16 val)
{
    return (std::bit_cast<uint8_t *>(&val))[1];
}

static void ImageStore(std::vector<uint8_t> &image, const int index, const vec4 &color)
{
    image.at(index * 4 * 2) = GetLowByte(static_cast<_Float16>(color.r));
    image.at(index * 4 * 2 + 1) = GetHighByte(static_cast<_Float16>(color.r));
    image.at(index * 4 * 2 + 2) = GetLowByte(static_cast<_Float16>(color.g));
    image.at(index * 4 * 2 + 3) = GetHighByte(static_cast<_Float16>(color.g));
    image.at(index * 4 * 2 + 4) = GetLowByte(static_cast<_Float16>(color.b));
    image.at(index * 4 * 2 + 5) = GetHighByte(static_cast<_Float16>(color.b));
    image.at(index * 4 * 2 + 6) = GetLowByte(static_cast<_Float16>(color.a));
    image.at(index * 4 * 2 + 7) = GetHighByte(static_cast<_Float16>(color.a));
}

static void MainFunction(std::vector<uint8_t> &outputLightmap)
{
    const Light light = lights.at(gl_WorkGroupID.x);

    const vec2 percents = ((vec2(gl_LocalInvocationID.x, gl_LocalInvocationID.y) /
                            vec2(gl_WorkGroupSize.x, gl_WorkGroupSize.y)) *
                           (static_cast<float>(gl_WorkGroupID.y + 1) / gl_NumWorkGroups.y)) *
                          static_cast<float>((gl_WorkGroupID.z + 1) / gl_WorkGroupSize.z);
    const float theta = percents.x * 2 * PI;
    const float phi = percents.y * 2 * PI;
    const vec3 ray = vec3(sinf(theta) * cosf(phi), cosf(theta), sinf(theta) * sinf(phi));

    Intersection closestIntersection = INTERSECTION_NONE;
    for (uint32_t i = 0; i < indices.size(); i += 3)
    {
        const MapTriangle triangle = MapTriangle(vertices.at(indices.at(i)),
                                                 vertices.at(indices.at(i + 1)),
                                                 vertices.at(indices.at(i + 2)));
        Intersection intersection = CalculateIntersection(light.position, ray, triangle);
        if (isinf(intersection.distance))
        {
            continue; // Slight optimization, I hope
        }
        intersection.lightStrength = GetLightStrength(intersection.distance, light);
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

    ImageStore(outputLightmap,
               ComputeLuxelIndex(closestIntersection.triangle, closestIntersection.intersection),
               closestIntersection.lightStrength * vec4(light.color, 1));
}
} // namespace ConvertedGlsl

void LightBakerCpu::Bake(const std::unordered_map<std::string, LevelMeshBuilder> &meshBuilders,
                         const std::vector<Light> &lights,
                         std::vector<uint8_t> &pixelData,
                         const glm::uvec2 &lightmapSize)
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

    ConvertedGlsl::width = lightmapSize.x;
    ConvertedGlsl::height = lightmapSize.y;
    ConvertedGlsl::lights = lights;
    ConvertedGlsl::vertices = vertices;
    ConvertedGlsl::indices = indices;

    ConvertedGlsl::gl_NumWorkGroups = {lights.size(), 1024, 4};
    ConvertedGlsl::gl_WorkGroupSize = {32, 32, 1};

    for (uint32_t gl_WorkGroupIDx = 0; gl_WorkGroupIDx < ConvertedGlsl::gl_NumWorkGroups.x; gl_WorkGroupIDx++)
    {
        std::vector<std::thread> threads;
        for (uint32_t gl_WorkGroupIDy = 0; gl_WorkGroupIDy < ConvertedGlsl::gl_NumWorkGroups.y; gl_WorkGroupIDy++)
        {
            threads.emplace_back([gl_WorkGroupIDx, gl_WorkGroupIDy, &pixelData]() -> void {
                for (uint32_t gl_WorkGroupIDz = 0; gl_WorkGroupIDz < ConvertedGlsl::gl_NumWorkGroups.z;
                     gl_WorkGroupIDz++)
                {
                    ConvertedGlsl::gl_WorkGroupID = {gl_WorkGroupIDx, gl_WorkGroupIDy, gl_WorkGroupIDz};
                    for (uint32_t gl_LocalInvocationIDx = 0; gl_LocalInvocationIDx < ConvertedGlsl::gl_WorkGroupSize.x;
                         gl_LocalInvocationIDx++)
                    {
                        for (uint32_t gl_LocalInvocationIDy = 0;
                             gl_LocalInvocationIDy < ConvertedGlsl::gl_WorkGroupSize.y;
                             gl_LocalInvocationIDy++)
                        {
                            for (uint32_t gl_LocalInvocationIDz = 0;
                                 gl_LocalInvocationIDz < ConvertedGlsl::gl_WorkGroupSize.z;
                                 gl_LocalInvocationIDz++)
                            {
                                ConvertedGlsl::gl_LocalInvocationID = {gl_LocalInvocationIDx,
                                                                       gl_LocalInvocationIDy,
                                                                       gl_LocalInvocationIDz};
                                ConvertedGlsl::gl_GlobalInvocationID = ConvertedGlsl::gl_WorkGroupID *
                                                                               ConvertedGlsl::gl_WorkGroupSize +
                                                                       ConvertedGlsl::gl_LocalInvocationID;
                                ConvertedGlsl::gl_LocalInvocationIndex = ConvertedGlsl::gl_LocalInvocationID.z *
                                                                                 ConvertedGlsl::gl_WorkGroupSize.x *
                                                                                 ConvertedGlsl::gl_WorkGroupSize.y +
                                                                         ConvertedGlsl::gl_LocalInvocationID.y *
                                                                                 ConvertedGlsl::gl_WorkGroupSize.x +
                                                                         ConvertedGlsl::gl_LocalInvocationID.x;
                                ConvertedGlsl::MainFunction(pixelData);
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
