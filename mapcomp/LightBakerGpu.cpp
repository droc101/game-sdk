//
// Created by NBT22 on 3/20/26.
//

#include "LightBakerGpu.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glslang/Public/ShaderLang.h>
#include <libassets/type/MapVertex.h>
#include <libassets/util/Error.h>
#include <libassets/util/Logger.h>
#include <libassets/util/ShaderCompiler.h>
#include <list>
#include <luna/luna.h>
#include <luna/lunaBuffer.h>
#include <luna/lunaCommandBuffer.h>
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
#include <volk.h>
#include <vulkan/vulkan_core.h>
#include "LevelMeshBuilder.h"
#include "Light.h"

static constexpr uint64_t RAY_COUNT = (uint64_t{1} << 30);
static constexpr uint32_t BOUNCES = 1;

static_assert(RAY_COUNT - 1 == static_cast<uint64_t>(static_cast<double>(RAY_COUNT) - 1),
              "Ray count must be representable as a double in order to be preserved in the shader!");
static_assert(RAY_COUNT % (1 << 15) == 0, "Ray count must be a multiple of (1 << 15)!");

/// This function is evil
template<typename... T>
static consteval std::array<VkSpecializationMapEntry, sizeof...(T)> GenerateSpecializationMapEntries()
{
    size_t index = 0;
    std::array<VkSpecializationMapEntry, sizeof...(T)> entries{};

    auto addEntry = [&index, &entries]<typename Type>() {
        entries.at(index) = VkSpecializationMapEntry{
            .constantID = static_cast<uint32_t>(index),
            .offset = index == 0 ? 0 : static_cast<uint32_t>(entries.at(index - 1).offset + entries.at(index - 1).size),
            .size = sizeof(Type),
        };
        index++;
    };
    (addEntry.template operator()<T>(), ...);
    return entries;
}

/// This function is about 10x more evil than generateSpecializationMapEntries
template<typename... T> requires(((std::__is_pair<T> &&
                                   std::is_same_v<typename T::first_type, VkDescriptorType> &&
                                   std::is_same_v<typename T::second_type::value_type, const char *>) &&
                                  ...))
static consteval auto GenerateDescriptorSetLayoutBindings(const T &...bindings)
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
    static constexpr std::array<VkDescriptorPoolSize, UNIQUE_BINDINGS.size() + 1> retArray = ([] {
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
                    poolSize.descriptorCount += (binding.descriptorCount == 0 ? 1 : binding.descriptorCount);
                    break;
                }
            }
        }
        return ret;
    })();
    static constexpr VkDescriptorPoolCreateFlags CREATION_FLAGS = ([] -> VkDescriptorPoolCreateFlags {
        VkDescriptorPoolCreateFlags ret{};
        for (const LunaDescriptorSetLayoutBinding &binding: *BINDINGS)
        {
            if ((binding.bindingFlags & VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT) ==
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT)
            {
                ret |= VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
            }
        }
        return ret;
    })();
    return LunaDescriptorPoolCreationInfo{
        .flags = CREATION_FLAGS,
        .maxSets = 1,
        .poolSizeCount = retArray.size(),
        .poolSizes = retArray.data(),
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
    static VkPhysicalDeviceRayTracingPipelineFeaturesKHR requiredRayTracingPipelineFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
        .rayTracingPipeline = VK_TRUE,
    };
    static VkPhysicalDeviceAccelerationStructureFeaturesKHR requiredAccelerationStructureFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
        .pNext = &requiredRayTracingPipelineFeatures,
        .accelerationStructure = VK_TRUE,
    };
    static VkPhysicalDeviceVulkan12Features required12Features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext = &requiredAccelerationStructureFeatures,
        .uniformAndStorageBuffer8BitAccess = VK_TRUE,
        .shaderInt8 = VK_TRUE,
        .descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE,
        .bufferDeviceAddress = VK_TRUE,
    };
    static constexpr VkPhysicalDeviceFeatures2 requiredFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &required12Features,
        .features = required10Features,
    };
    static constexpr std::array requiredExtensions = {
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
    };
    static constexpr LunaDeviceCreationInfo2 deviceCreationInfo = {
        .extensionCount = requiredExtensions.size(),
        .extensionNames = requiredExtensions.data(),
        .requiredFeatures = requiredFeatures,
        .physicalDevicePreferenceDefinition = &physicalDevicePreferenceDefinition,
        .allocatorCreateFlags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT |
                                VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
    };
    if (!checkResult(lunaCreateDevice2(&deviceCreationInfo)))
    {
        return;
    }

    vkGetPhysicalDeviceProperties2(lunaGetPhysicalDevice(), &physicalDeviceProperties);

    const LunaCommandBufferAllocationInfo allocationInfo = {
        .commandPool = LUNA_INTERNAL_COMPUTE_COMMAND_POOL,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    };
    if (!checkResult(lunaAllocateCommandBuffer(&allocationInfo, &commandBuffer)))
    {
        return;
    }

    initialized = true;
}

LightBakerGpu::~LightBakerGpu()
{
    lunaDestroyInstance();
}

static constexpr VmaAllocationCreateInfo VRAM_ALLOCATION_CREATE_INFO = {
    .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
};
static constexpr VmaAllocationCreateInfo MAPPED_ALLOCATION_CREATE_INFO = {
    .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
    .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
};

bool LightBakerGpu::bake(const std::unordered_map<std::string, LevelMeshBuilder> &meshBuilders,
                         const std::vector<Light> &lights,
                         const glm::uvec2 &lightmapSize,
                         std::vector<uint8_t> &pixelData)
{
    static constexpr uint64_t WIDTH = std::min(RAY_COUNT, uint64_t{1} << 15);
    static constexpr uint64_t HEIGHT = std::min(std::max(RAY_COUNT / WIDTH, uint64_t{1}), uint64_t{1} << 15);
    static constexpr uint64_t ITERATIONS = std::max(RAY_COUNT / WIDTH / HEIGHT, uint64_t{1});

    if (meshBuilders.empty())
    {
        return false;
    }
    if (lights.size() > 16384)
    {
        Logger::Error("Maximum number of lights allowed in a level is 16384!");
        return false;
    }

    pixelData.clear();

    const LunaBufferCreationInfo lightsBufferCreationInfo = {
        .size = sizeof(Light) * lights.size(),
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .allocationCreateInfo = &VRAM_ALLOCATION_CREATE_INFO,
    };
    if (!checkResult(lunaCreateBuffer(&lightsBufferCreationInfo, &lightsBuffer)))
    {
        return false;
    }

    const LunaBufferWriteInfo lightsBufferWriteInfo = {
        .bytes = sizeof(Light) * lights.size(),
        .data = lights.data(),
        .stageFlags = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
    };
    if (!checkResult(lunaWriteDataToBuffer(lightsBuffer, &lightsBufferWriteInfo)))
    {
        return false;
    }

    assert((lightmapSize.x * lightmapSize.y * lights.size()) % sizeof(uint32_t) == 0);
    const LunaBufferCreationInfo lightHitIndicesBufferCreationInfo = {
        .size = (lightmapSize.x * lightmapSize.y * lights.size()) / sizeof(uint32_t),
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .allocationCreateInfo = &VRAM_ALLOCATION_CREATE_INFO,
    };
    if (!checkResult(lunaCreateBuffer(&lightHitIndicesBufferCreationInfo, &lightHitIndicesBuffer)))
    {
        return false;
    }
    if (!checkResult(lunaFillBuffer(lightHitIndicesBuffer, 0)))
    {
        return false;
    }

    const LunaBufferCreationInfo lightmapCreationInfo = {
        .size = lightmapSize.x * lightmapSize.y * sizeof(float) * 3,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .allocationCreateInfo = &VRAM_ALLOCATION_CREATE_INFO,
    };
    if (!checkResult(lunaCreateBuffer(&lightmapCreationInfo, &lightmap)))
    {
        return false;
    }
    if (!checkResult(lunaFillBuffer(lightmap, 0)))
    {
        return false;
    }

    if (!createBLAS(meshBuilders))
    {
        return false;
    }
    if (!createTLAS())
    {
        return false;
    }
    if (!createAndWriteDescriptorSet())
    {
        return false;
    }
    if (!createPipeline(lightmapSize, lights.size()))
    {
        return false;
    }

    lunaDeviceWaitIdle();

    for (uint32_t i = 0; i < lights.size() * ITERATIONS; i++)
    {
        Logger::Info("Baking lighting {}%...",
                     static_cast<float>(100 * i) / static_cast<float>(lights.size() * ITERATIONS));

        if (!checkResult(lunaBeginSingleUseCommandBuffer(commandBuffer)))
        {
            return false;
        }

        const VkCommandBuffer vkCommandBuffer = lunaGetVkCommandBuffer(commandBuffer);

        vkCmdBindDescriptorSets(vkCommandBuffer,
                                VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                                pipelineLayout,
                                0,
                                1,
                                &descriptorSet,
                                0,
                                nullptr);
        vkCmdPushConstants(vkCommandBuffer,
                           pipelineLayout,
                           VK_SHADER_STAGE_RAYGEN_BIT_KHR,
                           0,
                           sizeof(lightIndex),
                           &lightIndex);
        vkCmdPushConstants(vkCommandBuffer,
                           pipelineLayout,
                           VK_SHADER_STAGE_RAYGEN_BIT_KHR,
                           sizeof(lightIndex),
                           sizeof(iteration),
                           &iteration);
        vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);

        static constexpr VkStridedDeviceAddressRegionKHR STRIDED_DEVICE_ADDRESS_REGION_NONE{};
        const VkStridedDeviceAddressRegionKHR raygenShaderBindingTableAddressRegion = {
            .deviceAddress = lunaGetBufferDeviceAddress(raygenShaderBindingTable),
            .stride = physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
            .size = physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
        };
        const VkStridedDeviceAddressRegionKHR closestHitShaderBindingTableAddressRegion = {
            .deviceAddress = lunaGetBufferDeviceAddress(closestHitShaderBindingTable),
            .stride = physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
            .size = physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
        };
        vkCmdTraceRaysKHR(vkCommandBuffer,
                          &raygenShaderBindingTableAddressRegion,
                          &STRIDED_DEVICE_ADDRESS_REGION_NONE,
                          &closestHitShaderBindingTableAddressRegion,
                          &STRIDED_DEVICE_ADDRESS_REGION_NONE,
                          WIDTH,
                          HEIGHT,
                          1);

        if (!checkResult(lunaEndCommandBuffer(commandBuffer)))
        {
            return false;
        }
        if (!checkResult(lunaSubmitInternalComputeQueue(commandBuffer, true)))
        {
            return false;
        }
        iteration = (iteration + 1) % ITERATIONS;
        if (iteration == 0)
        {
            lightIndex++;
        }
    }

    lunaDeviceWaitIdle();
    LunaBuffer outputLightmap{};
    if (!convertLightmapToFloat16(lightmapSize, outputLightmap))
    {
        return false;
    }

    Logger::Info("Saving Lightmap...");
    lunaDeviceWaitIdle();
    uint8_t *bufferData = static_cast<uint8_t *>(lunaGetBufferDataPointer(outputLightmap));
    pixelData.resize(lunaGetBufferSize(outputLightmap));
    std::copy_n(bufferData, pixelData.size(), pixelData.data());
    return true;
}

VkShaderModule LightBakerGpu::generateShaderModule(const std::filesystem::path &path,
                                                   const EShLanguage shaderType,
                                                   std::vector<uint32_t> &spirv)
{
    assert(spirv.empty());

    ShaderCompiler shaderCompiler(path, shaderType);
    shaderCompiler.SetTargetVersions(glslang::EShTargetClientVersion::EShTargetVulkan_1_2,
                                     glslang::EShTargetLanguageVersion::EShTargetSpv_1_4);
    if (shaderCompiler.Compile(spirv) != Error::ErrorCode::OK)
    {
        Logger::Error("Error compiling shader {}!", path.c_str());
        return VK_NULL_HANDLE;
    }

    VkShaderModule shaderModule{};
    const VkShaderModuleCreateInfo shaderModuleCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spirv.size() * sizeof(uint32_t),
        .pCode = spirv.data(),
    };
    if (!checkResult(vkCreateShaderModule(lunaGetDevice(), &shaderModuleCreateInfo, nullptr, &shaderModule)))
    {
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}

bool LightBakerGpu::createBLAS(const std::unordered_map<std::string, LevelMeshBuilder> &meshBuilders)
{
    // The position MUST be at the start of the struct for the acceleration structure to work properly
    static_assert(offsetof(MapVertex, position) == 0, "MapVertex::position must be at offset zero!");
    // The position of the vertex must be three single precision floating point numbers,
    //  since we use VK_FORMAT_R32G32B32_SFLOAT as the vertex format.
    //  If the type used for position changes, update the vertex format accordingly.
    static_assert(std::same_as<decltype(MapVertex::position), glm::vec3>,
                  "MapVertex::position must be a three-component vector of single precision floats!");

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

    const LunaBufferCreationInfo vertexBufferCreationInfo = {
        .size = vertexBufferByteCount,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                 VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .allocationCreateInfo = &VRAM_ALLOCATION_CREATE_INFO,
    };
    if (!checkResult(lunaCreateBuffer(&vertexBufferCreationInfo, &vertexBuffer)))
    {
        return false;
    }
    const LunaBufferCreationInfo indexBufferCreationInfo = {
        .size = indexBufferByteCount,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                 VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .allocationCreateInfo = &VRAM_ALLOCATION_CREATE_INFO,
    };
    if (!checkResult(lunaCreateBuffer(&indexBufferCreationInfo, &indexBuffer)))
    {
        return false;
    }

    const LunaBufferWriteInfo vertexBufferWriteInfo = {
        .bytes = vertexBufferByteCount,
        .data = vertices.data(),
        .stageFlags = VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
    };
    if (!checkResult(lunaWriteDataToBuffer(vertexBuffer, &vertexBufferWriteInfo)))
    {
        return false;
    }
    const LunaBufferWriteInfo indexBufferWriteInfo = {
        .bytes = indexBufferByteCount,
        .data = indices.data(),
        .stageFlags = VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
    };
    if (!checkResult(lunaWriteDataToBuffer(indexBuffer, &indexBufferWriteInfo)))
    {
        return false;
    }

    if (!checkResult(lunaDeviceWaitIdle()))
    {
        return false;
    }

    const VkDevice device = lunaGetDevice();

    if (!checkResult(lunaBeginSingleUseCommandBuffer(commandBuffer)))
    {
        return false;
    }

    const VkDeviceOrHostAddressConstKHR vertexDataAddress = {
        .deviceAddress = lunaGetBufferDeviceAddress(vertexBuffer),
    };
    const VkDeviceOrHostAddressConstKHR indexDataAddress = {
        .deviceAddress = lunaGetBufferDeviceAddress(indexBuffer),
    };
    const VkAccelerationStructureGeometryTrianglesDataKHR geometryTrianglesData = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
        .vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
        .vertexData = vertexDataAddress,
        .vertexStride = sizeof(MapVertex),
        .maxVertex = static_cast<uint32_t>(vertices.size() - 1),
        .indexType = VK_INDEX_TYPE_UINT32,
        .indexData = indexDataAddress,
    };
    const VkAccelerationStructureGeometryDataKHR geometryData = {
        .triangles = geometryTrianglesData,
    };
    const VkAccelerationStructureGeometryKHR geometry = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
        .geometry = geometryData,
        .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
    };

    const VkAccelerationStructureBuildGeometryInfoKHR buildSizesGeometryInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        .geometryCount = 1,
        .pGeometries = &geometry,
    };
    VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
    };
    const uint32_t triangleCount = indices.size() / 3;
    vkGetAccelerationStructureBuildSizesKHR(device,
                                            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                            &buildSizesGeometryInfo,
                                            &triangleCount,
                                            &buildSizesInfo);

    const LunaBufferCreationInfo bufferCreationInfo = {
        .size = buildSizesInfo.accelerationStructureSize,
        .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .alignment = 256, // 256 is directly required by the spec
        .allocationCreateInfo = &VRAM_ALLOCATION_CREATE_INFO,
    };
    if (!checkResult(lunaCreateBuffer(&bufferCreationInfo, &blas.buffer)))
    {
        return false;
    }

    const VkAccelerationStructureCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .buffer = lunaGetVkBuffer(blas.buffer),
        .offset = lunaGetBufferOffset(blas.buffer),
        .size = lunaGetBufferSize(blas.buffer),
        .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
    };
    if (!checkResult(vkCreateAccelerationStructureKHR(device, &createInfo, nullptr, &blas.accelerationStructure)))
    {
        return false;
    }

    const LunaBufferCreationInfo scratchBufferCreationInfo = {
        .size = buildSizesInfo.buildScratchSize,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .alignment = physicalDeviceAccelerationStructureProperties.minAccelerationStructureScratchOffsetAlignment,
        .allocationCreateInfo = &VRAM_ALLOCATION_CREATE_INFO,
    };
    if (!checkResult(lunaCreateBuffer(&scratchBufferCreationInfo, &blas.scratchBuffer)))
    {
        return false;
    }

    const VkDeviceOrHostAddressKHR scratchDataAddress = {
        .deviceAddress = lunaGetBufferDeviceAddress(blas.scratchBuffer),
    };
    const VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        .dstAccelerationStructure = blas.accelerationStructure,
        .geometryCount = 1,
        .pGeometries = &geometry,
        .scratchData = scratchDataAddress,
    };
    const VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo = {
        .primitiveCount = triangleCount,
    };
    const VkAccelerationStructureBuildRangeInfoKHR *pBuildRangeInfo = &buildRangeInfo;
    vkCmdBuildAccelerationStructuresKHR(lunaGetVkCommandBuffer(commandBuffer), 1, &buildGeometryInfo, &pBuildRangeInfo);

    if (!checkResult(lunaEndCommandBuffer(commandBuffer)))
    {
        return false;
    }
    return checkResult(lunaSubmitInternalComputeQueue(commandBuffer, true));
}

bool LightBakerGpu::createTLAS()
{
    static constexpr VkTransformMatrixKHR TRANSFORM_MATRIX_IDENTITY = {{
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
    }};

    const VkDevice device = lunaGetDevice();

    if (!checkResult(lunaBeginSingleUseCommandBuffer(commandBuffer)))
    {
        return false;
    }

    const LunaBufferCreationInfo instancesBufferCreationInfo = {
        .size = sizeof(VkAccelerationStructureInstanceKHR),
        .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .alignment = 16, // 16 is directly required by the spec
        .allocationCreateInfo = &VRAM_ALLOCATION_CREATE_INFO,
    };
    if (!checkResult(lunaCreateBuffer(&instancesBufferCreationInfo, &accelerationStructureInstancesBuffer)))
    {
        return false;
    }

    const VkAccelerationStructureDeviceAddressInfoKHR deviceAddressInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
        .accelerationStructure = blas.accelerationStructure,
    };
    const VkAccelerationStructureInstanceKHR instance = {
        .transform = TRANSFORM_MATRIX_IDENTITY,
        .mask = 0xFF,
        .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
        .accelerationStructureReference = vkGetAccelerationStructureDeviceAddressKHR(device, &deviceAddressInfo),
    };
    const LunaBufferWriteInfo instancesBufferWriteInfo = {
        .bytes = sizeof(instance),
        .data = &instance,
        .stageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    };
    if (!checkResult(lunaWriteDataToBuffer(accelerationStructureInstancesBuffer, &instancesBufferWriteInfo)))
    {
        return false;
    }

    const VkDeviceOrHostAddressConstKHR instancesDataAddress = {
        .deviceAddress = lunaGetBufferDeviceAddress(accelerationStructureInstancesBuffer),
    };
    const VkAccelerationStructureGeometryInstancesDataKHR geometryInstancesData = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
        .arrayOfPointers = VK_FALSE,
        .data = instancesDataAddress,
    };
    const VkAccelerationStructureGeometryDataKHR geometryData = {
        .instances = geometryInstancesData,
    };
    const VkAccelerationStructureGeometryKHR geometry = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
        .geometry = geometryData,
        .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
    };

    const VkAccelerationStructureBuildGeometryInfoKHR buildSizesGeometryInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        .geometryCount = 1,
        .pGeometries = &geometry,
    };
    VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
    };
    static constexpr uint32_t MAX_PRIMITIVE_COUNT = 1;
    vkGetAccelerationStructureBuildSizesKHR(device,
                                            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                            &buildSizesGeometryInfo,
                                            &MAX_PRIMITIVE_COUNT,
                                            &buildSizesInfo);

    const LunaBufferCreationInfo bufferCreationInfo = {
        .size = buildSizesInfo.accelerationStructureSize,
        .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .alignment = 256, // 256 is directly required by the spec
        .allocationCreateInfo = &VRAM_ALLOCATION_CREATE_INFO,
    };
    if (!checkResult(lunaCreateBuffer(&bufferCreationInfo, &tlas.buffer)))
    {
        return false;
    }

    const VkAccelerationStructureCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .buffer = lunaGetVkBuffer(tlas.buffer),
        .offset = lunaGetBufferOffset(tlas.buffer),
        .size = lunaGetBufferSize(tlas.buffer),
        .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
    };
    if (!checkResult(vkCreateAccelerationStructureKHR(device, &createInfo, nullptr, &tlas.accelerationStructure)))
    {
        return false;
    }

    const LunaBufferCreationInfo scratchBufferCreationInfo = {
        .size = buildSizesInfo.buildScratchSize,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .alignment = physicalDeviceAccelerationStructureProperties.minAccelerationStructureScratchOffsetAlignment,
        .allocationCreateInfo = &VRAM_ALLOCATION_CREATE_INFO,
    };
    if (!checkResult(lunaCreateBuffer(&scratchBufferCreationInfo, &tlas.scratchBuffer)))
    {
        return false;
    }

    const VkDeviceOrHostAddressKHR scratchDataAddress = {
        .deviceAddress = lunaGetBufferDeviceAddress(tlas.scratchBuffer),
    };
    const VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        .dstAccelerationStructure = tlas.accelerationStructure,
        .geometryCount = 1,
        .pGeometries = &geometry,
        .scratchData = scratchDataAddress,
    };
    const VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo = {
        .primitiveCount = MAX_PRIMITIVE_COUNT,
    };
    const VkAccelerationStructureBuildRangeInfoKHR *pBuildRangeInfo = &buildRangeInfo;
    vkCmdBuildAccelerationStructuresKHR(lunaGetVkCommandBuffer(commandBuffer), 1, &buildGeometryInfo, &pBuildRangeInfo);

    if (!checkResult(lunaEndCommandBuffer(commandBuffer)))
    {
        return false;
    }
    return checkResult(lunaSubmitInternalComputeQueue(commandBuffer, true));
}

bool LightBakerGpu::createPipeline(const glm::uvec2 &lightmapSize, const uint32_t lightCount)
{
    const VkDevice device = lunaGetDevice();

    std::vector<uint32_t> raygenShaderSpirv;
    const VkShaderModule raygenShaderModule = generateShaderModule("assets/shaders/lightmap/raygen.rgen",
                                                                   EShLangRayGen,
                                                                   raygenShaderSpirv);
    if (raygenShaderModule == VK_NULL_HANDLE)
    {
        return false;
    }
    std::vector<uint32_t> closestHitShaderSpirv;
    const VkShaderModule closestHitShaderModule = generateShaderModule("assets/shaders/lightmap/closesthit.rchit",
                                                                       EShLangClosestHit,
                                                                       closestHitShaderSpirv);
    if (closestHitShaderModule == VK_NULL_HANDLE)
    {
        return false;
    }

    static constexpr struct
    {
            double rayCount;
            uint32_t iterationCount;
            uint32_t bounces;
    } RAYGEN_SPECIALIZATION_DATA = {
        .rayCount = static_cast<double>(RAY_COUNT),
        .iterationCount = std::max(RAY_COUNT / (uint64_t{1} << 30), uint64_t{1}),
        .bounces = BOUNCES,
    };
    static constexpr std::array RAYGEN_MAP_ENTRIES = GenerateSpecializationMapEntries<double, uint32_t, uint32_t>();
    const VkSpecializationInfo raygenSpecializationInfo = {
        .mapEntryCount = RAYGEN_MAP_ENTRIES.size(),
        .pMapEntries = RAYGEN_MAP_ENTRIES.data(),
        .dataSize = sizeof(RAYGEN_SPECIALIZATION_DATA),
        .pData = &RAYGEN_SPECIALIZATION_DATA,
    };
    const std::array closestHitSpecializationData = {
        lightmapSize.x,
        lightmapSize.y,
        lightCount,
    };
    static constexpr std::array
            CLOSEST_HIT_MAP_ENTRIES = GenerateSpecializationMapEntries<uint32_t, uint32_t, uint32_t>();
    const VkSpecializationInfo closestHitSpecializationInfo = {
        .mapEntryCount = CLOSEST_HIT_MAP_ENTRIES.size(),
        .pMapEntries = CLOSEST_HIT_MAP_ENTRIES.data(),
        .dataSize = sizeof(closestHitSpecializationData),
        .pData = closestHitSpecializationData.data(),
    };
    const std::array shaderStages = {
        VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
            .module = raygenShaderModule,
            .pName = "main",
            .pSpecializationInfo = &raygenSpecializationInfo,
        },
        VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
            .module = closestHitShaderModule,
            .pName = "main",
            .pSpecializationInfo = &closestHitSpecializationInfo,
        },
    };

    const std::array shaderGroups = {
        VkRayTracingShaderGroupCreateInfoKHR{
            .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
            .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
            .generalShader = 0,
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR,
        },
        VkRayTracingShaderGroupCreateInfoKHR{
            .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
            .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
            .generalShader = VK_SHADER_UNUSED_KHR,
            .closestHitShader = 1,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR,
        },
    };

    const std::array pushConstantRanges = {
        VkPushConstantRange{
            .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
            .size = sizeof(lightIndex) + sizeof(iteration),
        },
    };
    const VkPipelineLayoutCreateInfo layoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptorSetLayout,
        .pushConstantRangeCount = pushConstantRanges.size(),
        .pPushConstantRanges = pushConstantRanges.data(),
    };
    if (!checkResult(vkCreatePipelineLayout(device, &layoutCreateInfo, nullptr, &pipelineLayout)))
    {
        return false;
    }

    const VkRayTracingPipelineCreateInfoKHR pipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
        .stageCount = shaderStages.size(),
        .pStages = shaderStages.data(),
        .groupCount = shaderGroups.size(),
        .pGroups = shaderGroups.data(),
        .maxPipelineRayRecursionDepth = 1,
        .layout = pipelineLayout,
    };
    if (!checkResult(vkCreateRayTracingPipelinesKHR(device,
                                                    VK_NULL_HANDLE,
                                                    VK_NULL_HANDLE,
                                                    1,
                                                    &pipelineCreateInfo,
                                                    nullptr,
                                                    &pipeline)))
    {
        return false;
    }

    vkDestroyShaderModule(device, raygenShaderModule, nullptr);
    vkDestroyShaderModule(device, closestHitShaderModule, nullptr);

    return createShaderBindingTables();
}

bool LightBakerGpu::createAndWriteDescriptorSet()
{
    const VkDevice device = lunaGetDevice();

    const std::array descriptorSetLayoutBindings = {
        VkDescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 3,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 4,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 5,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
        },
    };

    const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = descriptorSetLayoutBindings.size(),
        .pBindings = descriptorSetLayoutBindings.data(),
    };
    if (!checkResult(vkCreateDescriptorSetLayout(device,
                                                 &descriptorSetLayoutCreateInfo,
                                                 nullptr,
                                                 &descriptorSetLayout)))
    {
        return false;
    }

    const std::array descriptorPoolSizes = {
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
            .descriptorCount = 1,
        },
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 5,
        },
    };
    const VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1,
        .poolSizeCount = descriptorPoolSizes.size(),
        .pPoolSizes = descriptorPoolSizes.data(),
    };
    if (!checkResult(vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &descriptorPool)))
    {
        return false;
    }

    const VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &descriptorSetLayout,
    };
    if (!checkResult(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorSet)))
    {
        return false;
    }

    const VkWriteDescriptorSetAccelerationStructureKHR accelerationStructureWriteInfo = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
        .accelerationStructureCount = 1,
        .pAccelerationStructures = &tlas.accelerationStructure,
    };
    const VkDescriptorBufferInfo lightsBufferInfo{
        .buffer = lunaGetVkBuffer(lightsBuffer),
        .offset = lunaGetBufferOffset(lightsBuffer),
        .range = lunaGetBufferSize(lightsBuffer),
    };
    const VkDescriptorBufferInfo vertexBufferInfo{
        .buffer = lunaGetVkBuffer(vertexBuffer),
        .offset = lunaGetBufferOffset(vertexBuffer),
        .range = lunaGetBufferSize(vertexBuffer),
    };
    const VkDescriptorBufferInfo indexBufferInfo{
        .buffer = lunaGetVkBuffer(indexBuffer),
        .offset = lunaGetBufferOffset(indexBuffer),
        .range = lunaGetBufferSize(indexBuffer),
    };
    const VkDescriptorBufferInfo lightHitIndicesBufferInfo{
        .buffer = lunaGetVkBuffer(lightHitIndicesBuffer),
        .offset = lunaGetBufferOffset(lightHitIndicesBuffer),
        .range = lunaGetBufferSize(lightHitIndicesBuffer),
    };
    const VkDescriptorBufferInfo lightmapInfo{
        .buffer = lunaGetVkBuffer(lightmap),
        .offset = lunaGetBufferOffset(lightmap),
        .range = lunaGetBufferSize(lightmap),
    };
    const std::array descriptorSetWrites = {
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = &accelerationStructureWriteInfo,
            .dstSet = descriptorSet,
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
        },
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = 1,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &lightsBufferInfo,
        },
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = 2,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &vertexBufferInfo,
        },
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = 3,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &indexBufferInfo,
        },
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = 4,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &lightHitIndicesBufferInfo,
        },
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = 5,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &lightmapInfo,
        },
    };
    vkUpdateDescriptorSets(device, descriptorSetWrites.size(), descriptorSetWrites.data(), 0, nullptr);

    return true;
}

bool LightBakerGpu::createShaderBindingTables()
{
    static constexpr VkBufferUsageFlags USAGE_FLAGS = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR |
                                                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                                                      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    const LunaBufferCreationInfo raygenShaderBindingTableCreationInfo = {
        .size = physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
        .usage = USAGE_FLAGS,
        .alignment = physicalDeviceRayTracingPipelineProperties.shaderGroupBaseAlignment,
        .allocationCreateInfo = &VRAM_ALLOCATION_CREATE_INFO,
    };
    if (!checkResult(lunaCreateBuffer(&raygenShaderBindingTableCreationInfo, &raygenShaderBindingTable)))
    {
        return false;
    }

    const LunaBufferCreationInfo closestHitShaderBindingTableCreationInfo = {
        .size = physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
        .usage = USAGE_FLAGS,
        .alignment = physicalDeviceRayTracingPipelineProperties.shaderGroupBaseAlignment,
        .allocationCreateInfo = &VRAM_ALLOCATION_CREATE_INFO,
    };
    if (!checkResult(lunaCreateBuffer(&closestHitShaderBindingTableCreationInfo, &closestHitShaderBindingTable)))
    {
        return false;
    }

    std::vector<char> shaderHandles(2 * physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize);
    if (!checkResult(vkGetRayTracingShaderGroupHandlesKHR(lunaGetDevice(),
                                                          pipeline,
                                                          0,
                                                          2,
                                                          shaderHandles.size(),
                                                          shaderHandles.data())))
    {
        return false;
    }

    const LunaBufferWriteInfo raygenShaderBindingTableWriteInfo = {
        .bytes = physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
        .data = shaderHandles.data(),
        .stageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    };
    if (!checkResult(lunaWriteDataToBuffer(raygenShaderBindingTable, &raygenShaderBindingTableWriteInfo)))
    {
        return false;
    }

    const LunaBufferWriteInfo closestHitShaderBindingTableWriteInfo = {
        .bytes = physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
        .data = shaderHandles.data() + physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
        .stageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    };
    return checkResult(lunaWriteDataToBuffer(closestHitShaderBindingTable, &closestHitShaderBindingTableWriteInfo));
}

bool LightBakerGpu::convertLightmapToFloat16(const glm::uvec2 &lightmapSize, LunaBuffer &outputLightmap) const
{
    static constexpr glm::uvec2 WORK_GROUP_SIZE{32};

    Logger::Info("Finalizing Lightmap...");


    std::ifstream glslFile("assets/shaders/lightmap/convert_to_float16.comp");
    std::stringstream glsl;
    glsl << glslFile.rdbuf();
    glslFile.close();
    const ShaderCompiler shaderCompiler(glsl.str(),
                                        EShLangCompute,
                                        glslang::EShTargetClientVersion::EShTargetVulkan_1_2);
    std::vector<uint32_t> spirv;
    if (shaderCompiler.Compile(spirv) != Error::ErrorCode::OK)
    {
        Logger::Error("Error compiling shader assets/shaders/lightmap/convert_to_float16.comp!");
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
    static constexpr std::array DESCRIPTOR_SET_LAYOUT_BINDINGS = GenerateDescriptorSetLayoutBindings(
            std::pair{
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                std::array{"input lightmap"},
            },
            std::pair{
                VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
                std::array{"output lightmap"},
            });
    const LunaDescriptorSetLayoutCreationInfo descriptorSetLayoutCreationInfo = {
        .bindingCount = DESCRIPTOR_SET_LAYOUT_BINDINGS.size(),
        .bindings = DESCRIPTOR_SET_LAYOUT_BINDINGS.data(),
    };
    if (!checkResult(lunaCreateDescriptorSetLayout(&descriptorSetLayoutCreationInfo, &descriptorSetLayout)))
    {
        return false;
    }

    const std::array specializationData = {
        static_cast<uint32_t>(lightmapSize.x),
        WORK_GROUP_SIZE.x,
        WORK_GROUP_SIZE.y,
    };
    static constexpr std::array MAP_ENTRIES = GenerateSpecializationMapEntries<uint32_t, uint32_t, uint32_t>();
    const VkSpecializationInfo specializationInfo = {
        .mapEntryCount = MAP_ENTRIES.size(),
        .pMapEntries = MAP_ENTRIES.data(),
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
        .flags = VK_PIPELINE_CREATE_DISPATCH_BASE,
        .shaderStageCreationInfo = shaderStageCreationInfo,
        .layoutCreationInfo = layoutCreationInfo,
    };
    LunaComputePipeline pipeline{};
    if (!checkResult(lunaCreateComputePipeline(&pipelineCreationInfo, &pipeline)))
    {
        return false;
    }

    LunaDescriptorPool descriptorPool{};
    static constexpr LunaDescriptorPoolCreationInfo
            DESCRIPTOR_POOL_CREATION_INFO = generateDescriptorPoolCreationInfo<&DESCRIPTOR_SET_LAYOUT_BINDINGS>();
    if (!checkResult(lunaCreateDescriptorPool(&DESCRIPTOR_POOL_CREATION_INFO, &descriptorPool)))
    {
        return false;
    }

    const LunaDescriptorSetAllocationInfo allocationInfo = {
        .descriptorPool = descriptorPool,
        .setLayoutCount = 1,
        .setLayouts = &descriptorSetLayout,
    };
    LunaDescriptorSet descriptorSet{};
    if (!checkResult(lunaAllocateDescriptorSets(&allocationInfo, &descriptorSet)))
    {
        return false;
    }

    const LunaBufferCreationInfo levelGeometryLightmapCreationInfo = {
        .size = lightmapSize.x * lightmapSize.y * sizeof(_Float16) * 4,
        .usage = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
        .allocationCreateInfo = &MAPPED_ALLOCATION_CREATE_INFO,
    };
    if (!checkResult(lunaCreateBuffer(&levelGeometryLightmapCreationInfo, &outputLightmap)))
    {
        return false;
    }

    LunaBufferView outputLightmapBufferView{};
    const LunaBufferViewCreationInfo outputLightmapBufferViewCreationInfo = {
        .buffer = outputLightmap,
        .format = VK_FORMAT_R16G16B16A16_SFLOAT,
    };
    if (!checkResult(lunaCreateBufferView(&outputLightmapBufferViewCreationInfo, &outputLightmapBufferView)))
    {
        return false;
    }

    std::list<LunaDescriptorBufferInfo> bufferInfos;
    const std::array writes = generateWrites(descriptorSet,
                                             bufferInfos,
                                             std::pair{"input lightmap", lightmap},
                                             std::pair{"output lightmap", &outputLightmapBufferView});
    lunaWriteDescriptorSets(writes.size(), writes.data());

    const LunaDescriptorSetBindInfo descriptorSetBindInfo = {
        .descriptorSetCount = 1,
        .descriptorSets = &descriptorSet,
    };
    assert(lightmapSize.x % WORK_GROUP_SIZE.x == 0 && lightmapSize.y % WORK_GROUP_SIZE.y == 0);
    const LunaDispatchInfo dispatchInfo = {
        .pipeline = pipeline,
        .descriptorSetBindInfo = &descriptorSetBindInfo,
        .groupCountX = lightmapSize.x / WORK_GROUP_SIZE.x,
        .groupCountY = lightmapSize.y / WORK_GROUP_SIZE.y,
        .submitQueue = true,
    };
    return checkResult(lunaDispatch(&dispatchInfo));
}
