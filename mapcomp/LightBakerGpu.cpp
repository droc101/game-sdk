//
// Created by NBT22 on 3/20/26.
//

#include "LightBakerGpu.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <glm/geometric.hpp>
#include <glslang/Public/ShaderLang.h>
#include <iostream>
#include <libassets/type/MapVertex.h>
#include <libassets/util/Error.h>
#include <libassets/util/Logger.h>
#include <libassets/util/ShaderCompiler.h>
#include <luna/luna.h>
#include <luna/lunaBuffer.h>
#include <luna/lunaDevice.h>
#include <luna/lunaInstance.h>
#include <luna/lunaTypes.h>
#include <ranges>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include "LevelMeshBuilder.h"
#include "Light.h"

// These are up here for me to be able to easily edit them, but they can be moved elsewhere later
static constexpr glm::uvec2 WORK_GROUP_SIZE{32};
static constexpr glm::uvec2 NUM_WORK_GROUPS{256};
static constexpr uint32_t DISPATCH_COUNT = 16;

template<typename... T>
static consteval std::array<VkSpecializationMapEntry, sizeof...(T)> generateSpecializationMapEntries()
{
    size_t index = 0;
    std::array<VkSpecializationMapEntry, sizeof...(T)> entries{};

    auto AddEntry = [&index, &entries]<typename Type>() {
        entries.at(index) = VkSpecializationMapEntry{
            .constantID = static_cast<Type>(index),
            .offset = static_cast<Type>(index * sizeof(Type)),
            .size = sizeof(Type),
        };
        index++;
    };
    (AddEntry.template operator()<T>(), ...);
    return entries;
}

LightBakerGpu::LightBakerGpu()
{
    constexpr LunaInstanceCreationInfo instanceCreationInfo = {
        .apiVersion = VK_API_VERSION_1_2,
#ifndef NDEBUG
        .enableValidation = true,
#endif
    };
    if (!checkResult(lunaCreateInstance(&instanceCreationInfo)))
    {
        return;
    }

    constexpr LunaPhysicalDevicePreferenceDefinition physicalDevicePreferenceDefinition = {
        .preferredDeviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
    };
    constexpr VkPhysicalDeviceFeatures required10Features = {
        .shaderFloat64 = VK_TRUE,
    };
    VkPhysicalDeviceVulkan12Features required12Features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .uniformAndStorageBuffer8BitAccess = VK_TRUE,
        .shaderInt8 = VK_TRUE,
    };
    const VkPhysicalDeviceFeatures2 requiredFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &required12Features,
        .features = required10Features,
    };
    const LunaDeviceCreationInfo2 deviceCreationInfo = {
        .requiredFeatures = requiredFeatures,
        .physicalDevicePreferenceDefinition = &physicalDevicePreferenceDefinition,
    };
    if (!checkResult(lunaCreateDevice2(&deviceCreationInfo)))
    {
        return;
    }

    initialized = true;
}

LightBakerGpu::~LightBakerGpu()
{
    lunaDestroyInstance();
}

bool LightBakerGpu::bake(const std::unordered_map<std::string, LevelMeshBuilder> &meshBuilders,
                         const std::vector<Light> &lights,
                         const glm::ivec2 &lightmapSize,
                         std::vector<uint8_t> &pixelData)
{
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

    const size_t vertexBufferByteCount = vertices.size() * sizeof(MapVertex);
    const size_t indexBufferByteCount = indices.size() * sizeof(uint32_t);
    if (vertexBufferByteCount == 0 || indexBufferByteCount == 0)
    {
        assert(vertexBufferByteCount == 0 && indexBufferByteCount == 0);
        return false;
    }

    // TODO: With the stable release of Luna 0.3.0 these can be moved to be two regions within one buffer
    const LunaBufferCreationInfo vertexBufferCreationInfo = {
        .size = vertexBufferByteCount,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    };
    if (!checkResult(lunaCreateBuffer(&vertexBufferCreationInfo, &vertexBuffer)))
    {
        return false;
    }
    const LunaBufferCreationInfo indexBufferCreationInfo = {
        .size = indexBufferByteCount,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    };
    if (!checkResult(lunaCreateBuffer(&indexBufferCreationInfo, &indexBuffer)))
    {
        return false;
    }

    const LunaBufferWriteInfo vertexBufferWriteInfo = {
        .bytes = vertexBufferByteCount,
        .data = vertices.data(),
        .stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    };
    if (!checkResult(lunaWriteDataToBuffer(vertexBuffer, &vertexBufferWriteInfo)))
    {
        return false;
    }
    const LunaBufferWriteInfo indexBufferWriteInfo = {
        .bytes = indexBufferByteCount,
        .data = indices.data(),
        .stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    };
    if (!checkResult(lunaWriteDataToBuffer(indexBuffer, &indexBufferWriteInfo)))
    {
        return false;
    }

    VkPhysicalDeviceProperties properties{};
    lunaGetPhysicalDeviceProperties(&properties);
    for (LunaBuffer &pixelIndicesBuffer: pixelIndicesBuffers)
    {
        size_t bufferSize = sizeof(uint32_t) + sizeof(int) * lightmapSize.x * lightmapSize.y;
        if (bufferSize > properties.limits.maxStorageBufferRange)
        {
            bufferSize--;
            if (bufferSize > properties.limits.maxStorageBufferRange)
            {
                Logger::Error("Unable to allocate shader storage buffer of size {}", bufferSize);
                return false;
            }
        }
        const LunaBufferCreationInfo pixelIndicesBufferCreationInfo = {
            .size = bufferSize,
            .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        };
        lunaCreateBuffer(&pixelIndicesBufferCreationInfo, &pixelIndicesBuffer);
    }

    const LunaBufferCreationInfo hitIndicesBufferCreationInfo = {
        .size = lightmapSize.x * lightmapSize.y * sizeof(uint32_t),
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    };
    if (!checkResult(lunaCreateBuffer(&hitIndicesBufferCreationInfo, &hitIndicesBuffer)))
    {
        return false;
    }

    constexpr VmaAllocationCreateInfo allocationCreateInfo = {
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
    };
    const LunaBufferCreationInfo levelGeometryLightmapCreationInfo = {
        .size = lightmapSize.x * lightmapSize.y * sizeof(float) * 3,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .allocationCreateInfo = &allocationCreateInfo,
    };
    if (!checkResult(lunaCreateBuffer(&levelGeometryLightmapCreationInfo, &lightmap2d)))
    {
        return false;
    }

    uint8_t *bufferData = static_cast<uint8_t *>(lunaGetBufferDataPointer(lightmap2d));
    memset(bufferData, 0, lightmapSize.x * lightmapSize.y * sizeof(float) * 3);

    if (!bakeDirectLighting(lights, lightmapSize, indices.size()))
    {
        return false;
    }

    lunaDeviceWaitIdle();
    if (!bakeBounceLighting(lightmapSize, indices.size(), 1))
    {
        return false;
    }

    lunaDeviceWaitIdle();
    LunaBuffer outputLightmap2d{};
    if (!convertLightmapToFloat16(lightmapSize, outputLightmap2d))
    {
        return false;
    }

    lunaDeviceWaitIdle();
    bufferData = static_cast<uint8_t *>(lunaGetBufferDataPointer(outputLightmap2d));
    pixelData.resize(lunaGetBufferSize(outputLightmap2d));
    std::copy_n(bufferData, pixelData.size(), pixelData.data());
    return true;
}

bool LightBakerGpu::createPipeline(const std::string &shader,
                                   const VkSpecializationInfo &specializationInfo,
                                   const std::vector<LunaPushConstantsRange> &pushConstantRanges,
                                   const LunaDescriptorSetLayoutCreationInfo &descriptorSetLayoutCreationInfo,
                                   const LunaDescriptorPoolCreationInfo &descriptorPoolCreationInfo,
                                   LunaDescriptorSet &descriptorSet,
                                   LunaComputePipeline &pipeline)
{
    std::ifstream glslFile(shader);
    std::stringstream glsl;
    glsl << glslFile.rdbuf();
    glslFile.close();
    const ShaderCompiler shaderCompiler(glsl.str(),
                                        vk::ShaderStageFlagBits::eCompute,
                                        glslang::EShTargetClientVersion::EShTargetVulkan_1_2);
    std::vector<uint32_t> spirv;
    if (shaderCompiler.Compile(spirv) != Error::ErrorCode::OK)
    {
        Logger::Error("Error compiling shader!");
        return false;
    }

    LunaShaderModule shaderModule{};
    const LunaShaderModuleCreationInfo shaderModuleCreationInfo = {
        .creationInfoType = LUNA_SHADER_MODULE_CREATION_INFO_TYPE_SPIRV,
        .creationInfoUnion = {.spirv = {.size = spirv.size() * sizeof(uint32_t), .spirv = spirv.data()}},
    };
    if (!checkResult(lunaCreateShaderModule(&shaderModuleCreationInfo, &shaderModule)))
    {
        return false;
    }

    LunaDescriptorSetLayout descriptorSetLayout{};
    if (!checkResult(lunaCreateDescriptorSetLayout(&descriptorSetLayoutCreationInfo, &descriptorSetLayout)))
    {
        return false;
    }

    const LunaPipelineShaderStageCreationInfo shaderStageCreationInfo = {
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = shaderModule,
        .specializationInfo = &specializationInfo,
    };
    const LunaPipelineLayoutCreationInfo layoutCreationInfo = {
        .descriptorSetLayoutCount = 1,
        .descriptorSetLayouts = &descriptorSetLayout,
        .pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size()),
        .pushConstantsRanges = pushConstantRanges.data(),
    };
    const LunaComputePipelineCreationInfo pipelineCreationInfo = {
        .flags = VK_PIPELINE_CREATE_DISPATCH_BASE,
        .shaderStageCreationInfo = shaderStageCreationInfo,
        .layoutCreationInfo = layoutCreationInfo,
    };
    if (!checkResult(lunaCreateComputePipeline(&pipelineCreationInfo, &pipeline)))
    {
        return false;
    }

    LunaDescriptorPool descriptorPool{};
    if (!checkResult(lunaCreateDescriptorPool(&descriptorPoolCreationInfo, &descriptorPool)))
    {
        return false;
    }

    const LunaDescriptorSetAllocationInfo allocationInfo = {
        .descriptorPool = descriptorPool,
        .setLayoutCount = 1,
        .setLayouts = &descriptorSetLayout,
    };
    return checkResult(lunaAllocateDescriptorSets(&allocationInfo, &descriptorSet));
}

bool LightBakerGpu::bakeDirectLighting(const std::vector<Light> &lights,
                                       const glm::ivec2 &lightmapSize,
                                       const size_t indexCount) const
{
    LunaDescriptorSet descriptorSet{};
    LunaComputePipeline pipeline{};
    const std::array<uint32_t, 6> specializationData = {
        static_cast<uint32_t>(lightmapSize.x),
        static_cast<uint32_t>(lightmapSize.y),
        static_cast<uint32_t>(indexCount),
        DISPATCH_COUNT,
        WORK_GROUP_SIZE.x,
        WORK_GROUP_SIZE.y,
    };
    constexpr std::array<VkSpecializationMapEntry, 6>
            mapEntries = generateSpecializationMapEntries<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>();
    const VkSpecializationInfo specializationInfo = {
        .mapEntryCount = mapEntries.size(),
        .pMapEntries = mapEntries.data(),
        .dataSize = sizeof(specializationData),
        .pData = specializationData.data(),
    };
    constexpr std::array<LunaDescriptorSetLayoutBinding, 6> descriptorSetLayoutBindings{
        LunaDescriptorSetLayoutBinding{
            .bindingName = "vertices",
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
        LunaDescriptorSetLayoutBinding{
            .bindingName = "indices",
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
        LunaDescriptorSetLayoutBinding{
            .bindingName = "lights",
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
        LunaDescriptorSetLayoutBinding{
            .bindingName = "pixel indices",
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
        LunaDescriptorSetLayoutBinding{
            .bindingName = "hit indices",
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
        LunaDescriptorSetLayoutBinding{
            .bindingName = "lightmap2d",
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
    };
    const LunaDescriptorSetLayoutCreationInfo descriptorSetLayoutCreationInfo = {
        .bindingCount = descriptorSetLayoutBindings.size(),
        .bindings = descriptorSetLayoutBindings.data(),
    };
    constexpr std::array<VkDescriptorPoolSize, 1> descriptorPoolSizes = {
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 6,
        },
    };
    const LunaDescriptorPoolCreationInfo descriptorPoolCreationInfo = {
        .maxSets = 1,
        .poolSizeCount = descriptorPoolSizes.size(),
        .poolSizes = descriptorPoolSizes.data(),
    };
    if (!createPipeline("assets/shaders/lightmap_direct_lighting_pass.comp",
                        specializationInfo,
                        {},
                        descriptorSetLayoutCreationInfo,
                        descriptorPoolCreationInfo,
                        descriptorSet,
                        pipeline))
    {
        return false;
    }

    LunaBuffer lightsBuffer{};
    const LunaBufferCreationInfo lightsBufferCreationInfo = {
        .size = sizeof(Light) * lights.size(),
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    };
    lunaCreateBuffer(&lightsBufferCreationInfo, &lightsBuffer);

    const LunaBufferWriteInfo lightsBufferWriteInfo = {
        .bytes = sizeof(Light) * lights.size(),
        .data = lights.data(),
        .stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    };
    if (!checkResult(lunaWriteDataToBuffer(lightsBuffer, &lightsBufferWriteInfo)))
    {
        return false;
    }

    const std::array<LunaWriteDescriptorSet, 6> writes = {
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
            .bindingName = "pixel indices",
            .bufferInfo = pixelIndicesBuffers.at(0),
        },
        LunaWriteDescriptorSet{
            .descriptorSet = descriptorSet,
            .bindingName = "hit indices",
            .bufferInfo = hitIndicesBuffer,
        },
        LunaWriteDescriptorSet{
            .descriptorSet = descriptorSet,
            .bindingName = "lightmap2d",
            .bufferInfo = lightmap2d,
        },
    };
    lunaWriteDescriptorSets(writes.size(), writes.data());

    const LunaDescriptorSetBindInfo descriptorSetBindInfo = {
        .descriptorSetCount = 1,
        .descriptorSets = &descriptorSet,
    };
    for (uint32_t i = 0; i < DISPATCH_COUNT; i++)
    {
        const LunaDispatchBaseInfo dispatchInfo = {
            .pipeline = pipeline,
            .descriptorSetBindInfo = descriptorSetBindInfo,
            .baseGroupX = NUM_WORK_GROUPS.x * i,
            .groupCountX = NUM_WORK_GROUPS.x,
            .groupCountY = NUM_WORK_GROUPS.y,
            .groupCountZ = static_cast<uint32_t>(lights.size()),
            .submitCommandBuffer = true,
        };
        if (!checkResult(lunaDispatchBase(&dispatchInfo)))
        {
            return false;
        }
    }

    return true;
}

bool LightBakerGpu::bakeBounceLighting(const glm::ivec2 &lightmapSize,
                                       const size_t indexCount,
                                       const uint32_t bounceCount)
{
    LunaDescriptorSet descriptorSet{};
    LunaComputePipeline pipeline{};
    const std::array<uint32_t, 6> specializationData = {
        static_cast<uint32_t>(lightmapSize.x),
        static_cast<uint32_t>(lightmapSize.y),
        static_cast<uint32_t>(indexCount),
        1,
        1,
        1,
    };
    constexpr std::array<VkSpecializationMapEntry, 6>
            mapEntries = generateSpecializationMapEntries<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>();
    const VkSpecializationInfo specializationInfo = {
        .mapEntryCount = mapEntries.size(),
        .pMapEntries = mapEntries.data(),
        .dataSize = sizeof(specializationData),
        .pData = specializationData.data(),
    };
    constexpr std::array<LunaDescriptorSetLayoutBinding, 5> descriptorSetLayoutBindings{
        LunaDescriptorSetLayoutBinding{
            .bindingName = "vertices",
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
        LunaDescriptorSetLayoutBinding{
            .bindingName = "indices",
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
        LunaDescriptorSetLayoutBinding{
            .bindingName = "pixel indices",
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 2,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
        LunaDescriptorSetLayoutBinding{
            .bindingName = "hit indices",
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
        LunaDescriptorSetLayoutBinding{
            .bindingName = "lightmap2d",
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
    };
    const LunaDescriptorSetLayoutCreationInfo descriptorSetLayoutCreationInfo = {
        .bindingCount = descriptorSetLayoutBindings.size(),
        .bindings = descriptorSetLayoutBindings.data(),
    };
    constexpr std::array<VkDescriptorPoolSize, 1> descriptorPoolSizes = {
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 6,
        },
    };
    const LunaDescriptorPoolCreationInfo descriptorPoolCreationInfo = {
        .maxSets = 1,
        .poolSizeCount = descriptorPoolSizes.size(),
        .poolSizes = descriptorPoolSizes.data(),
    };
    if (!createPipeline("assets/shaders/lightmap_bounce_lighting_pass.comp",
                        specializationInfo,
                        {},
                        descriptorSetLayoutCreationInfo,
                        descriptorPoolCreationInfo,
                        descriptorSet,
                        pipeline))
    {
        return false;
    }

    const std::array<LunaWriteDescriptorSet, 6> writes = {
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
            .bindingName = "pixel indices",
            .descriptorArrayElement = 0,
            .bufferInfo = pixelIndicesBuffers.at(0),
        },
        LunaWriteDescriptorSet{
            .descriptorSet = descriptorSet,
            .bindingName = "pixel indices",
            .descriptorArrayElement = 1,
            .bufferInfo = pixelIndicesBuffers.at(1),
        },
        LunaWriteDescriptorSet{
            .descriptorSet = descriptorSet,
            .bindingName = "hit indices",
            .bufferInfo = hitIndicesBuffer,
        },
        LunaWriteDescriptorSet{
            .descriptorSet = descriptorSet,
            .bindingName = "lightmap2d",
            .bufferInfo = lightmap2d,
        },
    };
    lunaWriteDescriptorSets(writes.size(), writes.data());

    for (uint32_t i = 0; i < bounceCount; i++)
    {
        const uint32_t pixelIndexCount = *static_cast<uint32_t *>(lunaGetBufferDataPointer(pixelIndicesBuffers.at(0)));
        Logger::Verbose("{}", pixelIndexCount);
        const LunaDescriptorSetBindInfo descriptorSetBindInfo = {
            .descriptorSetCount = 1,
            .descriptorSets = &descriptorSet,
        };
        const float sqrtPixelIndexCount = std::sqrt(pixelIndexCount);
        const uint32_t y = std::floor(sqrtPixelIndexCount);
        const uint32_t z = std::ceil(sqrtPixelIndexCount);
        const LunaDispatchInfo dispatchInfo = {
            .pipeline = pipeline,
            .descriptorSetBindInfo = descriptorSetBindInfo,
            .groupCountX = 1,
            .groupCountY = y,
            .groupCountZ = z,
            .submitCommandBuffer = true,
        };
        if (!checkResult(lunaDispatch(&dispatchInfo)))
        {
            return false;
        }
        // const LunaDispatchBaseInfo dispatchBaseInfo = {
        //     .pipeline = pipeline,
        //     .descriptorSetBindInfo = descriptorSetBindInfo,
        //     .baseGroupY = y,
        //     .baseGroupZ = z,
        //     .groupCountX = 1,
        //     .groupCountY = 1,
        //     .groupCountZ = pixelIndexCount % z,
        //     .submitCommandBuffer = true,
        // };
        // if (!checkResult(lunaDispatchBase(&dispatchBaseInfo)))
        // {
        //     return false;
        // }

        // TODO: Once Luna 0.3.0 is finished, use lunaCopyBufferToBuffer
        const uint32_t *bufferData = static_cast<uint32_t *>(lunaGetBufferDataPointer(pixelIndicesBuffers.at(1)));
        assert(bufferData);
        const LunaBufferWriteInfo writeInfo = {
            .bytes = sizeof(uint32_t),
            .data = bufferData,
            .stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        };
        if (!checkResult(lunaWriteDataToBuffer(pixelIndicesBuffers.at(0), &writeInfo)))
        {
            return false;
        }
    }
    return true;
}

bool LightBakerGpu::convertLightmapToFloat16(const glm::ivec2 &lightmapSize, LunaBuffer &outputLightmap2d) const
{
    LunaDescriptorSet descriptorSet{};
    LunaComputePipeline pipeline{};
    const std::array<uint32_t, 3> specializationData = {
        static_cast<uint32_t>(lightmapSize.x),
        32,
        32,
    };
    constexpr std::array<VkSpecializationMapEntry, 3>
            mapEntries = generateSpecializationMapEntries<uint32_t, uint32_t, uint32_t>();
    const VkSpecializationInfo specializationInfo = {
        .mapEntryCount = mapEntries.size(),
        .pMapEntries = mapEntries.data(),
        .dataSize = sizeof(specializationData),
        .pData = specializationData.data(),
    };
    constexpr std::array<LunaDescriptorSetLayoutBinding, 2> descriptorSetLayoutBindings{
        LunaDescriptorSetLayoutBinding{
            .bindingName = "input lightmap",
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
        LunaDescriptorSetLayoutBinding{
            .bindingName = "output lightmap",
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
    };
    const LunaDescriptorSetLayoutCreationInfo descriptorSetLayoutCreationInfo = {
        .bindingCount = descriptorSetLayoutBindings.size(),
        .bindings = descriptorSetLayoutBindings.data(),
    };
    constexpr std::array<VkDescriptorPoolSize, 2> descriptorPoolSizes = {
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
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
    if (!createPipeline("assets/shaders/lightmap_convert_to_float16.comp",
                        specializationInfo,
                        {},
                        descriptorSetLayoutCreationInfo,
                        descriptorPoolCreationInfo,
                        descriptorSet,
                        pipeline))
    {
        return false;
    }

    constexpr VmaAllocationCreateInfo allocationCreateInfo = {
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
    };
    const LunaBufferCreationInfo levelGeometryLightmapCreationInfo = {
        .size = lightmapSize.x * lightmapSize.y * sizeof(_Float16) * 4,
        .usage = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
        .allocationCreateInfo = &allocationCreateInfo,
    };
    if (!checkResult(lunaCreateBuffer(&levelGeometryLightmapCreationInfo, &outputLightmap2d)))
    {
        return false;
    }

    LunaBufferView outputLightmap2dBufferView{};
    const LunaBufferViewCreationInfo outputLightmap2dBufferViewCreationInfo = {
        .buffer = outputLightmap2d,
        .format = VK_FORMAT_R16G16B16A16_SFLOAT,
    };
    if (!checkResult(lunaCreateBufferView(&outputLightmap2dBufferViewCreationInfo, &outputLightmap2dBufferView)))
    {
        return false;
    }

    const std::array<LunaWriteDescriptorSet, 2> writes = {
        LunaWriteDescriptorSet{
            .descriptorSet = descriptorSet,
            .bindingName = "input lightmap",
            .bufferInfo = lightmap2d,
        },
        LunaWriteDescriptorSet{
            .descriptorSet = descriptorSet,
            .bindingName = "output lightmap",
            .texelBufferView = outputLightmap2dBufferView,
        },
    };
    lunaWriteDescriptorSets(writes.size(), writes.data());

    const LunaDescriptorSetBindInfo descriptorSetBindInfo = {
        .descriptorSetCount = 1,
        .descriptorSets = &descriptorSet,
    };
    assert(lightmapSize.x % 32 == 0 && lightmapSize.y % 32 == 0);
    const glm::uvec2 numGroups = lightmapSize / 32;
    const LunaDispatchInfo dispatchInfo = {
        .pipeline = pipeline,
        .descriptorSetBindInfo = descriptorSetBindInfo,
        .groupCountX = numGroups.x,
        .groupCountY = numGroups.y,
        .submitCommandBuffer = true,
    };
    return checkResult(lunaDispatch(&dispatchInfo));
}
