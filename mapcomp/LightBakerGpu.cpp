//
// Created by NBT22 on 3/20/26.
//

#include "LightBakerGpu.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <glm/geometric.hpp>
#include <glslang/Public/ShaderLang.h>
#include <libassets/type/MapVertex.h>
#include <libassets/util/Error.h>
#include <libassets/util/Logger.h>
#include <libassets/util/ShaderCompiler.h>
#include <limits>
#include <luna/luna.h>
#include <luna/lunaBuffer.h>
#include <luna/lunaDevice.h>
#include <luna/lunaInstance.h>
#include <luna/lunaTypes.h>
#include <ranges>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include "LevelMeshBuilder.h"
#include "Light.h"

// These are up here for me to be able to easily edit them, but they can be moved elsewhere later
static constexpr glm::uvec2 WORK_GROUP_SIZE{32};
static constexpr glm::uvec2 NUM_WORK_GROUPS{128};
static constexpr uint32_t DISPATCH_COUNT = 128;

// Ensure that the total ray count does not overflow
static_assert(static_cast<uint64_t>(WORK_GROUP_SIZE.y * NUM_WORK_GROUPS.y + 1) *
                      WORK_GROUP_SIZE.x *
                      NUM_WORK_GROUPS.x *
                      DISPATCH_COUNT <
              std::numeric_limits<uint32_t>::max());

/// This function is evil
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

/// This function is about 10x more evil than generateSpecializationMapEntries
template<typename... T> requires(((std::__is_pair<T> &&
                                   std::is_same_v<typename T::first_type, VkDescriptorType> &&
                                   std::is_same_v<typename T::second_type::value_type, const char *>) &&
                                  ...))
static consteval auto generateDescriptorSetLayoutBindings(const T &...bindings)
{
    size_t index = 0;
    std::array<LunaDescriptorSetLayoutBinding,
               ((sizeof(T::second_type::_M_elems) / sizeof(typename T::second_type::value_type)) + ...)>
            entries{};

    auto addEntry = [&index, &entries]<typename Type>(const Type &binding) {
        for (const char *name: binding.second)
        {
            entries.at(index++) = LunaDescriptorSetLayoutBinding{
                .bindingName = name,
                .descriptorType = binding.first,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            };
        }
    };
    (addEntry(bindings), ...);

    return entries;
}

/// Oh my god why can this even exist let alone work
template<const auto *BINDINGS> static consteval LunaDescriptorPoolCreationInfo generateDescriptorPoolCreationInfo()
{
    static constexpr auto UNIQUE_BINDINGS = std::ranges::
            unique((const_cast<std::array<LunaDescriptorSetLayoutBinding, BINDINGS->size()> *>(BINDINGS))->begin(),
                   const_cast<std::array<LunaDescriptorSetLayoutBinding, BINDINGS->size()> *>(BINDINGS)->end(),
                   [](const LunaDescriptorSetLayoutBinding &a, const LunaDescriptorSetLayoutBinding &b) -> bool {
                       return a.descriptorType != b.descriptorType;
                   });
    static constexpr std::array<VkDescriptorPoolSize, UNIQUE_BINDINGS.size() + 1> ret = ([]() {
        std::array<VkDescriptorPoolSize, UNIQUE_BINDINGS.size() + 1> ret{};
        for (const LunaDescriptorSetLayoutBinding &binding: *BINDINGS)
        {
            for (VkDescriptorPoolSize &poolSize: ret)
            {
                if (poolSize.descriptorCount == 0)
                {
                    poolSize.type = binding.descriptorType;
                }
                if (poolSize.type == binding.descriptorType)
                {
                    ++poolSize.descriptorCount;
                    break;
                }
            }
        }
        return ret;
    })();
    return LunaDescriptorPoolCreationInfo{
        .maxSets = 1,
        .poolSizeCount = ret.size(),
        .poolSizes = ret.data(),
    };
}

template<typename... T> requires(((std::__is_pair<T> &&
                                   std::is_same_v<typename T::first_type, const char *> &&
                                   (std::is_same_v<typename T::second_type, const LunaDescriptorImageInfo *> ||
                                    std::is_same_v<typename T::second_type, LunaBuffer> ||
                                    std::is_same_v<typename T::second_type, const LunaDescriptorBufferInfo *> ||
                                    std::is_same_v<typename T::second_type, LunaBufferView *>)) &&
                                  ...))
static inline std::array<LunaWriteDescriptorSet, sizeof...(T)> generateWrites(const LunaDescriptorSet descriptorSet,
                                                                              std::list<LunaDescriptorBufferInfo>
                                                                                      &bufferInfos,
                                                                              const T &...writes)
{
    size_t index = 0;
    std::array<LunaWriteDescriptorSet, sizeof...(T)> entries{};

    auto AddEntry = [&descriptorSet, &index, &entries, &bufferInfos]<typename Type>(const Type &write) {
        if constexpr (std::is_same_v<typename Type::second_type, const LunaDescriptorImageInfo *>)
        {
            entries.at(index) = LunaWriteDescriptorSet{
                .descriptorSet = descriptorSet,
                .bindingName = write.first,
                .imageInfo = write.second,
            };
        } else if constexpr (std::is_same_v<typename Type::second_type, LunaBuffer>)
        {
            bufferInfos.emplace_back(write.second, 0, 0);
            entries.at(index) = LunaWriteDescriptorSet{
                .descriptorSet = descriptorSet,
                .bindingName = write.first,
                .bufferInfo = &bufferInfos.back(),
            };
        } else if constexpr (std::is_same_v<typename Type::second_type, const LunaDescriptorBufferInfo *>)
        {
            entries.at(index) = LunaWriteDescriptorSet{
                .descriptorSet = descriptorSet,
                .bindingName = write.first,
                .bufferInfo = write.second,
            };
        } else if constexpr (std::is_same_v<typename Type::second_type, LunaBufferView *>)
        {
            entries.at(index) = LunaWriteDescriptorSet{
                .descriptorSet = descriptorSet,
                .bindingName = write.first,
                .texelBufferView = *write.second,
            };
        } else
        {
            static_assert(false, "How did you manage to get an invalid type past the requires clause?!?!?");
        }
        index++;
    };
    (AddEntry(writes), ...);

    return entries;
}

template<typename... T> requires(((std::__is_pair<T> &&
                                   std::is_same_v<typename T::first_type, const char *> &&
                                   (std::is_same_v<typename T::second_type, const LunaDescriptorImageInfo *> ||
                                    std::is_same_v<typename T::second_type, LunaBuffer> ||
                                    std::is_same_v<typename T::second_type, const LunaDescriptorBufferInfo *> ||
                                    std::is_same_v<typename T::second_type, LunaBufferView *>)) &&
                                  ...) &&
                                 (!std::is_same_v<typename T::second_type, LunaBuffer> && ...))
static inline std::array<LunaWriteDescriptorSet, sizeof...(T)> generateWrites(const LunaDescriptorSet descriptorSet,
                                                                              const T &...writes)
{
    std::list<LunaDescriptorBufferInfo> list;
    return generateWrites<T...>(descriptorSet, list, writes...);
}

LightBakerGpu::LightBakerGpu()
{
    static constexpr LunaInstanceCreationInfo instanceCreationInfo = {
        .apiVersion = VK_API_VERSION_1_2,
#ifndef NDEBUG
        .enableValidation = true,
#endif
    };
    if (!checkResult(lunaCreateInstance(&instanceCreationInfo)))
    {
        return;
    }

    static constexpr LunaPhysicalDevicePreferenceDefinition physicalDevicePreferenceDefinition = {
        .preferredDeviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
    };
    static constexpr VkPhysicalDeviceFeatures required10Features = {
        .shaderFloat64 = VK_TRUE,
    };
    static VkPhysicalDeviceVulkan12Features required12Features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .uniformAndStorageBuffer8BitAccess = VK_TRUE,
        .shaderInt8 = VK_TRUE,
    };
    static constexpr VkPhysicalDeviceFeatures2 requiredFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &required12Features,
        .features = required10Features,
    };
    static constexpr LunaDeviceCreationInfo2 deviceCreationInfo = {
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
    for (LunaBuffer &luxelIndicesBuffer: luxelIndicesBuffers)
    {
        size_t bufferSize = sizeof(int) * lightmapSize.x * lightmapSize.y;
        if (bufferSize > properties.limits.maxStorageBufferRange)
        {
            bufferSize--;
            if (bufferSize > properties.limits.maxStorageBufferRange)
            {
                Logger::Error("Unable to allocate shader storage buffer of size {}", bufferSize);
                return false;
            }
        }
        const LunaBufferCreationInfo luxelIndicesBufferCreationInfo = {
            .size = bufferSize,
            .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        };
        lunaCreateBuffer(&luxelIndicesBufferCreationInfo, &luxelIndicesBuffer);
    }

    const LunaBufferCreationInfo hitIndicesBufferCreationInfo = {
        .size = sizeof(uint32_t) + lightmapSize.x * lightmapSize.y * sizeof(uint32_t),
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
    //
    // lunaDeviceWaitIdle();
    // if (!bakeBounceLighting(lightmapSize, indices.size(), 1))
    // {
    //     return false;
    // }

    lunaDeviceWaitIdle();
    LunaBuffer outputLightmap2d{};
    if (!convertLightmapToFloat16(lightmapSize, outputLightmap2d))
    {
        return false;
    }

    Logger::Info("Saving Lightmap...");
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
    Logger::Info("Baking direct lighting...");
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
    static constexpr std::array descriptorSetLayoutBindings = generateDescriptorSetLayoutBindings(std::pair{
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        std::array{"vertices", "indices", "lights", "luxel indices", "hit indices", "light hit indices", "lightmap2d"},
    });
    static constexpr LunaDescriptorSetLayoutCreationInfo descriptorSetLayoutCreationInfo = {
        .bindingCount = descriptorSetLayoutBindings.size(),
        .bindings = descriptorSetLayoutBindings.data(),
    };
    if (!createPipeline("assets/shaders/lightmap_direct_lighting_pass.comp",
                        specializationInfo,
                        {},
                        descriptorSetLayoutCreationInfo,
                        generateDescriptorPoolCreationInfo<&descriptorSetLayoutBindings>(),
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

    LunaBuffer lightHitIndicesBuffer{};
    const LunaBufferCreationInfo lightHitIndicesBufferCreationInfo = {
        .size = static_cast<VkDeviceSize>(lightmapSize.x * lightmapSize.y) * lights.size() / sizeof(uint32_t),
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    };
    lunaCreateBuffer(&lightHitIndicesBufferCreationInfo, &lightHitIndicesBuffer);

    std::list<LunaDescriptorBufferInfo> bufferInfos;
    const std::array writes = generateWrites(descriptorSet,
                                             bufferInfos,
                                             std::pair{"vertices", vertexBuffer},
                                             std::pair{"indices", indexBuffer},
                                             std::pair{"lights", lightsBuffer},
                                             std::pair{"luxel indices", luxelIndicesBuffers.at(0)},
                                             std::pair{"hit indices", hitIndicesBuffer},
                                             std::pair{"light hit indices", lightHitIndicesBuffer},
                                             std::pair{"lightmap2d", lightmap2d});
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
    static constexpr uint32_t MAX_WORK_GROUPS_Z = 256;

    LunaDescriptorSet descriptorSet{};
    LunaComputePipeline pipeline{};
    const std::array<uint32_t, 5> specializationData = {
        static_cast<uint32_t>(lightmapSize.x),
        static_cast<uint32_t>(lightmapSize.y),
        static_cast<uint32_t>(indexCount),
        WORK_GROUP_SIZE.x,
        WORK_GROUP_SIZE.y,
    };
    constexpr std::array<VkSpecializationMapEntry, 5>
            mapEntries = generateSpecializationMapEntries<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>();
    const VkSpecializationInfo specializationInfo = {
        .mapEntryCount = mapEntries.size(),
        .pMapEntries = mapEntries.data(),
        .dataSize = sizeof(specializationData),
        .pData = specializationData.data(),
    };
    static constexpr std::array descriptorSetLayoutBindings = generateDescriptorSetLayoutBindings(std::pair{
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        std::array{"vertices", "indices", "luxel indices", "hit indices", "lightmap2d"},
    });
    static constexpr LunaDescriptorSetLayoutCreationInfo descriptorSetLayoutCreationInfo = {
        .bindingCount = descriptorSetLayoutBindings.size(),
        .bindings = descriptorSetLayoutBindings.data(),
    };
    if (!createPipeline("assets/shaders/lightmap_bounce_lighting_pass.comp",
                        specializationInfo,
                        {},
                        descriptorSetLayoutCreationInfo,
                        generateDescriptorPoolCreationInfo<&descriptorSetLayoutBindings>(),
                        descriptorSet,
                        pipeline))
    {
        return false;
    }

    std::list<LunaDescriptorBufferInfo> bufferInfos;
    const std::array writes = generateWrites(descriptorSet,
                                             bufferInfos,
                                             std::pair{"vertices", vertexBuffer},
                                             std::pair{"indices", indexBuffer},
                                             std::pair{"hit indices", hitIndicesBuffer},
                                             std::pair{"lightmap2d", lightmap2d});
    lunaWriteDescriptorSets(writes.size(), writes.data());

    std::array<LunaDescriptorBufferInfo, 2> luxelIndicesBufferInfos = {
        LunaDescriptorBufferInfo{},
        LunaDescriptorBufferInfo{},
    };
    const std::array<LunaWriteDescriptorSet, 2> luxelIndiciesWrites = {
        LunaWriteDescriptorSet{
            .descriptorSet = descriptorSet,
            .bindingName = "luxel indices",
            .descriptorArrayElement = 0,
            .bufferInfo = &luxelIndicesBufferInfos.at(0),
        },
        LunaWriteDescriptorSet{
            .descriptorSet = descriptorSet,
            .bindingName = "luxel indices",
            .descriptorArrayElement = 1,
            .bufferInfo = &luxelIndicesBufferInfos.at(1),
        },
    };
    for (uint32_t i = 0; i < bounceCount; i++)
    {
        luxelIndicesBufferInfos.at(0).buffer = luxelIndicesBuffers.at(i % 2);
        luxelIndicesBufferInfos.at(1).buffer = luxelIndicesBuffers.at(i % 2);

        const uint32_t luxelIndexCount = *static_cast<uint32_t *>(lunaGetBufferDataPointer(hitIndicesBuffer));
        for (uint32_t j = 0; j < luxelIndexCount / MAX_WORK_GROUPS_Z; j++)
        {
            lunaWriteDescriptorSets(luxelIndiciesWrites.size(), luxelIndiciesWrites.data());
            const LunaDescriptorSetBindInfo descriptorSetBindInfo = {
                .descriptorSetCount = 1,
                .descriptorSets = &descriptorSet,
            };
            const LunaDispatchInfo dispatchInfo = {
                .pipeline = pipeline,
                .descriptorSetBindInfo = descriptorSetBindInfo,
                .groupCountX = NUM_WORK_GROUPS.x,
                .groupCountY = NUM_WORK_GROUPS.y,
                .groupCountZ = MAX_WORK_GROUPS_Z,
                .submitCommandBuffer = true,
            };
            if (!checkResult(lunaDispatch(&dispatchInfo)))
            {
                return false;
            }
            luxelIndicesBufferInfos.at(0).offset += sizeof(int) * MAX_WORK_GROUPS_Z;
            luxelIndicesBufferInfos.at(1).offset += sizeof(int) * MAX_WORK_GROUPS_Z;
        }
        if (luxelIndexCount % MAX_WORK_GROUPS_Z != 0)
        {
            lunaWriteDescriptorSets(luxelIndiciesWrites.size(), luxelIndiciesWrites.data());
            const LunaDescriptorSetBindInfo descriptorSetBindInfo = {
                .descriptorSetCount = 1,
                .descriptorSets = &descriptorSet,
            };
            const LunaDispatchInfo dispatchInfo = {
                .pipeline = pipeline,
                .descriptorSetBindInfo = descriptorSetBindInfo,
                .groupCountX = NUM_WORK_GROUPS.x,
                .groupCountY = NUM_WORK_GROUPS.y,
                .groupCountZ = MAX_WORK_GROUPS_Z,
                .submitCommandBuffer = true,
            };
            if (!checkResult(lunaDispatch(&dispatchInfo)))
            {
                return false;
            }
        }
        lunaDeviceWaitIdle();
    }
    return true;
}

bool LightBakerGpu::convertLightmapToFloat16(const glm::ivec2 &lightmapSize, LunaBuffer &outputLightmap2d) const
{
    Logger::Info("Finalizing Lightmap...");
    LunaDescriptorSet descriptorSet{};
    LunaComputePipeline pipeline{};
    const std::array<uint32_t, 3> specializationData = {
        static_cast<uint32_t>(lightmapSize.x),
        WORK_GROUP_SIZE.x,
        WORK_GROUP_SIZE.y,
    };
    constexpr std::array<VkSpecializationMapEntry, 3>
            mapEntries = generateSpecializationMapEntries<uint32_t, uint32_t, uint32_t>();
    const VkSpecializationInfo specializationInfo = {
        .mapEntryCount = mapEntries.size(),
        .pMapEntries = mapEntries.data(),
        .dataSize = sizeof(specializationData),
        .pData = specializationData.data(),
    };

    static constexpr std::array descriptorSetLayoutBindings = generateDescriptorSetLayoutBindings(
            std::pair{
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                std::array{"input lightmap"},
            },
            std::pair{
                VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
                std::array{"output lightmap"},
            });
    const LunaDescriptorSetLayoutCreationInfo descriptorSetLayoutCreationInfo = {
        .bindingCount = descriptorSetLayoutBindings.size(),
        .bindings = descriptorSetLayoutBindings.data(),
    };
    if (!createPipeline("assets/shaders/lightmap_convert_to_float16.comp",
                        specializationInfo,
                        {},
                        descriptorSetLayoutCreationInfo,
                        generateDescriptorPoolCreationInfo<&descriptorSetLayoutBindings>(),
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

    std::list<LunaDescriptorBufferInfo> bufferInfos;
    const std::array writes = generateWrites(descriptorSet,
                                             bufferInfos,
                                             std::pair{"input lightmap", lightmap2d},
                                             std::pair{"output lightmap", &outputLightmap2dBufferView});
    lunaWriteDescriptorSets(writes.size(), writes.data());

    const LunaDescriptorSetBindInfo descriptorSetBindInfo = {
        .descriptorSetCount = 1,
        .descriptorSets = &descriptorSet,
    };
    assert(lightmapSize.x % WORK_GROUP_SIZE.x == 0 && lightmapSize.y % WORK_GROUP_SIZE.y == 0);
    const LunaDispatchInfo dispatchInfo = {
        .pipeline = pipeline,
        .descriptorSetBindInfo = descriptorSetBindInfo,
        .groupCountX = lightmapSize.x / WORK_GROUP_SIZE.x,
        .groupCountY = lightmapSize.y / WORK_GROUP_SIZE.y,
        .submitCommandBuffer = true,
    };
    return checkResult(lunaDispatch(&dispatchInfo));
}
