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
#include <luna/lunaSynchronization.h>
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
constexpr VkPipelineStageFlagBits2 WAIT_STAGE = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT |
                                                VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
std::vector<uint32_t> spirv{};

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
        .queueFamilyProperties = VkQueueFamilyProperties{.queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT},
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

    constexpr LunaSemaphoreCreationInfo SEMAPHORE_CREATION_INFO{};
    if (!CheckResult(lunaCreateSemaphore(device, &SEMAPHORE_CREATION_INFO, &semaphore)))
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
                         const uint32_t bounceCount,
                         const uint32_t sampleCount,
                         std::vector<uint16_t> &pixelData)
{
    const uint64_t luxelCount = lightmapSize.x * lightmapSize.y;
    const uint64_t width = std::min(luxelCount, uint64_t{1} << 10);
    const uint64_t height = std::min(std::max(luxelCount / width, uint64_t{1}), uint64_t{1} << 10);
    const uint32_t iterations = std::max(luxelCount / width / height, uint64_t{1});

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

    const LunaBufferCreationInfo lightmapCreationInfo = {
        .size = lightmapSize.x * lightmapSize.y * sizeof(_Float16) * 4,
        .usage = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
        .queueFamilyIndexCount = 1,
        .queueFamilyIndices = &queueFamilyIndex,
        .allocationCreateInfo = &MAPPED_ALLOCATION_CREATE_INFO,
    };
    if (!CheckResult(lunaCreateBuffer(device, &lightmapCreationInfo, &lightmapOne)))
    {
        return false;
    }
    if (!CheckResult(lunaFillBuffer(device, commandBuffer, lightmapOne, 0, nullptr)))
    {
        return false;
    }
    const LunaBufferViewCreationInfo lightmapOneBufferViewCreationInfo = {
        .buffer = lightmapOne,
        .format = VK_FORMAT_R16G16B16A16_SFLOAT,
    };
    LunaBufferView lightmapOneLunaBufferView{};
    if (!CheckResult(lunaCreateBufferView(device, &lightmapOneBufferViewCreationInfo, &lightmapOneLunaBufferView)))
    {
        return false;
    }
    lightmapOneBufferView = lunaGetVkBufferView(lightmapOneLunaBufferView);

    if (!CheckResult(lunaCreateBuffer(device, &lightmapCreationInfo, &lightmapTwo)))
    {
        return false;
    }
    if (!CheckResult(lunaFillBuffer(device, commandBuffer, lightmapTwo, 0, nullptr)))
    {
        return false;
    }
    const LunaBufferViewCreationInfo lightmapTwoBufferViewCreationInfo = {
        .buffer = lightmapTwo,
        .format = VK_FORMAT_R16G16B16A16_SFLOAT,
    };
    LunaBufferView lightmapTwoLunaBufferView{};
    if (!CheckResult(lunaCreateBufferView(device, &lightmapTwoBufferViewCreationInfo, &lightmapTwoLunaBufferView)))
    {
        return false;
    }
    lightmapTwoBufferView = lunaGetVkBufferView(lightmapTwoLunaBufferView);

    uint32_t vertexCount{};
    uint32_t indexCount{};
    if (!CreateVertexAndIndexBuffers(meshBuilders, vertexCount, indexCount))
    {
        return false;
    }

    if (!PrecomputeLuxelInformation(lightmapSize, indexCount))
    {
        return false;
    }

    if (!CreateBLAS(vertexCount, indexCount))
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
    if (!CreateDirectLightingPipeline(lightmapSize, lights.size()))
    {
        return false;
    }
    if (!CreateGlobalIlluminationPipeline(lightmapSize, sampleCount))
    {
        return false;
    }
    if (!CreateShaderBindingTables())
    {
        return false;
    }

    lunaDeviceWaitIdle(device);

    const std::chrono::time_point<std::chrono::system_clock> start = std::chrono::high_resolution_clock::now();
    for (uint32_t iteration = 0; iteration < iterations; iteration++)
    {
        if (!SingleBakeIteration(width,
                                 height,
                                 static_cast<float>(100 * iteration) /
                                         static_cast<float>(iterations * (bounceCount + 1)),
                                 width * height * iteration,
                                 true))
        {
            return false;
        }
    }
    for (uint32_t bounce = 0; bounce < bounceCount; bounce++)
    {
        lunaDeviceWaitIdle(device);
        const std::array descriptorSetWrites = {
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptorSet,
                .dstBinding = (bounce + 1) % 2 + 1,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
                .pTexelBufferView = &lightmapOneBufferView,
            },
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptorSet,
                .dstBinding = bounce % 2 + 1,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
                .pTexelBufferView = &lightmapTwoBufferView,
            },
        };
        vkUpdateDescriptorSets(lunaGetVkDevice(device),
                               descriptorSetWrites.size(),
                               descriptorSetWrites.data(),
                               0,
                               nullptr);
        for (uint32_t iteration = 0; iteration < iterations; iteration++)
        {
            if (!SingleBakeIteration(width,
                                     height,
                                     static_cast<float>(100 * ((bounce + 1) * iterations + iteration)) /
                                             static_cast<float>(iterations * (bounceCount + 1)),
                                     width * height * iteration,
                                     false))
            {
                return false;
            }
        }
    }
    lunaDeviceWaitIdle(device);
    const std::chrono::time_point<std::chrono::system_clock> end = std::chrono::high_resolution_clock::now();
    Logger::Info("Compiled in {}us", std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());

    Logger::Info("Saving Lightmap...");
    const LunaBuffer finalLightmap = bounceCount % 2 == 0 ? lightmapTwo : lightmapOne;
    uint16_t *bufferData = static_cast<uint16_t *>(lunaGetBufferDataPointer(finalLightmap));
    pixelData.resize(lunaGetBufferSize(finalLightmap) / 2);
    std::copy_n(bufferData, pixelData.size(), pixelData.data());
    return true;
}

VkShaderModule LightBakerGpu::GenerateShaderModule(const std::filesystem::path &path,
                                                   const shaderc_shader_kind shaderKind) const
{
    spirv.clear();

    ShaderCompiler shaderCompiler(path, shaderKind, true);
    if (shaderCompiler.Compile(spirv) != Error::ErrorCode::OK)
    {
        Logger::Error("Error compiling shader {}!", path.string());
        Logger::Info("{}", shaderCompiler.GetErrorMessage());
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

bool LightBakerGpu::CreateVertexAndIndexBuffers(const std::unordered_map<std::string, LevelMeshBuilder> &meshBuilders,
                                                uint32_t &vertexCount,
                                                uint32_t &indexCount)
{
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
    vertexCount = vertices.size();
    indexCount = indices.size();

    const size_t vertexBufferByteCount = vertices.size() * sizeof(MapVertex);
    const size_t indexBufferByteCount = indices.size() * sizeof(uint32_t);

    const LunaBufferCreationInfo vertexBufferCreationInfo = {
        .size = vertexBufferByteCount,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
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
        .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
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

    return CheckResult(lunaDeviceWaitIdle(device));
}

bool LightBakerGpu::PrecomputeLuxelInformation(const glm::uvec2 &lightmapSize, const uint32_t indexCount)
{
    static constexpr size_t ATTACHMENT_COUNT = 3;
    static constexpr VkDeviceSize OFFSET = 0;

    const VkDevice vkDevice = lunaGetVkDevice(device);
    const VkCommandBuffer vkCommandBuffer = lunaGetVkCommandBuffer(commandBuffer);
    const VkBuffer vkVertexBuffer = lunaGetVkBuffer(vertexBuffer);
    const VkBuffer vkIndexBuffer = lunaGetVkBuffer(indexBuffer);

    static constexpr VkAttachmentDescription LUXEL_INFORMATION_ATTACHMENT_DESCRIPTION = {
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_GENERAL,
        .finalLayout = VK_IMAGE_LAYOUT_GENERAL,
    };
    static constexpr VkAttachmentDescription LUXEL_ALBEDO_ATTACHMENT_DESCRIPTION = {
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_GENERAL,
        .finalLayout = VK_IMAGE_LAYOUT_GENERAL,
    };
    static constexpr std::array<VkAttachmentDescription, ATTACHMENT_COUNT> ATTACHMENT_DESCRIPTIONS = {
        LUXEL_INFORMATION_ATTACHMENT_DESCRIPTION,
        LUXEL_INFORMATION_ATTACHMENT_DESCRIPTION,
        LUXEL_ALBEDO_ATTACHMENT_DESCRIPTION,
    };
    static constexpr std::array<VkAttachmentReference, ATTACHMENT_COUNT> ATTACHMENT_REFERENCES = {
        VkAttachmentReference{
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_GENERAL,
        },
        VkAttachmentReference{
            .attachment = 1,
            .layout = VK_IMAGE_LAYOUT_GENERAL,
        },
        VkAttachmentReference{
            .attachment = 2,
            .layout = VK_IMAGE_LAYOUT_GENERAL,
        },
    };
    static constexpr VkSubpassDescription SUBPASS = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = ATTACHMENT_REFERENCES.size(),
        .pColorAttachments = ATTACHMENT_REFERENCES.data(),
    };
    static constexpr VkSubpassDependency DEPENDENCY = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };
    const VkRenderPassCreateInfo renderPassCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = ATTACHMENT_DESCRIPTIONS.size(),
        .pAttachments = ATTACHMENT_DESCRIPTIONS.data(),
        .subpassCount = 1,
        .pSubpasses = &SUBPASS,
        .dependencyCount = 1,
        .pDependencies = &DEPENDENCY,
    };
    VkRenderPass renderPass{};
    if (!CheckResult(vkCreateRenderPass(vkDevice, &renderPassCreateInfo, nullptr, &renderPass)))
    {
        return false;
    }

    const LunaCommandBufferSubmitInfo submitInfo = {
        .queue = queue,
        .waitSemaphoreCount = 1,
        .waitSemaphores = &semaphore,
        .waitDstStageMasks = &WAIT_STAGE,
        .signalSemaphoreCount = 1,
        .signalSemaphores = &semaphore,
    };
    const LunaImageWriteInfo writeInfo = {
        .destinationStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
                                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .destinationAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .submitInfo = &submitInfo,
    };
    const LunaSamplerCreationInfo samplerCreationInfo = {};
    const LunaImageCreationInfo luxelInformationImageCreateInfo = {
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .width = lightmapSize.x,
        .height = lightmapSize.y,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        .queueFamilyIndexCount = 1,
        .queueFamilyIndices = &queueFamilyIndex,
        .layout = VK_IMAGE_LAYOUT_GENERAL,
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .writeInfo = writeInfo,
        .samplerCreationInfo = &samplerCreationInfo,
    };
    if (!CheckResult(lunaCreateImage(device, commandBuffer, &luxelInformationImageCreateInfo, &luxelPositionsImage)))
    {
        return false;
    }
    if (!CheckResult(lunaCreateImage(device, commandBuffer, &luxelInformationImageCreateInfo, &luxelNormalsImage)))
    {
        return false;
    }
    const LunaImageCreationInfo luxelAlbedosImageCreateInfo = {
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .width = lightmapSize.x,
        .height = lightmapSize.y,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        .queueFamilyIndexCount = 1,
        .queueFamilyIndices = &queueFamilyIndex,
        .layout = VK_IMAGE_LAYOUT_GENERAL,
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .writeInfo = writeInfo,
        .samplerCreationInfo = &samplerCreationInfo,
    };
    if (!CheckResult(lunaCreateImage(device, commandBuffer, &luxelAlbedosImageCreateInfo, &luxelAlbedosImage)))
    {
        return false;
    }

    const std::array<VkImageView, ATTACHMENT_COUNT> attachmentImageViews = {
        lunaGetVkImageView(luxelPositionsImage),
        lunaGetVkImageView(luxelNormalsImage),
        lunaGetVkImageView(luxelAlbedosImage),
    };
    const VkFramebufferCreateInfo framebufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = renderPass,
        .attachmentCount = attachmentImageViews.size(),
        .pAttachments = attachmentImageViews.data(),
        .width = lightmapSize.x,
        .height = lightmapSize.y,
        .layers = 1,
    };
    VkFramebuffer framebuffer{};
    if (!CheckResult(vkCreateFramebuffer(vkDevice, &framebufferCreateInfo, nullptr, &framebuffer)))
    {
        return false;
    }

    static constexpr VkPipelineLayoutCreateInfo PIPELINE_LAYOUT_CREATE_INFO = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    };
    VkPipelineLayout graphicsPipelineLayout{};
    if (!CheckResult(vkCreatePipelineLayout(vkDevice, &PIPELINE_LAYOUT_CREATE_INFO, nullptr, &graphicsPipelineLayout)))
    {
        return false;
    }

    const VkShaderModule vertexShaderModule = GenerateShaderModule("assets/shaders/lightmap/"
                                                                   "precompute_luxel_information.vert",
                                                                   shaderc_vertex_shader);
    if (vertexShaderModule == VK_NULL_HANDLE)
    {
        return false;
    }
    const VkShaderModule fragmentShaderModule = GenerateShaderModule("assets/shaders/lightmap/"
                                                                     "precompute_luxel_information.frag",
                                                                     shaderc_fragment_shader);
    if (fragmentShaderModule == VK_NULL_HANDLE)
    {
        return false;
    }

    const std::array shaderStages = {
        VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertexShaderModule,
            .pName = "main",
        },
        VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragmentShaderModule,
            .pName = "main",
        },
    };
    static constexpr VkVertexInputBindingDescription VERTEX_INPUT_BINDING_DESCRIPTION = {
        .stride = sizeof(MapVertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };
    static constexpr std::array VERTEX_INPUT_ATTRIBUTE_DESCRIPTIONS = {
        VkVertexInputAttributeDescription{
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(MapVertex, lightmapUv),
        },
        VkVertexInputAttributeDescription{
            .location = 1,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(MapVertex, position),
        },
        VkVertexInputAttributeDescription{
            .location = 2,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(MapVertex, normal),
        },
    };
    const VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &VERTEX_INPUT_BINDING_DESCRIPTION,
        .vertexAttributeDescriptionCount = VERTEX_INPUT_ATTRIBUTE_DESCRIPTIONS.size(),
        .pVertexAttributeDescriptions = VERTEX_INPUT_ATTRIBUTE_DESCRIPTIONS.data(),
    };
    const VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };
    const VkViewport viewport = {
        .width = static_cast<float>(lightmapSize.x),
        .height = static_cast<float>(lightmapSize.y),
        .maxDepth = 1.0f,
    };
    const VkRect2D scissors = {
        .extent = VkExtent2D{.width = lightmapSize.x, .height = lightmapSize.y},
    };
    const VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissors,
    };
    const VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1,
    };
    const VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };
    const VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    };
    static constexpr VkPipelineColorBlendAttachmentState COLOR_BLEND_ATTACHMENT = {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                          VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT |
                          VK_COLOR_COMPONENT_A_BIT,
    };
    static constexpr std::array<VkPipelineColorBlendAttachmentState, ATTACHMENT_COUNT> COLOR_BLEND_ATTACHMENTS = {
        COLOR_BLEND_ATTACHMENT,
        COLOR_BLEND_ATTACHMENT,
        COLOR_BLEND_ATTACHMENT,
    };
    const VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = COLOR_BLEND_ATTACHMENTS.size(),
        .pAttachments = COLOR_BLEND_ATTACHMENTS.data(),
    };
    const VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = shaderStages.size(),
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInputStateCreateInfo,
        .pInputAssemblyState = &inputAssemblyStateCreateInfo,
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &rasterizationStateCreateInfo,
        .pMultisampleState = &multisampleStateCreateInfo,
        .pDepthStencilState = &depthStencilStateCreateInfo,
        .pColorBlendState = &colorBlendStateCreateInfo,
        .layout = graphicsPipelineLayout,
        .renderPass = renderPass,
    };
    VkPipeline graphicsPipeline{};
    if (!CheckResult(vkCreateGraphicsPipelines(vkDevice,
                                               VK_NULL_HANDLE,
                                               1,
                                               &graphicsPipelineCreateInfo,
                                               nullptr,
                                               &graphicsPipeline)))
    {
        return false;
    }

    if (!CheckResult(lunaBeginSingleUseCommandBuffer(device, commandBuffer)))
    {
        return false;
    }
    static constexpr std::array<VkClearValue, ATTACHMENT_COUNT> CLEAR_VALUES{};
    const VkRenderPassBeginInfo renderPassBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = renderPass,
        .framebuffer = framebuffer,
        .renderArea = VkRect2D{.extent = VkExtent2D{.width = lightmapSize.x, .height = lightmapSize.y}},
        .clearValueCount = CLEAR_VALUES.size(),
        .pClearValues = CLEAR_VALUES.data(),
    };
    vkCmdBeginRenderPass(vkCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    vkCmdBindVertexBuffers(vkCommandBuffer, 0, 1, &vkVertexBuffer, &OFFSET);
    vkCmdBindIndexBuffer(vkCommandBuffer, vkIndexBuffer, OFFSET, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(vkCommandBuffer, indexCount, 1, 0, 0, 0);

    vkCmdEndRenderPass(vkCommandBuffer);

    if (!CheckResult(lunaEndAndSubmitCommandBuffer(device, commandBuffer, &submitInfo)))
    {
        return false;
    }

    return true;
}

bool LightBakerGpu::CreateBLAS(const uint32_t vertexCount, const uint32_t indexCount)
{
    // The position MUST be at the start of the struct for the acceleration structure to work properly
    static_assert(offsetof(MapVertex, position) == 0, "MapVertex::position must be at offset zero!");
    // The position of the vertex must be three single precision floating point numbers,
    //  since we use VK_FORMAT_R32G32B32_SFLOAT as the vertex format.
    //  If the type used for position changes, update the vertex format accordingly.
    static_assert(std::same_as<decltype(MapVertex::position), glm::vec3>,
                  "MapVertex::position must be a three-component vector of single precision floats!");


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
        .maxVertex = vertexCount - 1,
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
    const uint32_t triangleCount = indexCount / 3;
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
        .waitSemaphoreCount = 1,
        .waitSemaphores = &semaphore,
        .waitDstStageMasks = &WAIT_STAGE,
        .signalSemaphoreCount = 1,
        .signalSemaphores = &semaphore,
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
        .waitSemaphoreCount = 1,
        .waitSemaphores = &semaphore,
        .waitDstStageMasks = &WAIT_STAGE,
        .signalSemaphoreCount = 1,
        .signalSemaphores = &semaphore,
    };
    return CheckResult(lunaEndAndSubmitCommandBuffer(device, commandBuffer, &submitInfo));
}

bool LightBakerGpu::CreateDirectLightingPipeline(const glm::uvec2 &lightmapSize, const uint32_t lightCount)
{
    const VkShaderModule raygenShaderModule = GenerateShaderModule("assets/shaders/lightmap/"
                                                                   "direct_lighting.rgen",
                                                                   shaderc_raygen_shader);
    if (raygenShaderModule == VK_NULL_HANDLE)
    {
        return false;
    }
    const VkShaderModule missShaderModule = GenerateShaderModule("assets/shaders/lightmap/miss.rmiss",
                                                                 shaderc_miss_shader);
    if (missShaderModule == VK_NULL_HANDLE)
    {
        return false;
    }


    const std::array raygenSpecializationData = {
        lightmapSize.x,
        lightCount,
    };
    static constexpr SpecializationMapEntries<uint32_t, uint32_t> RAYGEN_MAP_ENTRIES{};
    const VkSpecializationInfo raygenSpecializationInfo = {
        .mapEntryCount = RAYGEN_MAP_ENTRIES.size(),
        .pMapEntries = RAYGEN_MAP_ENTRIES.data(),
        .dataSize = sizeof(raygenSpecializationData),
        .pData = raygenSpecializationData.data(),
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
            .stage = VK_SHADER_STAGE_MISS_BIT_KHR,
            .module = missShaderModule,
            .pName = "main",
        },
    };

    static constexpr std::array SHADER_GROUPS = {
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
            .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
            .generalShader = 1,
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR,
        },
    };

    static constexpr VkPushConstantRange PUSH_CONSTANT_RANGE = {
        .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        .size = sizeof(uint32_t),
    };
    const VkPipelineLayoutCreateInfo layoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptorSetLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &PUSH_CONSTANT_RANGE,
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
                                                    &directLightingPipeline)))
    {
        return false;
    }

    vkDestroyShaderModule(lunaGetVkDevice(device), raygenShaderModule, nullptr);
    vkDestroyShaderModule(lunaGetVkDevice(device), missShaderModule, nullptr);

    return true;
}

bool LightBakerGpu::CreateGlobalIlluminationPipeline(const glm::uvec2 &lightmapSize, const uint32_t sampleCount)
{
    const VkShaderModule raygenShaderModule = GenerateShaderModule("assets/shaders/lightmap/"
                                                                   "global_illumination.rgen",
                                                                   shaderc_raygen_shader);
    if (raygenShaderModule == VK_NULL_HANDLE)
    {
        return false;
    }
    const VkShaderModule closestHitShaderModule = GenerateShaderModule("assets/shaders/lightmap/closesthit.rchit",
                                                                       shaderc_closesthit_shader);
    if (closestHitShaderModule == VK_NULL_HANDLE)
    {
        return false;
    }


    const std::array raygenSpecializationData = {
        lightmapSize.x,
        sampleCount,
    };
    static constexpr SpecializationMapEntries<uint32_t, uint32_t> RAYGEN_MAP_ENTRIES{};
    const VkSpecializationInfo raygenSpecializationInfo = {
        .mapEntryCount = RAYGEN_MAP_ENTRIES.size(),
        .pMapEntries = RAYGEN_MAP_ENTRIES.data(),
        .dataSize = sizeof(raygenSpecializationData),
        .pData = raygenSpecializationData.data(),
    };
    static constexpr SpecializationMapEntries<uint32_t, uint32_t> CLOSEST_HIT_MAP_ENTRIES{};
    const VkSpecializationInfo closestHitSpecializationInfo = {
        .mapEntryCount = CLOSEST_HIT_MAP_ENTRIES.size(),
        .pMapEntries = CLOSEST_HIT_MAP_ENTRIES.data(),
        .dataSize = sizeof(lightmapSize),
        .pData = &lightmapSize,
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

    static constexpr std::array SHADER_GROUPS = {
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

    static constexpr VkPushConstantRange PUSH_CONSTANT_RANGE = {
        .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        .size = sizeof(uint32_t),
    };
    const VkPipelineLayoutCreateInfo layoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptorSetLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &PUSH_CONSTANT_RANGE,
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
                                                    &globalIlluminationPipeline)))
    {
        return false;
    }

    vkDestroyShaderModule(lunaGetVkDevice(device), raygenShaderModule, nullptr);
    vkDestroyShaderModule(lunaGetVkDevice(device), closestHitShaderModule, nullptr);

    return true;
}

bool LightBakerGpu::CreateAndWriteDescriptorSet()
{
    static constexpr DescriptorSetLayoutBindings DESCRIPTOR_SET_LAYOUT_BINDINGS{
        std::pair{
            // accelerationStructure
            VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        },
        std::pair{
            // inputLightmap
            VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
        },
        std::pair{
            // outputLightmap
            VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        },
        std::pair{
            // luxelPositions
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        },
        std::pair{
            // luxelNormals
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        },
        std::pair{
            // luxelAlbedos
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
        },
        std::pair{
            // lightsData
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        },
        std::pair{
            // vertexData
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
        },
        std::pair{
            // indexData
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
            .type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
            .descriptorCount = 2,
        },
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 3,
        },
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 3,
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
    const VkDescriptorImageInfo luxelPositionsImageInfo = {
        .sampler = lunaGetVkSampler(luxelPositionsImage),
        .imageView = lunaGetVkImageView(luxelPositionsImage),
        .imageLayout = lunaGetImageLayout(luxelPositionsImage),
    };
    const VkDescriptorImageInfo luxelNormalsImageInfo = {
        .sampler = lunaGetVkSampler(luxelNormalsImage),
        .imageView = lunaGetVkImageView(luxelNormalsImage),
        .imageLayout = lunaGetImageLayout(luxelNormalsImage),
    };
    const VkDescriptorImageInfo luxelAlbedosImageInfo = {
        .sampler = lunaGetVkSampler(luxelAlbedosImage),
        .imageView = lunaGetVkImageView(luxelAlbedosImage),
        .imageLayout = lunaGetImageLayout(luxelAlbedosImage),
    };
    const VkDescriptorBufferInfo lightsBufferInfo = {
        .buffer = lunaGetVkBuffer(lightsBuffer),
        .offset = lunaGetBufferOffset(lightsBuffer),
        .range = lunaGetBufferSize(lightsBuffer),
    };
    const VkDescriptorBufferInfo vertexBufferInfo = {
        .buffer = lunaGetVkBuffer(vertexBuffer),
        .offset = lunaGetBufferOffset(vertexBuffer),
        .range = lunaGetBufferSize(vertexBuffer),
    };
    const VkDescriptorBufferInfo indexBufferInfo = {
        .buffer = lunaGetVkBuffer(indexBuffer),
        .offset = lunaGetBufferOffset(indexBuffer),
        .range = lunaGetBufferSize(indexBuffer),
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
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
            .pTexelBufferView = &lightmapOneBufferView,
        },
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = 2,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
            .pTexelBufferView = &lightmapTwoBufferView,
        },
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = 3,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo = &luxelPositionsImageInfo,
        },
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = 4,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo = &luxelNormalsImageInfo,
        },
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = 5,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo = &luxelAlbedosImageInfo,
        },
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = 6,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &lightsBufferInfo,
        },
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = 7,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &vertexBufferInfo,
        },
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = 8,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &indexBufferInfo,
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
    const LunaBufferCreationInfo shaderBindingTableCreationInfo = {
        .size = physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
        .alignment = physicalDeviceRayTracingPipelineProperties.shaderGroupBaseAlignment,
        .usage = USAGE_FLAGS,
        .queueFamilyIndexCount = 1,
        .queueFamilyIndices = &queueFamilyIndex,
        .allocationCreateInfo = &VRAM_ALLOCATION_CREATE_INFO,
    };
    if (!CheckResult(lunaCreateBuffer(device,
                                      &shaderBindingTableCreationInfo,
                                      &directLightingRaygenShaderBindingTable)))
    {
        return false;
    }
    if (!CheckResult(lunaCreateBuffer(device, &shaderBindingTableCreationInfo, &missShaderBindingTable)))
    {
        return false;
    }
    if (!CheckResult(lunaCreateBuffer(device,
                                      &shaderBindingTableCreationInfo,
                                      &globalIlluminationRaygenShaderBindingTable)))
    {
        return false;
    }
    if (!CheckResult(lunaCreateBuffer(device, &shaderBindingTableCreationInfo, &closestHitShaderBindingTable)))
    {
        return false;
    }

    std::vector<char> shaderHandles(2 * physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize);
    if (!CheckResult(vkGetRayTracingShaderGroupHandlesKHR(lunaGetVkDevice(device),
                                                          directLightingPipeline,
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
        .stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
    };
    if (!CheckResult(lunaWriteDataToBuffer(device,
                                           commandBuffer,
                                           directLightingRaygenShaderBindingTable,
                                           &raygenShaderBindingTableWriteInfo)))
    {
        return false;
    }

    const LunaBufferWriteInfo secondShaderBindingTableWriteInfo = {
        .bytes = physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
        .data = shaderHandles.data() + physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
        .stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
    };
    if (!CheckResult(lunaWriteDataToBuffer(device,
                                           commandBuffer,
                                           missShaderBindingTable,
                                           &secondShaderBindingTableWriteInfo)))
    {
        return false;
    }


    if (!CheckResult(vkGetRayTracingShaderGroupHandlesKHR(lunaGetVkDevice(device),
                                                          globalIlluminationPipeline,
                                                          0,
                                                          2,
                                                          shaderHandles.size(),
                                                          shaderHandles.data())))
    {
        return false;
    }

    if (!CheckResult(lunaWriteDataToBuffer(device,
                                           commandBuffer,
                                           globalIlluminationRaygenShaderBindingTable,
                                           &raygenShaderBindingTableWriteInfo)))
    {
        return false;
    }

    if (!CheckResult(lunaWriteDataToBuffer(device,
                                           commandBuffer,
                                           closestHitShaderBindingTable,
                                           &secondShaderBindingTableWriteInfo)))
    {
        return false;
    }


    return true;
}

bool LightBakerGpu::SingleBakeIteration(const uint64_t width,
                                        const uint64_t height,
                                        const float percentDone,
                                        const uint32_t baseLuxelIndex,
                                        const bool directLighting) const
{
    Logger::Info("Baking lighting {}%...", percentDone);

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
                       sizeof(baseLuxelIndex),
                       &baseLuxelIndex);
    if (directLighting)
    {
        vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, directLightingPipeline);

        static constexpr VkStridedDeviceAddressRegionKHR STRIDED_DEVICE_ADDRESS_REGION_NONE{};
        const VkStridedDeviceAddressRegionKHR raygenShaderBindingTableAddressRegion = {
            .deviceAddress = lunaGetBufferDeviceAddress(device, directLightingRaygenShaderBindingTable),
            .stride = physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
            .size = physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
        };
        const VkStridedDeviceAddressRegionKHR missShaderBindingTableAddressRegion = {
            .deviceAddress = lunaGetBufferDeviceAddress(device, missShaderBindingTable),
            .stride = physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
            .size = physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
        };
        vkCmdTraceRaysKHR(vkCommandBuffer,
                          &raygenShaderBindingTableAddressRegion,
                          &missShaderBindingTableAddressRegion,
                          &STRIDED_DEVICE_ADDRESS_REGION_NONE,
                          &STRIDED_DEVICE_ADDRESS_REGION_NONE,
                          width,
                          height,
                          1);
    } else
    {
        vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, globalIlluminationPipeline);

        static constexpr VkStridedDeviceAddressRegionKHR STRIDED_DEVICE_ADDRESS_REGION_NONE{};
        const VkStridedDeviceAddressRegionKHR raygenShaderBindingTableAddressRegion = {
            .deviceAddress = lunaGetBufferDeviceAddress(device, globalIlluminationRaygenShaderBindingTable),
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
    }
    const LunaCommandBufferSubmitInfo submitInfo = {
        .queue = queue,
        .waitSemaphoreCount = 1,
        .waitSemaphores = &semaphore,
        .waitDstStageMasks = &WAIT_STAGE,
        .signalSemaphoreCount = 1,
        .signalSemaphores = &semaphore,
    };
    return CheckResult(lunaEndAndSubmitCommandBuffer(device, commandBuffer, &submitInfo));
}
