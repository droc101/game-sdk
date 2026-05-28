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
#include <filesystem>
#include <fstream>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glslang/Public/ShaderLang.h>
#include <libassets/type/Actor.h>
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

namespace
{
namespace Concepts
{
    template<typename T> concept LunaDescriptorSetLayoutBindingPair = std::__is_pair<T> &&
                                                                      std::same_as<typename T::first_type,
                                                                                   VkDescriptorType> &&
                                                                      std::same_as<typename T::second_type::value_type,
                                                                                   const char *>;
    template<typename T> concept VkDescriptorSetLayoutBindingPair = std::__is_pair<T> &&
                                                                    std::same_as<typename T::first_type,
                                                                                 VkDescriptorType> &&
                                                                    (std::same_as<typename T::second_type,
                                                                                  VkShaderStageFlagBits> ||
                                                                     std::same_as<typename T::second_type,
                                                                                  VkShaderStageFlags> ||
                                                                     std::same_as<typename T::second_type, int>);
} // namespace Concepts

template<typename... T> class SpecializationMapEntries: public std::array<VkSpecializationMapEntry, sizeof...(T)>
{
    public:
        constexpr SpecializationMapEntries()
        {
            size_t index = 0;

            auto addEntry = [&index, this]<typename Type>() {
                this->at(index) = VkSpecializationMapEntry{
                    .constantID = static_cast<uint32_t>(index),
                    .offset = index == 0 ? 0
                                         : static_cast<uint32_t>(this->at(index - 1).offset + this->at(index - 1).size),
                    .size = sizeof(Type),
                };
                index++;
            };
            (addEntry.template operator()<T>(), ...);
        }
};

template<typename... T> class DescriptorSetLayoutBindings
{
    public:
        explicit constexpr DescriptorSetLayoutBindings(const T &...) = delete;
};
template<Concepts::LunaDescriptorSetLayoutBindingPair... T> class DescriptorSetLayoutBindings<T...>
    : public std::array<LunaDescriptorSetLayoutBinding,
                        ((sizeof(T::second_type::_M_elems) / sizeof(typename T::second_type::value_type)) + ...)>
{
    public:
        explicit constexpr DescriptorSetLayoutBindings(const T &...bindings)
        {
            size_t index = 0;

            auto AddEntry = [&index, this]<typename Type>(const Type &binding) {
                for (const char *name: binding.second)
                {
                    this->at(index++) = LunaDescriptorSetLayoutBinding{
                        .bindingName = name,
                        .descriptorType = binding.first,
                        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                    };
                }
            };
            (AddEntry(bindings), ...);
        }
};
template<Concepts::VkDescriptorSetLayoutBindingPair... T> class DescriptorSetLayoutBindings<T...>
    : public std::array<VkDescriptorSetLayoutBinding, sizeof...(T)>
{
    public:
        explicit constexpr DescriptorSetLayoutBindings(const T &...bindings)
        {
            size_t index = 0;

            auto AddEntry = [&index, this]<typename Type>(const Type &binding) {
                this->at(index) = VkDescriptorSetLayoutBinding{
                    .binding = static_cast<uint32_t>(index),
                    .descriptorType = binding.first,
                    .descriptorCount = 1,
                    .stageFlags = static_cast<VkShaderStageFlags>(binding.second),
                };
                index++;
            };
            (AddEntry(bindings), ...);
        }
};

template<auto *BINDINGS> class DescriptorPoolCreationInfo: public LunaDescriptorPoolCreationInfo
{
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
        static constexpr size_t POOL_SIZE_COUNT = ([] -> size_t {
            size_t count = BINDINGS->size();
            for (size_t i = 0; i < BINDINGS->size() - 1; i++)
            {
                const LunaDescriptorSetLayoutBinding &binding = BINDINGS->at(i);
                for (size_t j = i + 1; j < BINDINGS->size(); j++)
                {
                    if (binding.descriptorType == BINDINGS->at(j).descriptorType)
                    {
                        --count;
                        break;
                    }
                }
            }
            return count;
        })();
        static constexpr auto POOL_SIZES = ([] {
            std::array<VkDescriptorPoolSize, POOL_SIZE_COUNT> ret{};
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

    public:
        consteval DescriptorPoolCreationInfo():
            LunaDescriptorPoolCreationInfo{
                .flags = CREATION_FLAGS,
                .maxSets = 1,
                .poolSizeCount = POOL_SIZE_COUNT,
                .poolSizes = POOL_SIZES.data(),
            }
        {}
};
} // namespace

LightBakerGpu::LightBakerGpu()
{
    static constexpr LunaInstanceCreationInfo INSTANCE_CREATION_INFO = {
        .apiVersion = VK_API_VERSION_1_2,
#ifndef NDEBUG
        .enableValidation = true,
#endif
    };
    if (!CheckResult(lunaCreateInstance(&INSTANCE_CREATION_INFO)))
    {
        return;
    }

    static constexpr LunaPhysicalDevicePreferenceDefinition PHYSICAL_DEVICE_PREFERENCE_DEFINITION = {
        .preferredDeviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
    };
    static constexpr VkPhysicalDeviceFeatures REQUIRED_1_0_FEATURES = {
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
    static constexpr VkPhysicalDeviceFeatures2 REQUIRED_FEATURES = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &required12Features,
        .features = REQUIRED_1_0_FEATURES,
    };
    static constexpr std::array REQUIRED_EXTENSIONS = {
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
    };
    static constexpr LunaDeviceCreationInfo2 DEVICE_CREATION_INFO = {
        .extensionCount = REQUIRED_EXTENSIONS.size(),
        .extensionNames = REQUIRED_EXTENSIONS.data(),
        .requiredFeatures = REQUIRED_FEATURES,
        .physicalDevicePreferenceDefinition = &PHYSICAL_DEVICE_PREFERENCE_DEFINITION,
        .allocatorCreateFlags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT |
                                VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
    };
    if (!CheckResult(lunaCreateDevice2(&DEVICE_CREATION_INFO, &device)))
    {
        return;
    }

    constexpr LunaQueueFamilyProperties REQUIRED_PROPERTIES = {
        .queueFamilyProperties = VkQueueFamilyProperties{.queueFlags = VK_QUEUE_COMPUTE_BIT},
    };
    queueFamilyIndex = lunaGetDeviceQueueFamilyIndex(device, &REQUIRED_PROPERTIES);
    vkGetDeviceQueue(lunaGetVkDevice(device), queueFamilyIndex, 0, &queue);

    const LunaCommandPoolCreationInfo commandPoolCreationInfo = {
        .queueFamilyIndex = queueFamilyIndex,
    };
    if (!CheckResult(lunaCreateCommandPool(device, &commandPoolCreationInfo, &commandPool)))
    {
        return;
    }
    if (!CheckResult(lunaAllocateCommandBuffer(device, commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, &commandBuffer)))
    {
        return;
    }

    lunaGetPhysicalDeviceProperties2(device, &physicalDeviceProperties);
    Logger::Info("Vulkan Initialized");
    Logger::Info("Vulkan Device: {}", physicalDeviceProperties.properties.deviceName);
    Logger::Info("Vulkan Version: {}.{}.{}",
                 VK_API_VERSION_MAJOR(physicalDeviceProperties.properties.apiVersion),
                 VK_API_VERSION_MINOR(physicalDeviceProperties.properties.apiVersion),
                 VK_API_VERSION_PATCH(physicalDeviceProperties.properties.apiVersion));

    initialized = true;
}

LightBakerGpu::~LightBakerGpu()
{
    lunaDestroyInstance();
}

static constexpr VmaAllocationCreateInfo VRAM_ALLOCATION_CREATE_INFO = {
    // TODO: Drop the flags when Luna is able to do buffer writes correctly
    .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
    .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
};
static constexpr VmaAllocationCreateInfo MAPPED_ALLOCATION_CREATE_INFO = {
    .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
    .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
};

bool LightBakerGpu::Bake(const std::unordered_map<std::string, LevelMeshBuilder> &meshBuilders,
                         const std::vector<Light> &lights,
                         const glm::uvec2 &lightmapSize,
                         const uint64_t rayCount,
                         const uint32_t bounceCount,
                         std::vector<uint16_t> &pixelData)
{
    const uint64_t width = std::min(rayCount, uint64_t{1} << 15);
    const uint64_t height = std::min(std::max(rayCount / width, uint64_t{1}), uint64_t{1} << 15);
    const uint64_t iterations = std::max(rayCount / width / height, uint64_t{1});

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
        .queueFamilyIndexCount = 1,
        .queueFamilyIndices = &queueFamilyIndex,
        .allocationCreateInfo = &VRAM_ALLOCATION_CREATE_INFO,
    };
    if (!CheckResult(lunaCreateBuffer(device, &lightsBufferCreationInfo, &lightsBuffer)))
    {
        return false;
    }

    const LunaBufferWriteInfo lightsBufferWriteInfo = {
        .bytes = sizeof(Light) * lights.size(),
        .data = lights.data(),
        .stageFlags = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
    };
    if (!CheckResult(lunaWriteDataToBuffer(device, commandBuffer, lightsBuffer, &lightsBufferWriteInfo)))
    {
        return false;
    }

    assert((lightmapSize.x * lightmapSize.y * lights.size()) % sizeof(uint32_t) == 0);
    const LunaBufferCreationInfo lightHitIndicesBufferCreationInfo = {
        .size = (lightmapSize.x * lightmapSize.y * lights.size()) / sizeof(uint32_t),
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .queueFamilyIndexCount = 1,
        .queueFamilyIndices = &queueFamilyIndex,
        .allocationCreateInfo = &VRAM_ALLOCATION_CREATE_INFO,
    };
    if (!CheckResult(lunaCreateBuffer(device, &lightHitIndicesBufferCreationInfo, &lightHitIndicesBuffer)))
    {
        return false;
    }
    if (!CheckResult(lunaFillBuffer(device, commandBuffer, lightHitIndicesBuffer, 0, nullptr)))
    {
        return false;
    }

    const LunaBufferCreationInfo lightmapCreationInfo = {
        .size = lightmapSize.x * lightmapSize.y * sizeof(float) * 3,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .queueFamilyIndexCount = 1,
        .queueFamilyIndices = &queueFamilyIndex,
        .allocationCreateInfo = &VRAM_ALLOCATION_CREATE_INFO,
    };
    if (!CheckResult(lunaCreateBuffer(device, &lightmapCreationInfo, &lightmap)))
    {
        return false;
    }
    if (!CheckResult(lunaFillBuffer(device, commandBuffer, lightmap, 0, nullptr)))
    {
        return false;
    }

    if (!CreateBLAS(meshBuilders))
    {
        return false;
    }
    if (!CreateTLAS())
    {
        return false;
    }
    if (!CreateAndWriteDescriptorSet())
    {
        return false;
    }
    if (!CreatePipeline(lightmapSize, lights.size(), rayCount, bounceCount))
    {
        return false;
    }

    lunaDeviceWaitIdle(device);

    for (uint32_t i = 0; i < lights.size() * iterations; i++)
    {
        Logger::Info("Baking lighting {}%...",
                     static_cast<float>(100 * i) / static_cast<float>(lights.size() * iterations));

        if (!CheckResult(lunaBeginSingleUseCommandBuffer(device, commandBuffer)))
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
            .deviceAddress = lunaGetBufferDeviceAddress(device, raygenShaderBindingTable),
            .stride = physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
            .size = physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
        };
        const VkStridedDeviceAddressRegionKHR closestHitShaderBindingTableAddressRegion = {
            .deviceAddress = lunaGetBufferDeviceAddress(device, closestHitShaderBindingTable),
            .stride = physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
            .size = physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
        };
        vkCmdTraceRaysKHR(vkCommandBuffer,
                          &raygenShaderBindingTableAddressRegion,
                          &STRIDED_DEVICE_ADDRESS_REGION_NONE,
                          &closestHitShaderBindingTableAddressRegion,
                          &STRIDED_DEVICE_ADDRESS_REGION_NONE,
                          width,
                          height,
                          1);
        const LunaCommandBufferSubmitInfo submitInfo = {
            .queue = queue,
            // .stageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        };
        if (!CheckResult(lunaEndAndSubmitCommandBuffer(device, commandBuffer, &submitInfo)))
        {
            return false;
        }
        iteration = (iteration + 1) % iterations;
        if (iteration == 0)
        {
            lightIndex++;
        }
    }

    lunaDeviceWaitIdle(device);
    LunaBuffer outputLightmap{};
    if (!ConvertLightmapToFloat16(lightmapSize, outputLightmap))
    {
        return false;
    }

    Logger::Info("Saving Lightmap...");
    lunaDeviceWaitIdle(device);
    uint16_t *bufferData = static_cast<uint16_t *>(lunaGetBufferDataPointer(outputLightmap));
    pixelData.resize(lunaGetBufferSize(outputLightmap) / 2);
    std::copy_n(bufferData, pixelData.size(), pixelData.data());
    return true;
}

VkShaderModule LightBakerGpu::GenerateShaderModule(const std::filesystem::path &path,
                                                   const EShLanguage shaderType,
                                                   std::vector<uint32_t> &spirv) const
{
    assert(spirv.empty());

    ShaderCompiler shaderCompiler(path, shaderType);
    shaderCompiler.SetTargetVersions(glslang::EShTargetClientVersion::EShTargetVulkan_1_2,
                                     glslang::EShTargetLanguageVersion::EShTargetSpv_1_4);
    if (shaderCompiler.Compile(spirv) != Error::ErrorCode::OK)
    {
        Logger::Error("Error compiling shader {}!", path.string());
        return VK_NULL_HANDLE;
    }

    VkShaderModule shaderModule{};
    const VkShaderModuleCreateInfo shaderModuleCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spirv.size() * sizeof(uint32_t),
        .pCode = spirv.data(),
    };
    if (!CheckResult(vkCreateShaderModule(lunaGetVkDevice(device), &shaderModuleCreateInfo, nullptr, &shaderModule)))
    {
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}

bool LightBakerGpu::CreateBLAS(const std::unordered_map<std::string, LevelMeshBuilder> &meshBuilders)
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
        .queueFamilyIndexCount = 1,
        .queueFamilyIndices = &queueFamilyIndex,
        .allocationCreateInfo = &VRAM_ALLOCATION_CREATE_INFO,
    };
    if (!CheckResult(lunaCreateBuffer(device, &vertexBufferCreationInfo, &vertexBuffer)))
    {
        return false;
    }
    const LunaBufferCreationInfo indexBufferCreationInfo = {
        .size = indexBufferByteCount,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                 VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .queueFamilyIndexCount = 1,
        .queueFamilyIndices = &queueFamilyIndex,
        .allocationCreateInfo = &VRAM_ALLOCATION_CREATE_INFO,
    };
    if (!CheckResult(lunaCreateBuffer(device, &indexBufferCreationInfo, &indexBuffer)))
    {
        return false;
    }

    const LunaBufferWriteInfo vertexBufferWriteInfo = {
        .bytes = vertexBufferByteCount,
        .data = vertices.data(),
        .stageFlags = VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
    };
    if (!CheckResult(lunaWriteDataToBuffer(device, commandBuffer, vertexBuffer, &vertexBufferWriteInfo)))
    {
        return false;
    }
    const LunaBufferWriteInfo indexBufferWriteInfo = {
        .bytes = indexBufferByteCount,
        .data = indices.data(),
        .stageFlags = VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
    };
    if (!CheckResult(lunaWriteDataToBuffer(device, commandBuffer, indexBuffer, &indexBufferWriteInfo)))
    {
        return false;
    }

    if (!CheckResult(lunaDeviceWaitIdle(device)))
    {
        return false;
    }

    if (!CheckResult(lunaBeginSingleUseCommandBuffer(device, commandBuffer)))
    {
        return false;
    }

    const VkDeviceOrHostAddressConstKHR vertexDataAddress = {
        .deviceAddress = lunaGetBufferDeviceAddress(device, vertexBuffer),
    };
    const VkDeviceOrHostAddressConstKHR indexDataAddress = {
        .deviceAddress = lunaGetBufferDeviceAddress(device, indexBuffer),
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
    vkGetAccelerationStructureBuildSizesKHR(lunaGetVkDevice(device),
                                            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                            &buildSizesGeometryInfo,
                                            &triangleCount,
                                            &buildSizesInfo);

    const LunaBufferCreationInfo bufferCreationInfo = {
        .size = buildSizesInfo.accelerationStructureSize,
        .alignment = 256, // 256 is directly required by the spec
        .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .queueFamilyIndexCount = 1,
        .queueFamilyIndices = &queueFamilyIndex,
        .allocationCreateInfo = &VRAM_ALLOCATION_CREATE_INFO,
    };
    if (!CheckResult(lunaCreateBuffer(device, &bufferCreationInfo, &blas.buffer)))
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
    if (!CheckResult(vkCreateAccelerationStructureKHR(lunaGetVkDevice(device),
                                                      &createInfo,
                                                      nullptr,
                                                      &blas.accelerationStructure)))
    {
        return false;
    }

    const LunaBufferCreationInfo scratchBufferCreationInfo = {
        .size = buildSizesInfo.buildScratchSize,
        .alignment = physicalDeviceAccelerationStructureProperties.minAccelerationStructureScratchOffsetAlignment,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .queueFamilyIndexCount = 1,
        .queueFamilyIndices = &queueFamilyIndex,
        .allocationCreateInfo = &VRAM_ALLOCATION_CREATE_INFO,
    };
    if (!CheckResult(lunaCreateBuffer(device, &scratchBufferCreationInfo, &blas.scratchBuffer)))
    {
        return false;
    }

    const VkDeviceOrHostAddressKHR scratchDataAddress = {
        .deviceAddress = lunaGetBufferDeviceAddress(device, blas.scratchBuffer),
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

    const LunaCommandBufferSubmitInfo submitInfo = {
        .queue = queue,
        // .stageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    };
    return CheckResult(lunaEndAndSubmitCommandBuffer(device, commandBuffer, &submitInfo));
}

bool LightBakerGpu::CreateTLAS()
{
    static constexpr VkTransformMatrixKHR TRANSFORM_MATRIX_IDENTITY = {{
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
    }};

    if (!CheckResult(lunaBeginSingleUseCommandBuffer(device, commandBuffer)))
    {
        return false;
    }

    const LunaBufferCreationInfo instancesBufferCreationInfo = {
        .size = sizeof(VkAccelerationStructureInstanceKHR),
        .alignment = 16, // 16 is directly required by the spec
        .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .queueFamilyIndexCount = 1,
        .queueFamilyIndices = &queueFamilyIndex,
        .allocationCreateInfo = &VRAM_ALLOCATION_CREATE_INFO,
    };
    if (!CheckResult(lunaCreateBuffer(device, &instancesBufferCreationInfo, &accelerationStructureInstancesBuffer)))
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
        .accelerationStructureReference = vkGetAccelerationStructureDeviceAddressKHR(lunaGetVkDevice(device),
                                                                                     &deviceAddressInfo),
    };
    const LunaBufferWriteInfo instancesBufferWriteInfo = {
        .bytes = sizeof(instance),
        .data = &instance,
        .stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    };
    if (!CheckResult(lunaWriteDataToBuffer(device,
                                           commandBuffer,
                                           accelerationStructureInstancesBuffer,
                                           &instancesBufferWriteInfo)))
    {
        return false;
    }

    const VkDeviceOrHostAddressConstKHR instancesDataAddress = {
        .deviceAddress = lunaGetBufferDeviceAddress(device, accelerationStructureInstancesBuffer),
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
    vkGetAccelerationStructureBuildSizesKHR(lunaGetVkDevice(device),
                                            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                            &buildSizesGeometryInfo,
                                            &MAX_PRIMITIVE_COUNT,
                                            &buildSizesInfo);

    const LunaBufferCreationInfo bufferCreationInfo = {
        .size = buildSizesInfo.accelerationStructureSize,
        .alignment = 256, // 256 is directly required by the spec
        .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .queueFamilyIndexCount = 1,
        .queueFamilyIndices = &queueFamilyIndex,
        .allocationCreateInfo = &VRAM_ALLOCATION_CREATE_INFO,
    };
    if (!CheckResult(lunaCreateBuffer(device, &bufferCreationInfo, &tlas.buffer)))
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
    if (!CheckResult(vkCreateAccelerationStructureKHR(lunaGetVkDevice(device),
                                                      &createInfo,
                                                      nullptr,
                                                      &tlas.accelerationStructure)))
    {
        return false;
    }

    const LunaBufferCreationInfo scratchBufferCreationInfo = {
        .size = buildSizesInfo.buildScratchSize,
        .alignment = physicalDeviceAccelerationStructureProperties.minAccelerationStructureScratchOffsetAlignment,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .queueFamilyIndexCount = 1,
        .queueFamilyIndices = &queueFamilyIndex,
        .allocationCreateInfo = &VRAM_ALLOCATION_CREATE_INFO,
    };
    if (!CheckResult(lunaCreateBuffer(device, &scratchBufferCreationInfo, &tlas.scratchBuffer)))
    {
        return false;
    }

    const VkDeviceOrHostAddressKHR scratchDataAddress = {
        .deviceAddress = lunaGetBufferDeviceAddress(device, tlas.scratchBuffer),
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
    constexpr VkAccelerationStructureBuildRangeInfoKHR BUILD_RANGE_INFO = {
        .primitiveCount = MAX_PRIMITIVE_COUNT,
    };
    const VkAccelerationStructureBuildRangeInfoKHR *pBuildRangeInfo = &BUILD_RANGE_INFO;
    vkCmdBuildAccelerationStructuresKHR(lunaGetVkCommandBuffer(commandBuffer), 1, &buildGeometryInfo, &pBuildRangeInfo);

    const LunaCommandBufferSubmitInfo submitInfo = {
        .queue = queue,
        // .stageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    };
    return CheckResult(lunaEndAndSubmitCommandBuffer(device, commandBuffer, &submitInfo));
}

bool LightBakerGpu::CreatePipeline(const glm::uvec2 &lightmapSize,
                                   const uint32_t lightCount,
                                   const uint64_t rayCount,
                                   const uint32_t bounceCount)
{
    std::vector<uint32_t> raygenShaderSpirv;
    const VkShaderModule raygenShaderModule = GenerateShaderModule("assets/shaders/lightmap/raygen.rgen",
                                                                   EShLangRayGen,
                                                                   raygenShaderSpirv);
    if (raygenShaderModule == VK_NULL_HANDLE)
    {
        return false;
    }
    std::vector<uint32_t> closestHitShaderSpirv;
    const VkShaderModule closestHitShaderModule = GenerateShaderModule("assets/shaders/lightmap/closesthit.rchit",
                                                                       EShLangClosestHit,
                                                                       closestHitShaderSpirv);
    if (closestHitShaderModule == VK_NULL_HANDLE)
    {
        return false;
    }

    const struct
    {
            double rayCount;
            uint32_t iterationCount;
            uint32_t bounces;
    } raygenSpecializationData = {
        .rayCount = static_cast<double>(rayCount),
        .iterationCount = static_cast<uint32_t>(std::max(rayCount / (uint64_t{1} << 30), uint64_t{1})),
        .bounces = bounceCount,
    };
    static constexpr SpecializationMapEntries<double, uint32_t, uint32_t> RAYGEN_MAP_ENTRIES{};
    const VkSpecializationInfo raygenSpecializationInfo = {
        .mapEntryCount = RAYGEN_MAP_ENTRIES.size(),
        .pMapEntries = RAYGEN_MAP_ENTRIES.data(),
        .dataSize = sizeof(raygenSpecializationData),
        .pData = &raygenSpecializationData,
    };
    const std::array closestHitSpecializationData = {
        lightmapSize.x,
        lightmapSize.y,
        lightCount,
    };
    static constexpr SpecializationMapEntries<uint32_t, uint32_t, uint32_t> CLOSEST_HIT_MAP_ENTRIES{};
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

    constexpr std::array SHADER_GROUPS = {
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

    constexpr std::array PUSH_CONSTANT_RANGES = {
        VkPushConstantRange{
            .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
            .size = sizeof(lightIndex) + sizeof(iteration),
        },
    };
    const VkPipelineLayoutCreateInfo layoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptorSetLayout,
        .pushConstantRangeCount = PUSH_CONSTANT_RANGES.size(),
        .pPushConstantRanges = PUSH_CONSTANT_RANGES.data(),
    };
    if (!CheckResult(vkCreatePipelineLayout(lunaGetVkDevice(device), &layoutCreateInfo, nullptr, &pipelineLayout)))
    {
        return false;
    }

    const VkRayTracingPipelineCreateInfoKHR pipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
        .stageCount = shaderStages.size(),
        .pStages = shaderStages.data(),
        .groupCount = SHADER_GROUPS.size(),
        .pGroups = SHADER_GROUPS.data(),
        .maxPipelineRayRecursionDepth = 1,
        .layout = pipelineLayout,
    };
    if (!CheckResult(vkCreateRayTracingPipelinesKHR(lunaGetVkDevice(device),
                                                    VK_NULL_HANDLE,
                                                    VK_NULL_HANDLE,
                                                    1,
                                                    &pipelineCreateInfo,
                                                    nullptr,
                                                    &pipeline)))
    {
        return false;
    }

    vkDestroyShaderModule(lunaGetVkDevice(device), raygenShaderModule, nullptr);
    vkDestroyShaderModule(lunaGetVkDevice(device), closestHitShaderModule, nullptr);

    return CreateShaderBindingTables();
}

bool LightBakerGpu::CreateAndWriteDescriptorSet()
{
    static constexpr DescriptorSetLayoutBindings DESCRIPTOR_SET_LAYOUT_BINDINGS{
        std::pair{
            VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
        },
        std::pair{
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        },
        std::pair{
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
        },
        std::pair{
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
        },
        std::pair{
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
        },
        std::pair{
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
        },
    };

    const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = DESCRIPTOR_SET_LAYOUT_BINDINGS.size(),
        .pBindings = DESCRIPTOR_SET_LAYOUT_BINDINGS.data(),
    };
    if (!CheckResult(vkCreateDescriptorSetLayout(lunaGetVkDevice(device),
                                                 &descriptorSetLayoutCreateInfo,
                                                 nullptr,
                                                 &descriptorSetLayout)))
    {
        return false;
    }

    constexpr std::array DESCRIPTOR_POOL_SIZES = {
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
        .poolSizeCount = DESCRIPTOR_POOL_SIZES.size(),
        .pPoolSizes = DESCRIPTOR_POOL_SIZES.data(),
    };
    if (!CheckResult(vkCreateDescriptorPool(lunaGetVkDevice(device),
                                            &descriptorPoolCreateInfo,
                                            nullptr,
                                            &descriptorPool)))
    {
        return false;
    }

    const VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &descriptorSetLayout,
    };
    if (!CheckResult(vkAllocateDescriptorSets(lunaGetVkDevice(device), &descriptorSetAllocateInfo, &descriptorSet)))
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
    vkUpdateDescriptorSets(lunaGetVkDevice(device), descriptorSetWrites.size(), descriptorSetWrites.data(), 0, nullptr);

    return true;
}

bool LightBakerGpu::CreateShaderBindingTables()
{
    static constexpr VkBufferUsageFlags USAGE_FLAGS = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR |
                                                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                                                      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    const LunaBufferCreationInfo raygenShaderBindingTableCreationInfo = {
        .size = physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
        .alignment = physicalDeviceRayTracingPipelineProperties.shaderGroupBaseAlignment,
        .usage = USAGE_FLAGS,
        .queueFamilyIndexCount = 1,
        .queueFamilyIndices = &queueFamilyIndex,
        .allocationCreateInfo = &VRAM_ALLOCATION_CREATE_INFO,
    };
    if (!CheckResult(lunaCreateBuffer(device, &raygenShaderBindingTableCreationInfo, &raygenShaderBindingTable)))
    {
        return false;
    }

    const LunaBufferCreationInfo closestHitShaderBindingTableCreationInfo = {
        .size = physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
        .alignment = physicalDeviceRayTracingPipelineProperties.shaderGroupBaseAlignment,
        .usage = USAGE_FLAGS,
        .queueFamilyIndexCount = 1,
        .queueFamilyIndices = &queueFamilyIndex,
        .allocationCreateInfo = &VRAM_ALLOCATION_CREATE_INFO,
    };
    if (!CheckResult(lunaCreateBuffer(device,
                                      &closestHitShaderBindingTableCreationInfo,
                                      &closestHitShaderBindingTable)))
    {
        return false;
    }

    std::vector<char> shaderHandles(2 * physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize);
    if (!CheckResult(vkGetRayTracingShaderGroupHandlesKHR(lunaGetVkDevice(device),
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
        .stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    };
    if (!CheckResult(lunaWriteDataToBuffer(device,
                                           commandBuffer,
                                           raygenShaderBindingTable,
                                           &raygenShaderBindingTableWriteInfo)))
    {
        return false;
    }

    const LunaBufferWriteInfo closestHitShaderBindingTableWriteInfo = {
        .bytes = physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
        .data = shaderHandles.data() + physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
        .stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    };
    return CheckResult(lunaWriteDataToBuffer(device,
                                             commandBuffer,
                                             closestHitShaderBindingTable,
                                             &closestHitShaderBindingTableWriteInfo));
}

bool LightBakerGpu::ConvertLightmapToFloat16(const glm::uvec2 &lightmapSize, LunaBuffer &outputLightmap) const
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
    if (!CheckResult(lunaCreateShaderModule(device, &shaderModuleCreationInfo, &shaderModule)))
    {
        return false;
    }

    LunaDescriptorSetLayout descriptorSetLayout{};
    static constexpr DescriptorSetLayoutBindings DESCRIPTOR_SET_LAYOUT_BINDINGS{
        std::pair{
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            std::array{"input lightmap"},
        },
        std::pair{
            VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
            std::array{"output lightmap"},
        },
    };
    constexpr LunaDescriptorSetLayoutCreationInfo DESCRIPTOR_SET_LAYOUT_CREATION_INFO = {
        .bindingCount = DESCRIPTOR_SET_LAYOUT_BINDINGS.size(),
        .bindings = DESCRIPTOR_SET_LAYOUT_BINDINGS.data(),
    };
    if (!CheckResult(lunaCreateDescriptorSetLayout(device, &DESCRIPTOR_SET_LAYOUT_CREATION_INFO, &descriptorSetLayout)))
    {
        return false;
    }

    const std::array specializationData = {
        static_cast<uint32_t>(lightmapSize.x),
        WORK_GROUP_SIZE.x,
        WORK_GROUP_SIZE.y,
    };
    static constexpr SpecializationMapEntries<uint32_t, uint32_t, uint32_t> MAP_ENTRIES{};
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
    if (!CheckResult(lunaCreateComputePipeline(device, &pipelineCreationInfo, &pipeline)))
    {
        return false;
    }

    LunaDescriptorPool descriptorPool{};
    static constexpr DescriptorPoolCreationInfo<&DESCRIPTOR_SET_LAYOUT_BINDINGS> DESCRIPTOR_POOL_CREATION_INFO{};
    if (!CheckResult(lunaCreateDescriptorPool(device, &DESCRIPTOR_POOL_CREATION_INFO, &descriptorPool)))
    {
        return false;
    }

    const LunaDescriptorSetAllocationInfo allocationInfo = {
        .descriptorPool = descriptorPool,
        .setLayoutCount = 1,
        .setLayouts = &descriptorSetLayout,
    };
    LunaDescriptorSet descriptorSet{};
    if (!CheckResult(lunaAllocateDescriptorSets(device, &allocationInfo, &descriptorSet)))
    {
        return false;
    }

    const LunaBufferCreationInfo levelGeometryLightmapCreationInfo = {
        .size = lightmapSize.x * lightmapSize.y * sizeof(_Float16) * 4,
        .usage = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
        .queueFamilyIndexCount = 1,
        .queueFamilyIndices = &queueFamilyIndex,
        .allocationCreateInfo = &MAPPED_ALLOCATION_CREATE_INFO,
    };
    if (!CheckResult(lunaCreateBuffer(device, &levelGeometryLightmapCreationInfo, &outputLightmap)))
    {
        return false;
    }

    LunaBufferView outputLightmapBufferView{};
    const LunaBufferViewCreationInfo outputLightmapBufferViewCreationInfo = {
        .buffer = outputLightmap,
        .format = VK_FORMAT_R16G16B16A16_SFLOAT,
    };
    if (!CheckResult(lunaCreateBufferView(device, &outputLightmapBufferViewCreationInfo, &outputLightmapBufferView)))
    {
        return false;
    }

    const LunaDescriptorBufferInfo inputLightmapBufferInfo{
        .buffer = lightmap,
    };
    const std::array<LunaWriteDescriptorSet, 2> writes{
        LunaWriteDescriptorSet{
            .descriptorSet = descriptorSet,
            .bindingName = "input lightmap",
            .bufferInfo = &inputLightmapBufferInfo,
        },
        LunaWriteDescriptorSet{
            .descriptorSet = descriptorSet,
            .bindingName = "output lightmap",
            .texelBufferView = outputLightmapBufferView,
        },
    };
    lunaWriteDescriptorSets(device, writes.size(), writes.data());

    const LunaDescriptorSetBindInfo descriptorSetBindInfo = {
        .descriptorSetCount = 1,
        .descriptorSets = &descriptorSet,
    };
    assert(lightmapSize.x % WORK_GROUP_SIZE.x == 0 && lightmapSize.y % WORK_GROUP_SIZE.y == 0);
    const LunaCommandBufferSubmitInfo submitInfo = {
        .queue = queue,
        // .stageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    };
    const LunaDispatchInfo dispatchInfo = {
        .pipeline = pipeline,
        .descriptorSetBindInfo = &descriptorSetBindInfo,
        .groupCountX = lightmapSize.x / WORK_GROUP_SIZE.x,
        .groupCountY = lightmapSize.y / WORK_GROUP_SIZE.y,
        .submitInfo = &submitInfo,
    };
    return CheckResult(lunaDispatch(device, commandBuffer, &dispatchInfo));
}
