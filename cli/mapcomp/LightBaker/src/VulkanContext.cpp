//
// Created by NBT22 on 11/16/26.
//

#include "VulkanContext.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <glslang/Public/ShaderLang.h>
#include <libassets/asset/LevelMaterialAsset.h>
#include <libassets/asset/TextureAsset.h>
#include <libassets/util/Error.h>
#include <libassets/util/Logger.h>
#include <libassets/util/SearchPathManager.h>
#include <libassets/util/ShaderCompiler.h>
#include <luna/lunaCommandBuffer.h>
#include <luna/lunaDevice.h>
#include <luna/lunaImage.h>
#include <luna/lunaInstance.h>
#include <luna/lunaSynchronization.h>
#include <luna/lunaTypes.h>
#include <string>
#include <vector>
#include <volk.h>
#include <vulkan/vulkan_core.h>

static constexpr VkPipelineStageFlagBits2 WAIT_STAGE = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT |
                                                       VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;

VulkanContext::VulkanContext(SearchPathManager pathManager, const bool enableNsightCapture):
    pathManager(std::move(pathManager))
{
#ifdef INVALID_PRESENT_FOR_CAPTURING
    static constexpr std::array INSTANCE_EXTENSIONS = {VK_KHR_SURFACE_EXTENSION_NAME};
#else
    static constexpr std::array<const char *const, 0> INSTANCE_EXTENSIONS{};
#endif

    static constexpr LunaInstanceCreationInfo INSTANCE_CREATION_INFO = {
        .apiVersion = VK_API_VERSION_1_2,
        .extensionCount = INSTANCE_EXTENSIONS.size(),
        .extensionNames = INSTANCE_EXTENSIONS.data(),
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
        .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
        .descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE,
        .runtimeDescriptorArray = VK_TRUE,
        .scalarBlockLayout = VK_TRUE,
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
#ifdef INVALID_PRESENT_FOR_CAPTURING
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#endif
    };
    static constexpr std::array OPTIONAL_EXTENSIONS = {VK_EXT_FRAME_BOUNDARY_EXTENSION_NAME};
    static VkPhysicalDeviceFrameBoundaryFeaturesEXT frameBoundaryFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAME_BOUNDARY_FEATURES_EXT,
        .frameBoundary = VK_TRUE,
    };
    static constexpr VkPhysicalDeviceFeatures2 OPTIONAL_FEATURES = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &frameBoundaryFeatures,
    };
    const LunaDeviceCreationInfo deviceCreationInfo = {
        .requiredExtensionCount = REQUIRED_EXTENSIONS.size(),
        .requiredExtensionNames = REQUIRED_EXTENSIONS.data(),
        .optionalExtensionCount = OPTIONAL_EXTENSIONS.size(),
        .optionalExtensionNames = OPTIONAL_EXTENSIONS.data(),
        .requiredFeatures = REQUIRED_FEATURES,
        .optionalFeatures = &OPTIONAL_FEATURES,
        .physicalDevicePreferenceDefinition = &PHYSICAL_DEVICE_PREFERENCE_DEFINITION,
    };
    if (!CheckResult(lunaCreateDevice(&deviceCreationInfo, &device)))
    {
        return;
    }

    if (enableNsightCapture)
    {
        VkPhysicalDeviceFrameBoundaryFeaturesEXT enabledFrameBoundaryFeatures = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAME_BOUNDARY_FEATURES_EXT,
            .frameBoundary = VK_TRUE,
        };
        VkPhysicalDeviceFeatures2 enabledFeatures = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = &enabledFrameBoundaryFeatures,
        };
        frameBoundaryEnabled = lunaDeviceIsExtensionEnabled(device, VK_EXT_FRAME_BOUNDARY_EXTENSION_NAME) &&
                               lunaGetEnabledDeviceFeatures(device, &enabledFeatures);
        if (!frameBoundaryEnabled)
        {
            Logger::Warning("VK_EXT_frame_boundary is not available, so no Nsight Graphics capture will be made");
        }
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

    const VkDevice vkDevice = lunaGetVkDevice(device);

    static constexpr uint32_t MAX_TEXTURES = 512;
    static constexpr VkDescriptorPoolSize POOL_SIZE = {
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = MAX_TEXTURES,
    };
    static constexpr VkDescriptorPoolCreateInfo POOL_CREATE_INFO = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = &POOL_SIZE,
    };
    if (!CheckResult(vkCreateDescriptorPool(vkDevice, &POOL_CREATE_INFO, nullptr, &texturesDescriptorPool)))
    {
        return;
    }

    static constexpr VkDescriptorSetLayoutBinding DESCRIPTOR_BINDING = {
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = MAX_TEXTURES,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR,
    };
    static constexpr VkDescriptorSetLayoutCreateInfo LAYOUT_CREATE_INFO = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &DESCRIPTOR_BINDING,
    };
    if (!CheckResult(vkCreateDescriptorSetLayout(vkDevice, &LAYOUT_CREATE_INFO, nullptr, &texturesDescriptorSetLayout)))
    {
        return;
    }
    const VkDescriptorSetAllocateInfo allocationInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = texturesDescriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &texturesDescriptorSetLayout,
    };
    if (!CheckResult(vkAllocateDescriptorSets(vkDevice, &allocationInfo, &texturesDescriptorSet)))
    {
        return;
    }

    submitInfo = {
        .queue = queue,
        .waitSemaphoreCount = 1,
        .waitSemaphores = &semaphore,
        .waitDstStageMasks = &WAIT_STAGE,
        .signalSemaphoreCount = 1,
        .signalSemaphores = &semaphore,
    };

    lunaGetPhysicalDeviceProperties2(device, &physicalDeviceProperties);
    Logger::Info("Vulkan Initialized");
    Logger::Info("Vulkan Device: {}", physicalDeviceProperties.properties.deviceName);
    Logger::Info("Vulkan Version: {}.{}.{}",
                 VK_API_VERSION_MAJOR(physicalDeviceProperties.properties.apiVersion),
                 VK_API_VERSION_MINOR(physicalDeviceProperties.properties.apiVersion),
                 VK_API_VERSION_PATCH(physicalDeviceProperties.properties.apiVersion));

    valid = true;
}

VulkanContext::~VulkanContext()
{
    valid = false;
    lunaDeviceWaitIdle(device);

    vkDestroyDescriptorSetLayout(lunaGetVkDevice(device), texturesDescriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(lunaGetVkDevice(device), texturesDescriptorPool, nullptr);

    lunaDestroyInstance();
}

bool VulkanContext::GetTextureIndex(const std::string &texture, uint32_t &index)
{
    for (index = 0; index < loadedTextures.size(); index++)
    {
        if (loadedTextures.at(index).first == texture)
        {
            return true;
        }
    }

    const std::string texturePath = pathManager.GetAssetPath(texture);
    TextureAsset image{};
    const Error error = image.LoadFromAsset(texturePath);
    if (error.Failed())
    {
        Logger::Error("Creating texture asset \"{}\" failed with error: {}", texturePath, error);
    }
    const VkSamplerAddressMode samplerAddressMode = image.repeat ? VK_SAMPLER_ADDRESS_MODE_REPEAT
                                                                 : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    const VkFilter filter = image.filter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    const LunaSamplerCreationInfo samplerCreationInfo = {
        .magFilter = filter,
        .minFilter = filter,
        .addressModeU = samplerAddressMode,
        .addressModeV = samplerAddressMode,
        .addressModeW = samplerAddressMode,
        .maxLod = VK_LOD_CLAMP_NONE,
    };
    const VkFormat format = image.GetFormat() == TextureAsset::PixelFormat::RGBAF16 ? VK_FORMAT_R16G16B16A16_SFLOAT
                                                                                    : VK_FORMAT_R8G8B8A8_UNORM;
    const size_t bytesPerTexelChannel = image.GetFormat() == TextureAsset::PixelFormat::RGBAF16 ? sizeof(_Float16)
                                                                                                : sizeof(uint8_t);
    const LunaImageWriteInfo writeInfo = {
        .bytes = image.GetWidth() * image.GetHeight() * 4 * bytesPerTexelChannel,
        .pixels = image.GetPixelsRGBA(),
        .sourceStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        .destinationStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .destinationAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .submitInfo = &submitInfo,
    };
    const LunaImageCreationInfo creationInfo = {
        .format = format,
        .width = image.GetWidth(),
        .height = image.GetHeight(),
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT,
        .queueFamilyIndexCount = 1,
        .queueFamilyIndices = &queueFamilyIndex,
        .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .writeInfo = writeInfo,
        .samplerCreationInfo = &samplerCreationInfo,
    };
    LunaImage lunaImage{};
    if (!CheckResult(lunaCreateImage(device, commandBuffer, &creationInfo, &lunaImage)))
    {
        return false;
    }

    const VkDescriptorImageInfo imageInfo = {
        .sampler = lunaGetVkSampler(lunaImage),
        .imageView = lunaGetVkImageView(lunaImage),
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    const VkWriteDescriptorSet writeDescriptor = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = texturesDescriptorSet,
        .dstArrayElement = index,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &imageInfo,
    };
    vkUpdateDescriptorSets(lunaGetVkDevice(device), 1, &writeDescriptor, 0, nullptr);

    loadedTextures.emplace_back(texture, lunaImage);
    return true;
}

bool VulkanContext::GetMaterialTextureIndex(const std::string &materialName, uint32_t &index)
{
    const std::string materialPath = pathManager.GetAssetPath(materialName);
    LevelMaterialAsset material{};
    const Error error = material.LoadFromAsset(materialPath);
    if (error.Failed())
    {
        Logger::Error("Creating material asset \"{}\" failed with error: {}", materialPath, error);
    }

    return GetTextureIndex(material.texture, index);
}

VkShaderModule VulkanContext::GenerateShaderModule(const std::filesystem::path &path,
                                                   const EShLanguage shaderType) const
{
    std::vector<uint32_t> spirv{};
    ShaderCompiler shaderCompiler(path, shaderType, true, SHADER_DEBUG_INFO);
    if (shaderCompiler.Compile(spirv).Failed())
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

bool VulkanContext::SubmitFrameBoundary()
{
    if (!frameBoundaryEnabled)
    {
        return false;
    }
    const VkFrameBoundaryEXT frameBoundary = {
        .sType = VK_STRUCTURE_TYPE_FRAME_BOUNDARY_EXT,
        .flags = VK_FRAME_BOUNDARY_FRAME_END_BIT_EXT,
        .frameID = currentFrameId++,
    };
    const VkSubmitInfo frameBoundarySubmitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = &frameBoundary,
    };
    return CheckResult(vkQueueSubmit(queue, 1, &frameBoundarySubmitInfo, VK_NULL_HANDLE));
}
