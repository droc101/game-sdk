//
// Created by NBT22 on 3/11/26.
//

#include "LightBaker.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <glm/geometric.hpp>
#include <glslang/Public/ShaderLang.h>
#include <iostream>
#include <libassets/type/MapVertex.h>
#include <libassets/util/Error.h>
#include <libassets/util/ShaderCompiler.h>
#include <luna/luna.h>
#include <luna/lunaBuffer.h>
#include <luna/lunaDevice.h>
#include <luna/lunaImage.h>
#include <luna/lunaInstance.h>
#include <luna/lunaTypes.h>
#include <ranges>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include "LevelMeshBuilder.h"
#include "libassets/type/Actor.h"
#include "Light.h"

static constexpr bool CheckResult(const VkResult result)
{
    if (result != VK_SUCCESS)
    {
        printf("Error %d in Vulkan function!\n", result);
        return false;
    }
    return true;
}

LightBaker::LightBaker()
{
    constexpr LunaInstanceCreationInfo instanceCreationInfo = {
        .apiVersion = VK_API_VERSION_1_2,
#ifndef NDEBUG
        .enableValidation = true,
#endif
    };
    if (!CheckResult(lunaCreateInstance(&instanceCreationInfo)))
    {
        return;
    }

    constexpr LunaPhysicalDevicePreferenceDefinition physicalDevicePreferenceDefinition = {
        .preferredDeviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
    };
    const LunaDeviceCreationInfo deviceCreationInfo = {
        .physicalDevicePreferenceDefinition = &physicalDevicePreferenceDefinition,
    };
    if (!CheckResult(lunaCreateDevice(&deviceCreationInfo)))
    {
        return;
    }

    // constexpr LunaImageCreationInfo lightmap3dCreationInfo = {
    //     .format = VK_FORMAT_R16G16B16A16_SFLOAT,
    //     .width = LIGHTMAP_3D_SIZE,
    //     .height = LIGHTMAP_3D_SIZE,
    //     .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    //     .layout = VK_IMAGE_LAYOUT_GENERAL,
    //     .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    //     .writeInfo = {.destinationStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT},
    // };
    // if (!CheckResult(lunaCreateImage3D(&lightmap3dCreationInfo, LIGHTMAP_3D_SIZE, &lightmap3d)))
    // {
    //     return;
    // }

    // TODO: With the stable release of Luna 0.3.0 these can be moved to be two regions within one buffer
    constexpr LunaBufferCreationInfo bufferCreationInfo = {
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    };
    if (!CheckResult(lunaCreateBuffer(&bufferCreationInfo, &vertexBuffer)))
    {
        return;
    }
    if (!CheckResult(lunaCreateBuffer(&bufferCreationInfo, &indexBuffer)))
    {
        return;
    }

    constexpr std::array<LunaDescriptorSetLayoutBinding, 4> descriptorSetLayoutBindings{
        LunaDescriptorSetLayoutBinding{
            .bindingName = "vertices",
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
        LunaDescriptorSetLayoutBinding{
            .bindingName = "indices",
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
        LunaDescriptorSetLayoutBinding{
            .bindingName = "lights",
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
        LunaDescriptorSetLayoutBinding{
            .bindingName = "level geometry lightmap",
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
    };
    const LunaDescriptorSetLayoutCreationInfo descriptorSetLayoutCreationInfo = {
        .bindingCount = descriptorSetLayoutBindings.size(),
        .bindings = descriptorSetLayoutBindings.data(),
    };
    if (!CheckResult(lunaCreateDescriptorSetLayout(&descriptorSetLayoutCreationInfo, &descriptorSetLayout)))
    {
        return;
    }

    constexpr std::array<VkDescriptorPoolSize, 2> descriptorPoolSizes = {
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 3,
        },
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
            .descriptorCount = 1,
        },
    };
    const LunaDescriptorPoolCreationInfo descriptorPoolCreationInfo = {
        .maxSets = 1,
        .poolSizeCount = descriptorPoolSizes.size(),
        .poolSizes = descriptorPoolSizes.data(),
    };
    if (!CheckResult(lunaCreateDescriptorPool(&descriptorPoolCreationInfo, &descriptorPool)))
    {
        return;
    }

    const LunaDescriptorSetAllocationInfo allocationInfo = {
        .descriptorPool = descriptorPool,
        .setLayoutCount = 1,
        .setLayouts = &descriptorSetLayout,
    };
    if (!CheckResult(lunaAllocateDescriptorSets(&allocationInfo, &descriptorSet)))
    {
        return;
    }

    initialized = true;
}

LightBaker::~LightBaker()
{
    lunaDestroyInstance();
}

bool LightBaker::bake(const std::unordered_map<std::string, LevelMeshBuilder> &meshBuilders,
                      const std::vector<Light> &lights,
                      std::vector<uint8_t> &pixelData,
                      const glm::ivec2 &lightmapSize)
{
    // This is checks that MapVertex and Light are both POD, which is required to directly write from the pointer to the buffer.
    //  If they are not POD then the data will not be properly packed in memory.
    static_assert(std::is_standard_layout_v<MapVertex> &&
                  std::is_trivially_default_constructible_v<MapVertex> &&
                  std::is_trivially_copyable_v<MapVertex>);
    static_assert(std::is_standard_layout_v<Light> &&
                  std::is_trivially_default_constructible_v<Light> &&
                  std::is_trivially_copyable_v<Light>);

    pixelData.clear();

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

    std::ifstream glslFile("assets/shaders/lightmap_direct_lighting_pass.comp");
    std::stringstream glsl;
    glsl << glslFile.rdbuf();
    glslFile.close();
    const ShaderCompiler shaderCompiler(glsl.str(),
                                        vk::ShaderStageFlagBits::eCompute,
                                        glslang::EShTargetClientVersion::EShTargetVulkan_1_2);
    std::vector<uint32_t> spirv;
    if (shaderCompiler.Compile(spirv) != Error::ErrorCode::OK)
    {
        printf("Error compiling shader!\n");
        return false;
    }

    const LunaShaderModuleCreationInfo shaderModuleCreationInfo = {
        .creationInfoType = LUNA_SHADER_MODULE_CREATION_INFO_TYPE_SPIRV,
        .creationInfoUnion = {.spirv = {.size = spirv.size() * sizeof(uint32_t), .spirv = spirv.data()}},
    };
    if (!CheckResult(lunaCreateShaderModule(&shaderModuleCreationInfo, &shaderModule)))
    {
        return false;
    }

    const std::array<uint32_t, 6> specializationData = {
        static_cast<uint32_t>(lightmapSize.x),
        static_cast<uint32_t>(lightmapSize.y),
        static_cast<uint32_t>(indices.size()),
        32,
        32,
        1,
    };
    constexpr std::array<VkSpecializationMapEntry, 6> mapEntries = {
        VkSpecializationMapEntry{
            .constantID = 0,
            .offset = 0 * sizeof(uint32_t),
            .size = sizeof(uint32_t),
        },
        VkSpecializationMapEntry{
            .constantID = 1,
            .offset = 1 * sizeof(uint32_t),
            .size = sizeof(uint32_t),
        },
        VkSpecializationMapEntry{
            .constantID = 2,
            .offset = 2 * sizeof(uint32_t),
            .size = sizeof(uint32_t),
        },
        VkSpecializationMapEntry{
            .constantID = 3,
            .offset = 3 * sizeof(uint32_t),
            .size = sizeof(uint32_t),
        },
        VkSpecializationMapEntry{
            .constantID = 4,
            .offset = 4 * sizeof(uint32_t),
            .size = sizeof(uint32_t),
        },
        VkSpecializationMapEntry{
            .constantID = 5,
            .offset = 5 * sizeof(uint32_t),
            .size = sizeof(uint32_t),
        },
    };
    const VkSpecializationInfo specializationInfo = {
        .mapEntryCount = mapEntries.size(),
        .pMapEntries = mapEntries.data(),
        .dataSize = sizeof(specializationData),
        .pData = specializationData.data(),
    };
    const LunaPipelineShaderStageCreationInfo shaderStageCreationInfo = {
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = shaderModule,
        .specializationInfo = &specializationInfo,
    };
    const LunaPipelineLayoutCreationInfo layoutCreationInfo = {
        .descriptorSetLayoutCount = 1,
        .descriptorSetLayouts = &descriptorSetLayout,
    };
    const LunaComputePipelineCreationInfo pipelineCreationInfo = {
        .shaderStageCreationInfo = shaderStageCreationInfo,
        .layoutCreationInfo = layoutCreationInfo,
    };
    if (!CheckResult(lunaCreateComputePipeline(&pipelineCreationInfo, &pipeline)))
    {
        return false;
    }

    const size_t vertexBufferByteCount = vertices.size() * sizeof(MapVertex);
    const size_t indexBufferByteCount = indices.size() * sizeof(uint32_t);
    if (vertexBufferByteCount == 0 || indexBufferByteCount == 0)
    {
        assert(vertexBufferByteCount == 0 && indexBufferByteCount == 0);
        return false;
    }
    if (!CheckResult(lunaResizeBuffer(&vertexBuffer, vertexBufferByteCount)))
    {
        return false;
    }
    if (!CheckResult(lunaResizeBuffer(&indexBuffer, indexBufferByteCount)))
    {
        return false;
    }

    LunaBuffer lightsBuffer{};
    const LunaBufferCreationInfo lightsBufferCreationInfo = {
        .size = sizeof(Light) * lights.size(),
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    };
    lunaCreateBuffer(&lightsBufferCreationInfo, &lightsBuffer);

    const LunaBufferWriteInfo vertexBufferWriteInfo = {
        .bytes = vertexBufferByteCount,
        .data = vertices.data(),
    };
    if (!CheckResult(lunaWriteDataToBuffer(vertexBuffer, &vertexBufferWriteInfo)))
    {
        return false;
    }
    const LunaBufferWriteInfo indexBufferWriteInfo = {
        .bytes = indexBufferByteCount,
        .data = indices.data(),
    };
    if (!CheckResult(lunaWriteDataToBuffer(indexBuffer, &indexBufferWriteInfo)))
    {
        return false;
    }
    const LunaBufferWriteInfo lightsBufferWriteInfo = {
        .bytes = sizeof(Light) * lights.size(),
        .data = lights.data(),
    };
    if (!CheckResult(lunaWriteDataToBuffer(lightsBuffer, &lightsBufferWriteInfo)))
    {
        return false;
    }

    const LunaBufferCreationInfo levelGeometryLightmapCreationInfo = {
        .size = static_cast<VkDeviceSize>(lightmapSize.x * lightmapSize.y * sizeof(_Float16) * 4),
        .usage = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
    };
    lunaCreateBuffer(&levelGeometryLightmapCreationInfo, &levelGeometryLightmap);

    void *asd = lunaGetBufferDataPointer(levelGeometryLightmap);
    memset(asd, 0, static_cast<VkDeviceSize>(lightmapSize.x * lightmapSize.y * sizeof(_Float16) * 4));

    const LunaBufferViewCreationInfo levelGeometryLightmapBufferViewCreationInfo = {
        .buffer = levelGeometryLightmap,
        .format = VK_FORMAT_R16G16B16A16_SFLOAT,
    };
    const std::array<LunaWriteDescriptorSet, 4> writes = {
        LunaWriteDescriptorSet{
            .descriptorSet = descriptorSet,
            .bindingName = "vertices",
            .bufferInfo = vertexBuffer,
        },
        LunaWriteDescriptorSet{
            .descriptorSet = descriptorSet,
            .bindingName = "indices",
            .bufferInfo = indexBuffer,
        },
        LunaWriteDescriptorSet{
            .descriptorSet = descriptorSet,
            .bindingName = "lights",
            .bufferInfo = lightsBuffer,
        },
        LunaWriteDescriptorSet{
            .descriptorSet = descriptorSet,
            .bindingName = "level geometry lightmap",
            .texelBufferView = &levelGeometryLightmapBufferViewCreationInfo,
        },
    };
    lunaWriteDescriptorSets(writes.size(), writes.data());

    const LunaDescriptorSetBindInfo descriptorSetBindInfo = {
        .descriptorSetCount = 1,
        .descriptorSets = &descriptorSet,
    };
    const LunaDispatchComputeInfo dispatchComputeInfo = {
        .pipeline = pipeline,
        .descriptorSetBindInfo = descriptorSetBindInfo,
        .groupCountX = static_cast<uint32_t>(lights.size()),
        .groupCountY = 65535,
        .groupCountZ = 65535,
        .submitCommandBuffer = true,
    };
    if (!CheckResult(lunaDispatchCompute(&dispatchComputeInfo)))
    {
        return false;
    }

    lunaDeviceWaitIdle();

    uint8_t *bufferData = static_cast<uint8_t *>(lunaGetBufferDataPointer(levelGeometryLightmap));
    pixelData.resize(lunaGetBufferSize(levelGeometryLightmap));
    std::copy_n(bufferData, pixelData.size(), pixelData.data());

    // TODO: Cleanup
    return true;
}

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

float getLightStrength(float distance, Light light) {
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
        if (intersection.lightStrength < EPSILON) {
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

    imageStore(outputLightmap, computeLuxelIndex(closestIntersection.triangle, closestIntersection.intersection), closestIntersection.lightStrength * vec4(light.color, 1));
}
} // namespace converted_glsl

bool LightBaker::bakeCPU(const std::unordered_map<std::string, LevelMeshBuilder> &meshBuilders,
                         const std::vector<Light> &lights,
                         std::vector<uint8_t> &pixelData,
                         const glm::ivec2 &lightmapSize)
{
    static_assert(std::is_standard_layout_v<MapVertex> &&
                  std::is_trivially_default_constructible_v<MapVertex> &&
                  std::is_trivially_copyable_v<MapVertex>);
    static_assert(std::is_standard_layout_v<Light> &&
                  std::is_trivially_default_constructible_v<Light> &&
                  std::is_trivially_copyable_v<Light>);

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

    return true;
}
