//
// Created by NBT22 on 3/20/26.
//

#include "LightBaker.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <glm/glm.hpp>
#include <glslang/Public/ShaderLang.h>
#include <libassets/asset/ModelAsset.h>
#include <libassets/type/ActorDefinition.h>
#include <libassets/type/MapVertex.h>
#include <libassets/type/ModelLod.h>
#include <libassets/type/ModelVertex.h>
#include <libassets/type/renderDefs/ModelRenderDefinition.h>
#include <libassets/type/renderDefs/RenderDefinition.h>
#include <libassets/util/ActorDefinitionManager.h>
#include <libassets/util/Error.h>
#include <libassets/util/Logger.h>
#include <libassets/util/SearchPathManager.h>
#include <libassets/util/ShaderCompiler.h>
#include <luna/luna.h>
#include <luna/lunaBuffer.h>
#include <luna/lunaCommandBuffer.h>
#include <luna/lunaDevice.h>
#include <luna/lunaImage.h>
#include <luna/lunaTypes.h>
#include <memory>
#include <utility>
#include <vector>
#include <volk.h>
#include <vulkan/vulkan_core.h>
#include "BrushMeshBuilder.h"
#include "Light.h"
#include "NsightManager.h"
#include "RenderdocManager.h"
#include "VulkanContext.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

#include "ShaderLight.hpp"

namespace
{
constexpr uint32_t MAX_DISPATCH_DIMENSION = 1u << 8u;

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

            auto addEntry = [&index, this]<typename Type> {
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
template<Concepts::VkDescriptorSetLayoutBindingPair... T> class DescriptorSetLayoutBindings<T...>
    : public std::array<VkDescriptorSetLayoutBinding, sizeof...(T)>
{
    public:
        explicit constexpr DescriptorSetLayoutBindings(const T &...bindings)
        {
            size_t index = 0;

            auto addEntry = [&index, this]<typename Type>(const Type &binding) {
                this->at(index) = VkDescriptorSetLayoutBinding{
                    .binding = static_cast<uint32_t>(index),
                    .descriptorType = binding.first,
                    .descriptorCount = 1,
                    .stageFlags = static_cast<VkShaderStageFlags>(binding.second),
                };
                index++;
            };
            (addEntry(bindings), ...);
        }
};
} // namespace

LightBaker::LightBaker(VulkanContext &context,
                       const ActorDefinitionManager &definitionManager,
                       const std::vector<BrushMeshBuilder> &meshBuilders,
                       const std::vector<Actor> &actors,
                       const std::vector<Light> &lights,
                       const glm::uvec2 lightmapSize,
                       const uint32_t bounceCount,
                       const uint32_t sampleCount):
    context(context),
    lightCount(lights.size()),
    lightmapSize(lightmapSize),
    bounceCount(bounceCount),
    sampleCount(sampleCount),
    tlas(context),
    lightmapOne(context, lightmapSize),
    lightmapTwo(context, lightmapSize),
    lightmapThree(context, lightmapSize)
{
    if (meshBuilders.empty())
    {
        valid = false;
        return;
    }
    if (lights.size() > 16384)
    {
        Logger::Error("Maximum number of lights allowed in a level is 16384!");
        valid = false;
        return;
    }

    if (enableRenderdocCapture)
    {
        enableRenderdocCapture = RenderdocManager::InitRenderdoc();
    }
    if (enableNsightCapture)
    {
        enableNsightCapture = NsightManager::InitNsight();
    }

    std::vector<ShaderLight> shaderLights;
    shaderLights.reserve(lights.size());
    for (const Light &light: lights)
    {
        shaderLights.emplace_back(context, light);
    }

    const VkDeviceSize lightsBufferSize = shaderLights.size() * sizeof(ShaderLight);
    const LunaBufferCreationInfo lightsBufferCreationInfo = {
        .size = lightsBufferSize,
        .alignment = context.physicalDeviceProperties.properties.limits.minStorageBufferOffsetAlignment,
        .memoryType = LUNA_MEMORY_TYPE_DEVICE_LOCAL,
    };
    if (!CheckResult(lunaCreateBuffer(context.device, &lightsBufferCreationInfo, &lightsBuffer)))
    {
        valid = false;
        return;
    }

    const LunaBufferWriteInfo lightsBufferWriteInfo = {
        .size = lightsBufferSize,
        .data = shaderLights.data(),
        .sourceStageMask = VK_PIPELINE_STAGE_2_NONE,
        .sourceAccessMask = VK_ACCESS_2_NONE,
        .destinationStageMask = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
        .destinationAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
    };
    if (!CheckResult(lunaWriteDataToBuffer(context.device,
                                           context.commandBuffer,
                                           lightsBuffer,
                                           &lightsBufferWriteInfo)))
    {
        valid = false;
        return;
    }

    if (!CreateVertexAndIndexBuffers(meshBuilders))
    {
        valid = false;
        return;
    }

    if (!PrecomputeLuxelInformation())
    {
        valid = false;
        return;
    }

    if (!CacheEmissiveLuxelIndices())
    {
        valid = false;
        return;
    }

    Logger::Verbose("{} Emissive luxels in lightmap",
                    *static_cast<const uint32_t *>(lunaGetBufferDataPointer(emissiveLuxelIndicesBuffer)));

    if (!CreateBLAS(definitionManager, meshBuilders, actors))
    {
        valid = false;
        return;
    }
    if (!CreateTLAS())
    {
        valid = false;
        return;
    }
    if (!CreateAndWriteDescriptorSet())
    {
        valid = false;
        return;
    }
    if (!CreateGlobalIlluminationPipeline())
    {
        valid = false;
        return;
    }

    valid = true;
}

// TODO return an Error from this now that they can contain more details
bool LightBaker::Bake(std::vector<uint16_t> &pixelData, std::vector<uint16_t> &indirectLightingPixelData)
{
    if (!IsValid())
    {
        return false;
    }

    if (enableRenderdocCapture)
    {
        RenderdocManager::StartCapture("bake");
    }
    if (context.frameBoundaryEnabled)
    {
        NsightManager::StartCapture();
        if (!context.SubmitFrameBoundary())
        {
            return false;
        }
    }
    const uint64_t luxelCount = lightmapSize.x * lightmapSize.y;
    const uint32_t width = std::min(luxelCount, uint64_t{MAX_DISPATCH_DIMENSION});
    const uint32_t height = std::min(std::max(luxelCount / width, uint64_t{1}), uint64_t{MAX_DISPATCH_DIMENSION});
    const uint32_t iterations = std::max(luxelCount / width / height, uint64_t{1});
    const uint32_t perLuxelBounceIterations = sampleCount / SAMPLES_PER_THREAD;
    const float totalBounceDispatches = static_cast<float>(iterations * perLuxelBounceIterations * bounceCount);
    const float totalDispatches = static_cast<float>(2 * iterations) + totalBounceDispatches;


    if (!CreateDirectLightingPipeline(VK_FALSE))
    {
        return false;
    }

    uint32_t nextPercentLog = 10;
    for (uint32_t iteration = 0; iteration < iterations; iteration++)
    {
        const float percentDone = static_cast<float>(100 * iteration) / totalDispatches;
        if (percentDone > static_cast<float>(nextPercentLog))
        {
            Logger::Info("Baking lighting {}%...", nextPercentLog);
            nextPercentLog = (static_cast<uint32_t>(percentDone) / 10 + 1) * 10;
        }
        if (!SingleBakeIteration(width, height, width * height * iteration, 0, true))
        {
            return false;
        }
    }
    for (uint32_t bounce = 0; bounce < bounceCount; bounce++)
    {
        lunaDeviceWaitIdle(context.device);
        lightmapThree.WriteToDescriptorSet(descriptorSet, bounce % 2 + 2);
        lightmapTwo.WriteToDescriptorSet(descriptorSet, (bounce + 1) % 2 + 2);

        for (uint32_t iteration = 0; iteration < iterations; iteration++)
        {
            lunaDeviceWaitIdle(context.device);
            for (uint32_t perLuxelIteration = 0; perLuxelIteration < perLuxelBounceIterations; perLuxelIteration++)
            {
                const float percentDone = static_cast<float>(100 * (iterations +
                                                                    bounce * iterations * perLuxelBounceIterations +
                                                                    iteration * perLuxelBounceIterations +
                                                                    perLuxelIteration)) /
                                          totalDispatches;
                if (percentDone > static_cast<float>(nextPercentLog))
                {
                    Logger::Info("Baking lighting {}%...", nextPercentLog);
                    nextPercentLog = (static_cast<uint32_t>(percentDone) / 10 + 1) * 10;
                }
                if (!SingleBakeIteration(width,
                                         height,
                                         width * height * iteration,
                                         perLuxelIteration * SAMPLES_PER_THREAD,
                                         false))
                {
                    return false;
                }
            }
        }
    }

    lightmapOne.AddPadding(indirectLightingPixelData);

    if (!CreateDirectLightingPipeline(VK_TRUE))
    {
        return false;
    }
    for (uint32_t iteration = 0; iteration < iterations; iteration++)
    {
        const float percentDone = 100 *
                                  (static_cast<float>(iterations + iteration) + totalBounceDispatches) /
                                  totalDispatches;
        if (percentDone > static_cast<float>(nextPercentLog))
        {
            Logger::Info("Baking lighting {}%...", nextPercentLog);
            nextPercentLog = (static_cast<uint32_t>(percentDone) / 10 + 1) * 10;
        }
        if (!SingleBakeIteration(width, height, width * height * iteration, 0, true))
        {
            return false;
        }
    }

    if (context.frameBoundaryEnabled)
    {
        if (!context.SubmitFrameBoundary())
        {
            return false;
        }
        NsightManager::EndCapture();
    }
    if (enableRenderdocCapture)
    {
        RenderdocManager::EndCapture();
    }

    lightmapOne.AddPadding(pixelData);

#ifdef INVALID_PRESENT_FOR_CAPTURING
    static constexpr VkPresentInfoKHR PRESENT_INFO = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    };
    vkQueuePresentKHR(queue, &PRESENT_INFO);
#endif

    return true;
}

bool LightBaker::IsValid() const
{
    return valid;
}

bool LightBaker::CreateVertexAndIndexBuffers(const std::vector<BrushMeshBuilder> &meshBuilders)
{
    size_t indexOffset = 0;
    std::vector<MapVertex> vertices{};
    std::vector<uint32_t> indices{};
    for (const BrushMeshBuilder &builder: meshBuilders)
    {
        vertices.insert(vertices.end(), builder.GetVertices().begin(), builder.GetVertices().end());
        for (const uint32_t index: builder.GetIndices())
        {
            indices.emplace_back(index + indexOffset);
        }
        indexOffset += builder.GetVertices().size();
    }
    brushesVertexCount = vertices.size();
    brushesIndexCount = indices.size();

    const size_t vertexBufferByteCount = vertices.size() * sizeof(MapVertex);
    const size_t indexBufferByteCount = indices.size() * sizeof(uint32_t);

    const LunaBufferCreationInfo vertexBufferCreationInfo = {
        .size = vertexBufferByteCount,
        .memoryType = LUNA_MEMORY_TYPE_DEVICE_LOCAL,
        .additionalUsageFlags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    };
    if (!CheckResult(lunaCreateBuffer(context.device, &vertexBufferCreationInfo, &brushesVertexBuffer)))
    {
        return false;
    }
    const LunaBufferCreationInfo indexBufferCreationInfo = {
        .size = indexBufferByteCount,
        .memoryType = LUNA_MEMORY_TYPE_DEVICE_LOCAL,
        .additionalUsageFlags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    };
    if (!CheckResult(lunaCreateBuffer(context.device, &indexBufferCreationInfo, &brushesIndexBuffer)))
    {
        return false;
    }

    const LunaBufferWriteInfo vertexBufferWriteInfo = {
        .size = vertexBufferByteCount,
        .data = vertices.data(),
        .sourceStageMask = VK_PIPELINE_STAGE_2_NONE,
        .sourceAccessMask = VK_ACCESS_2_NONE,
        .destinationStageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR |
                                VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
        .destinationAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR |
                                 VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR |
                                 VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT,
    };
    if (!CheckResult(lunaWriteDataToBuffer(context.device,
                                           context.commandBuffer,
                                           brushesVertexBuffer,
                                           &vertexBufferWriteInfo)))
    {
        return false;
    }
    const LunaBufferWriteInfo indexBufferWriteInfo = {
        .size = indexBufferByteCount,
        .data = indices.data(),
        .sourceStageMask = VK_PIPELINE_STAGE_2_NONE,
        .sourceAccessMask = VK_ACCESS_2_NONE,
        .destinationStageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR |
                                VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
        .destinationAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR |
                                 VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR |
                                 VK_ACCESS_2_INDEX_READ_BIT,
    };
    if (!CheckResult(lunaWriteDataToBuffer(context.device,
                                           context.commandBuffer,
                                           brushesIndexBuffer,
                                           &indexBufferWriteInfo)))
    {
        return false;
    }

    return CheckResult(lunaDeviceWaitIdle(context.device));
}

bool LightBaker::PrecomputeLuxelInformation()
{
    static constexpr size_t ATTACHMENT_COUNT = 3;

    const VkDevice vkDevice = lunaGetVkDevice(context.device);
    const VkCommandBuffer vkCommandBuffer = lunaGetVkCommandBuffer(context.commandBuffer);

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
        .format = VK_FORMAT_R16G16B16A16_SFLOAT,
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
    static constexpr VkRenderPassCreateInfo RENDER_PASS_CREATE_INFO = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = ATTACHMENT_DESCRIPTIONS.size(),
        .pAttachments = ATTACHMENT_DESCRIPTIONS.data(),
        .subpassCount = 1,
        .pSubpasses = &SUBPASS,
        .dependencyCount = 1,
        .pDependencies = &DEPENDENCY,
    };
    VkRenderPass renderPass{};
    if (!CheckResult(vkCreateRenderPass(vkDevice, &RENDER_PASS_CREATE_INFO, nullptr, &renderPass)))
    {
        return false;
    }

    const LunaImageWriteInfo writeInfo = {
        .destinationStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
                                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .destinationAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .submitInfo = &context.submitInfo,
    };
    static constexpr LunaSamplerCreationInfo SAMPLER_CREATION_INFO{};
    const LunaImageCreationInfo luxelInformationImageCreateInfo = {
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .width = lightmapSize.x,
        .height = lightmapSize.y,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        .queueFamilyIndexCount = 1,
        .queueFamilyIndices = &context.queueFamilyIndex,
        .layout = VK_IMAGE_LAYOUT_GENERAL,
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .writeInfo = writeInfo,
        .samplerCreationInfo = &SAMPLER_CREATION_INFO,
    };
    if (!CheckResult(lunaCreateImage(context.device,
                                     context.commandBuffer,
                                     &luxelInformationImageCreateInfo,
                                     &luxelPositionsImage)))
    {
        return false;
    }
    if (!CheckResult(lunaCreateImage(context.device,
                                     context.commandBuffer,
                                     &luxelInformationImageCreateInfo,
                                     &luxelNormalsImage)))
    {
        return false;
    }
    const LunaImageCreationInfo luxelAlbedosImageCreateInfo = {
        .format = VK_FORMAT_R16G16B16A16_SFLOAT,
        .width = lightmapSize.x,
        .height = lightmapSize.y,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        .queueFamilyIndexCount = 1,
        .queueFamilyIndices = &context.queueFamilyIndex,
        .layout = VK_IMAGE_LAYOUT_GENERAL,
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .writeInfo = writeInfo,
        .samplerCreationInfo = &SAMPLER_CREATION_INFO,
    };
    if (!CheckResult(lunaCreateImage(context.device,
                                     context.commandBuffer,
                                     &luxelAlbedosImageCreateInfo,
                                     &luxelAlbedosImage)))
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

    const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &context.texturesDescriptorSetLayout,
    };
    VkPipelineLayout graphicsPipelineLayout{};
    if (!CheckResult(vkCreatePipelineLayout(vkDevice, &pipelineLayoutCreateInfo, nullptr, &graphicsPipelineLayout)))
    {
        return false;
    }

    const VkShaderModule vertexShaderModule = context.GenerateShaderModule("assets/shaders/lightmap/"
                                                                           "precompute_luxel_information.vert",
                                                                           EShLangVertex);
    if (vertexShaderModule == VK_NULL_HANDLE)
    {
        return false;
    }
    const VkShaderModule fragmentShaderModule = context.GenerateShaderModule("assets/shaders/lightmap/"
                                                                             "precompute_luxel_information.frag",
                                                                             EShLangFragment);
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
            .location = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(MapVertex, position),
        },
        VkVertexInputAttributeDescription{
            .location = 1,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(MapVertex, uv),
        },
        VkVertexInputAttributeDescription{
            .location = 2,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(MapVertex, lightmapUv),
        },
        VkVertexInputAttributeDescription{
            .location = 3,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(MapVertex, normal),
        },
        VkVertexInputAttributeDescription{
            .location = 4,
            .format = VK_FORMAT_R32_UINT,
            .offset = offsetof(MapVertex, textureIndex),
        },
        VkVertexInputAttributeDescription{
            .location = 5,
            .format = VK_FORMAT_R32_SFLOAT,
            .offset = offsetof(MapVertex, emissive),
        },
    };
    static constexpr VkPipelineVertexInputStateCreateInfo VERTEX_INPUT_STATE_CREATE_INFO = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &VERTEX_INPUT_BINDING_DESCRIPTION,
        .vertexAttributeDescriptionCount = VERTEX_INPUT_ATTRIBUTE_DESCRIPTIONS.size(),
        .pVertexAttributeDescriptions = VERTEX_INPUT_ATTRIBUTE_DESCRIPTIONS.data(),
    };
    static constexpr VkPipelineInputAssemblyStateCreateInfo INPUT_ASSEMBLY_STATE_CREATE_INFO = {
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
    static constexpr VkPipelineRasterizationStateCreateInfo RASTERIZATION_STATE_CREATE_INFO = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1,
    };
    static constexpr VkPipelineMultisampleStateCreateInfo MULTISAMPLE_STATE_CREATE_INFO = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };
    static constexpr VkPipelineDepthStencilStateCreateInfo DEPTH_STENCIL_STATE_CREATE_INFO = {
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
    static constexpr VkPipelineColorBlendStateCreateInfo COLOR_BLEND_STATE_CREATE_INFO = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = COLOR_BLEND_ATTACHMENTS.size(),
        .pAttachments = COLOR_BLEND_ATTACHMENTS.data(),
    };
    const VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = shaderStages.size(),
        .pStages = shaderStages.data(),
        .pVertexInputState = &VERTEX_INPUT_STATE_CREATE_INFO,
        .pInputAssemblyState = &INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &RASTERIZATION_STATE_CREATE_INFO,
        .pMultisampleState = &MULTISAMPLE_STATE_CREATE_INFO,
        .pDepthStencilState = &DEPTH_STENCIL_STATE_CREATE_INFO,
        .pColorBlendState = &COLOR_BLEND_STATE_CREATE_INFO,
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

    if (!CheckResult(lunaBeginSingleUseCommandBuffer(context.device, context.commandBuffer)))
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

    const VkBuffer vkVertexBuffer = lunaGetVkBuffer(brushesVertexBuffer);
    const VkDeviceSize vertexBufferOffset = lunaGetBufferOffset(brushesVertexBuffer);
    vkCmdBindVertexBuffers(vkCommandBuffer, 0, 1, &vkVertexBuffer, &vertexBufferOffset);
    vkCmdBindIndexBuffer(vkCommandBuffer,
                         lunaGetVkBuffer(brushesIndexBuffer),
                         lunaGetBufferOffset(brushesIndexBuffer),
                         VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(vkCommandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            graphicsPipelineLayout,
                            0,
                            1,
                            &context.texturesDescriptorSet,
                            0,
                            nullptr);
    vkCmdDrawIndexed(vkCommandBuffer, brushesIndexCount, 1, 0, 0, 0);

    vkCmdEndRenderPass(vkCommandBuffer);

    return CheckResult(lunaEndAndSubmitCommandBuffer(context.device, context.commandBuffer, &context.submitInfo));
}

// TODO: This function is a bit of a mess of spaghetti
bool LightBaker::CacheEmissiveLuxelIndices()
{
    std::vector<uint32_t> spirv{};
    static constexpr const char *PATH_STRING = "assets/shaders/lightmap/cache_emissive_luxel_indices.comp";
    ShaderCompiler shaderCompiler(PATH_STRING, EShLangCompute, true, VulkanContext::SHADER_DEBUG_INFO);
    if (shaderCompiler.Compile(spirv).Failed())
    {
        Logger::Error("Error compiling shader {}!", PATH_STRING);
        Logger::Info("{}", shaderCompiler.GetErrorMessage());
        return false;
    }
    const LunaSpirvShaderModuleCreationInfo spirvShaderModuleCreationInfo = {
        .size = spirv.size() * sizeof(uint32_t),
        .spirv = spirv.data(),
    };
    const LunaShaderModuleCreationInfo shaderModuleCreationInfo = {
        .creationInfoType = LUNA_SHADER_MODULE_CREATION_INFO_TYPE_SPIRV,
        .creationInfoUnion = LunaShaderModuleCreationInfoTypeUnion{.spirv = spirvShaderModuleCreationInfo},
    };
    LunaShaderModule shaderModule{};
    if (!CheckResult(lunaCreateShaderModule(context.device, &shaderModuleCreationInfo, &shaderModule)))
    {
        return false;
    }
    static constexpr SpecializationMapEntries<uint32_t> SPECIALIZATION_MAP_ENTRIES{};
    const VkSpecializationInfo specializationInfo = {
        .mapEntryCount = SPECIALIZATION_MAP_ENTRIES.size(),
        .pMapEntries = SPECIALIZATION_MAP_ENTRIES.data(),
        .dataSize = sizeof(lightmapSize.x),
        .pData = &lightmapSize.x,
    };
    const LunaPipelineShaderStageCreationInfo shaderStageCreationInfo = {
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = shaderModule,
        .specializationInfo = &specializationInfo,
    };
    static constexpr std::array DESCRIPTOR_SET_LAYOUT_BINDINGS = {
        LunaDescriptorSetLayoutBinding{
            .bindingName = "Luxel Normals",
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
        LunaDescriptorSetLayoutBinding{
            .bindingName = "Emissive Luxel Indices",
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
    };
    static constexpr LunaDescriptorSetLayoutCreationInfo DESCRIPTOR_SET_LAYOUT_CREATION_INFO = {
        .bindingCount = DESCRIPTOR_SET_LAYOUT_BINDINGS.size(),
        .bindings = DESCRIPTOR_SET_LAYOUT_BINDINGS.data(),
    };
    LunaDescriptorSetLayout computePipelineDescriptorSetLayout{};
    if (!CheckResult(lunaCreateDescriptorSetLayout(context.device,
                                                   &DESCRIPTOR_SET_LAYOUT_CREATION_INFO,
                                                   &computePipelineDescriptorSetLayout)))
    {
        return false;
    }
    const LunaPipelineLayoutCreationInfo pipelineLayoutCreationInfo = {
        .descriptorSetLayoutCount = 1,
        .descriptorSetLayouts = &computePipelineDescriptorSetLayout,
    };
    const LunaComputePipelineCreationInfo computePipelineCreationInfo = {
        .shaderStageCreationInfo = shaderStageCreationInfo,
        .layoutCreationInfo = pipelineLayoutCreationInfo,
    };
    LunaComputePipeline computePipeline{};
    if (!CheckResult(lunaCreateComputePipeline(context.device, &computePipelineCreationInfo, &computePipeline)))
    {
        return false;
    }

    const LunaBufferCreationInfo emissiveLuxelIndicesBufferCreationInfo = {
        .size = sizeof(uint32_t) + lightmapSize.x * lightmapSize.y * sizeof(int),
        .alignment = context.physicalDeviceProperties.properties.limits.minStorageBufferOffsetAlignment,
        .memoryType = LUNA_MEMORY_TYPE_DEVICE_LOCAL,
    };
    if (!CheckResult(lunaCreateBuffer(context.device,
                                      &emissiveLuxelIndicesBufferCreationInfo,
                                      &emissiveLuxelIndicesBuffer)))
    {
        return false;
    }

    static constexpr uint32_t ZERO = 0;
    static constexpr LunaBufferWriteInfo WRITE_INFO = {
        .size = sizeof(ZERO),
        .data = &ZERO,
        .sourceStageMask = VK_PIPELINE_STAGE_2_NONE,
        .sourceAccessMask = VK_ACCESS_2_NONE,
        .destinationStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .destinationAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
    };
    if (!CheckResult(lunaWriteDataToBuffer(context.device,
                                           context.commandBuffer,
                                           emissiveLuxelIndicesBuffer,
                                           &WRITE_INFO)))
    {
        return false;
    }

    static constexpr std::array DESCRIPTOR_POOL_SIZES = {
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
        },
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
        },
    };
    static constexpr LunaDescriptorPoolCreationInfo DESCRIPTOR_POOL_CREATION_INFO = {
        .maxSets = 1,
        .poolSizeCount = DESCRIPTOR_POOL_SIZES.size(),
        .poolSizes = DESCRIPTOR_POOL_SIZES.data(),
    };
    LunaDescriptorPool computeDescriptorPool{};
    if (!CheckResult(lunaCreateDescriptorPool(context.device, &DESCRIPTOR_POOL_CREATION_INFO, &computeDescriptorPool)))
    {
        return false;
    }
    const LunaDescriptorSetAllocationInfo allocationInfo = {
        .descriptorPool = computeDescriptorPool,
        .setLayoutCount = 1,
        .setLayouts = &computePipelineDescriptorSetLayout,
    };
    LunaDescriptorSet computeDescriptorSet{};
    LunaDescriptorSet *computeDescriptorSetPtr = &computeDescriptorSet;
    if (!CheckResult(lunaAllocateDescriptorSets(context.device, &allocationInfo, &computeDescriptorSetPtr)))
    {
        return false;
    }
    const LunaDescriptorImageInfo luxelNormalsImageInfo = {
        .image = luxelNormalsImage,
        .imageLayout = lunaGetImageLayout(luxelNormalsImage),
    };
    const LunaDescriptorBufferInfo emissiveLuxelIndicesBufferInfo = {
        .buffer = emissiveLuxelIndicesBuffer,
    };
    const std::array writes = {
        LunaWriteDescriptorSet{
            .descriptorSet = computeDescriptorSet,
            .bindingName = "Luxel Normals",
            .imageInfos = &luxelNormalsImageInfo,
        },
        LunaWriteDescriptorSet{
            .descriptorSet = computeDescriptorSet,
            .bindingName = "Emissive Luxel Indices",
            .bufferInfos = &emissiveLuxelIndicesBufferInfo,
        },
    };
    lunaWriteDescriptorSets(context.device, writes.size(), writes.data());

    if (!CheckResult(lunaBeginSingleUseCommandBuffer(context.device, context.commandBuffer)))
    {
        return false;
    }

    const LunaDescriptorSetBindInfo descriptorSetBindInfo = {
        .descriptorSetCount = 1,
        .descriptorSets = &computeDescriptorSet,
    };
    const uint64_t workGroupCount = lightmapSize.x * lightmapSize.y / 256u;
    const uint32_t width = std::min(workGroupCount, uint64_t{MAX_DISPATCH_DIMENSION});
    const uint32_t height = std::min(std::max(workGroupCount / width, uint64_t{1}), uint64_t{MAX_DISPATCH_DIMENSION});
    const uint32_t depth = std::max(workGroupCount / width / height, uint64_t{1});
    const LunaDispatchBaseInfo dispatchInfo = {
        .pipeline = computePipeline,
        .descriptorSetBindInfo = &descriptorSetBindInfo,
        .groupCountX = width,
        .groupCountY = height,
        .groupCountZ = depth,
    };
    if (!CheckResult(lunaDispatchBase(context.device, context.commandBuffer, &dispatchInfo)))
    {
        return false;
    }

    return CheckResult(lunaEndAndSubmitCommandBuffer(context.device, context.commandBuffer, &context.submitInfo));
}

LunaBuffer LightBaker::CreateLodVertexBufferForBLAS(const ModelLod &lod) const
{
    const VkDeviceSize bufferSize = lod.vertices.size() * sizeof(decltype(lod.vertices)::value_type);

    LunaBuffer buffer{};
    const LunaBufferCreationInfo creationInfo = {
        .size = bufferSize,
        .memoryType = LUNA_MEMORY_TYPE_DEVICE_LOCAL,
        .additionalUsageFlags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    };
    if (!CheckResult(lunaCreateBuffer(context.device, &creationInfo, &buffer)))
    {
        return LUNA_NULL_HANDLE;
    }
    const LunaBufferWriteInfo writeInfo = {
        .size = bufferSize,
        .data = lod.vertices.data(),
        .destinationStageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        .destinationAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR |
                                 VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
    };
    if (!CheckResult(lunaWriteDataToBuffer(context.device, context.commandBuffer, buffer, &writeInfo)))
    {
        lunaDestroyBuffer(buffer);
        return LUNA_NULL_HANDLE;
    }
    return buffer;
}

bool LightBaker::CreateBLAS(const ActorDefinitionManager &definitionManager,
                            const std::vector<BrushMeshBuilder> &meshBuilders,
                            const std::vector<Actor> &actors)
{
    // The position MUST be at the start of the struct for the acceleration structure to work properly
    static_assert(offsetof(MapVertex, position) == 0, "MapVertex::position must be at offset zero!");
    static_assert(offsetof(ModelVertex, position) == 0, "ModelVertex::position must be at offset zero!");
    // The position of the vertex must be three single precision floating point numbers,
    //  since we use VK_FORMAT_R32G32B32_SFLOAT as the vertex format.
    //  If the type used for position changes, update the vertex format accordingly.
    static_assert(std::same_as<decltype(MapVertex::position), glm::vec3>,
                  "MapVertex::position must be a three-component vector of single precision floats!");
    static_assert(std::same_as<decltype(ModelVertex::position), glm::vec3>,
                  "ModelVertex::position must be a three-component vector of single precision floats!");


    if (!CheckResult(lunaBeginSingleUseCommandBuffer(context.device, context.commandBuffer)))
    {
        return false;
    }

    bottomLevelAccelerationStructures.clear();
    bottomLevelAccelerationStructures.reserve(meshBuilders.size());
    VkDeviceSize indexBufferOffset = 0;
    for (const BrushMeshBuilder &meshBuilder: meshBuilders)
    {
        static constexpr VkTransformMatrixKHR TRANSFORM_MATRIX_IDENTITY = {
            {
                {1, 0, 0, 0},
                {0, 1, 0, 0},
                {0, 0, 1, 0},
            },
        };

        const VkDeviceOrHostAddressConstKHR vertexDataAddress = {
            .deviceAddress = lunaGetBufferDeviceAddress(context.device, brushesVertexBuffer),
        };
        const VkDeviceOrHostAddressConstKHR indexDataAddress = {
            .deviceAddress = lunaGetBufferDeviceAddress(context.device, brushesIndexBuffer) + indexBufferOffset,
        };
        const VkAccelerationStructureGeometryTrianglesDataKHR trianglesData = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
            .vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
            .vertexData = vertexDataAddress,
            .vertexStride = sizeof(MapVertex),
            .maxVertex = brushesVertexCount - 1,
            .indexType = VK_INDEX_TYPE_UINT32,
            .indexData = indexDataAddress,
        };
        const AccelerationStructure &blas = bottomLevelAccelerationStructures
                                                    .emplace_back(context,
                                                                  trianglesData,
                                                                  meshBuilder.GetIndices().size() / 3,
                                                                  TRANSFORM_MATRIX_IDENTITY,
                                                                  indexBufferOffset / sizeof(uint32_t),
                                                                  0xFF);
        if (!blas.IsValid())
        {
            return false;
        }

        indexBufferOffset += meshBuilder.GetIndices().size() * sizeof(uint32_t);
    }
    for (const Actor &actor: actors)
    {
        const ActorDefinition &actorDefinition = definitionManager.GetActorDefinition(actor.className);
        for (const std::shared_ptr<RenderDefinition> &renderDefinition: actorDefinition.renderDefinitions)
        {
            if (renderDefinition == nullptr ||
                renderDefinition->GetType() != RenderDefinition::RenderDefinitionType::RD_TYPE_MODEL)
            {
                continue;
            }
            ModelRenderDefinition *modelRenderDefinition = dynamic_cast<ModelRenderDefinition *>(renderDefinition
                                                                                                         .get());
            if (modelRenderDefinition == nullptr)
            {
                continue;
            }
            if (!modelRenderDefinition->GetAffectLightmap(actor))
            {
                break;
            }

            ModelAsset modelAsset{};
            const Error error = modelAsset.LoadFromAsset(context.pathManager
                                                                 .GetAssetPath(modelRenderDefinition->GetModel(actor)));
            if (error.Failed())
            {
                break;
            }
            const ModelLod &lod = modelAsset.GetLod(0);

            const LunaBuffer vertexBuffer = CreateLodVertexBufferForBLAS(lod);
            if (vertexBuffer == LUNA_NULL_HANDLE)
            {
                break;
            }

            for (const ModelLod::ModelComponent &component: lod.components)
            {
                // TODO: Respect the shadow casting property and transparency on the material, as well as mod color and vertex color

                const VkDeviceSize indexBufferSize = component.indices.size() *
                                                     sizeof(decltype(component.indices)::value_type);
                LunaBuffer indexBuffer{};
                const LunaBufferCreationInfo indexBufferCreationInfo = {
                    .size = indexBufferSize,
                    .memoryType = LUNA_MEMORY_TYPE_DEVICE_LOCAL,
                    .additionalUsageFlags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                                            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                };
                if (!CheckResult(lunaCreateBuffer(context.device, &indexBufferCreationInfo, &indexBuffer)))
                {
                    continue;
                }
                const LunaBufferWriteInfo writeInfo = {
                    .size = indexBufferSize,
                    .data = component.indices.data(),
                    .destinationStageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                    .destinationAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
                };
                if (!CheckResult(lunaWriteDataToBuffer(context.device, context.commandBuffer, indexBuffer, &writeInfo)))
                {
                    lunaDestroyBuffer(indexBuffer);
                    continue;
                }

                glm::mat4 transformMatrix = actor.GetTransformMatrix();
                transformMatrix = glm::translate(transformMatrix, component.centerOffset);

                const VkDeviceOrHostAddressConstKHR vertexDataAddress = {
                    .deviceAddress = lunaGetBufferDeviceAddress(context.device, vertexBuffer),
                };
                const VkDeviceOrHostAddressConstKHR indexDataAddress = {
                    .deviceAddress = lunaGetBufferDeviceAddress(context.device, indexBuffer),
                };
                const VkAccelerationStructureGeometryTrianglesDataKHR trianglesData = {
                    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
                    .vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
                    .vertexData = vertexDataAddress,
                    .vertexStride = sizeof(ModelVertex),
                    .maxVertex = static_cast<uint32_t>(lod.vertices.size() - 1),
                    .indexType = VK_INDEX_TYPE_UINT32,
                    .indexData = indexDataAddress,
                };
                const AccelerationStructure &blas = bottomLevelAccelerationStructures
                                                            .emplace_back(context,
                                                                          trianglesData,
                                                                          component.indices.size() / 3,
                                                                          VkTransformMatrixKHR{
                                                                              {
                                                                                  {
                                                                                      transformMatrix[0][0],
                                                                                      transformMatrix[1][0],
                                                                                      transformMatrix[2][0],
                                                                                      transformMatrix[3][0],
                                                                                  },
                                                                                  {
                                                                                      transformMatrix[0][1],
                                                                                      transformMatrix[1][1],
                                                                                      transformMatrix[2][1],
                                                                                      transformMatrix[3][1],
                                                                                  },
                                                                                  {
                                                                                      transformMatrix[0][2],
                                                                                      transformMatrix[1][2],
                                                                                      transformMatrix[2][2],
                                                                                      transformMatrix[3][2],
                                                                                  },
                                                                              },
                                                                          },
                                                                          0,
                                                                          0x80);
                if (!blas.IsValid())
                {
                    return false;
                }
            }

            break;
        }
    }

    return CheckResult(lunaEndAndSubmitCommandBuffer(context.device, context.commandBuffer, &context.submitInfo));
}

bool LightBaker::CreateTLAS()
{
    if (!CheckResult(lunaBeginSingleUseCommandBuffer(context.device, context.commandBuffer)))
    {
        return false;
    }

    tlas = AccelerationStructure(context, bottomLevelAccelerationStructures);
    if (!tlas.IsValid())
    {
        return false;
    }

    return CheckResult(lunaEndAndSubmitCommandBuffer(context.device, context.commandBuffer, &context.submitInfo));
}

bool LightBaker::CreateDirectLightingPipeline(const VkBool32 writeDirect)
{
    const VkShaderModule raygenShaderModule = context.GenerateShaderModule("assets/shaders/lightmap/"
                                                                           "direct_lighting.rgen",
                                                                           EShLangRayGen);
    if (raygenShaderModule == VK_NULL_HANDLE)
    {
        return false;
    }
    const VkShaderModule missShaderModule = context.GenerateShaderModule("assets/shaders/lightmap/miss.rmiss",
                                                                         EShLangMiss);
    if (missShaderModule == VK_NULL_HANDLE)
    {
        return false;
    }

    if (directLightingPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(lunaGetVkDevice(context.device), directLightingPipeline, nullptr);
    }

    const std::array raygenSpecializationData = {
        lightmapSize.x,
        lightmapSize.y,
        lightCount,
        writeDirect,
    };
    static constexpr SpecializationMapEntries<uint32_t, uint32_t, uint32_t, VkBool32> RAYGEN_MAP_ENTRIES{};
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
        .size = sizeof(uint32_t) * 2,
    };
    const std::array descriptorSetLayouts = {descriptorSetLayout, context.texturesDescriptorSetLayout};
    const VkPipelineLayoutCreateInfo layoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = descriptorSetLayouts.size(),
        .pSetLayouts = descriptorSetLayouts.data(),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &PUSH_CONSTANT_RANGE,
    };
    if (!CheckResult(vkCreatePipelineLayout(lunaGetVkDevice(context.device),
                                            &layoutCreateInfo,
                                            nullptr,
                                            &directLightingPipelineLayout)))
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
        .layout = directLightingPipelineLayout,
    };
    if (!CheckResult(vkCreateRayTracingPipelinesKHR(lunaGetVkDevice(context.device),
                                                    VK_NULL_HANDLE,
                                                    VK_NULL_HANDLE,
                                                    1,
                                                    &pipelineCreateInfo,
                                                    nullptr,
                                                    &directLightingPipeline)))
    {
        return false;
    }

    vkDestroyShaderModule(lunaGetVkDevice(context.device), raygenShaderModule, nullptr);
    vkDestroyShaderModule(lunaGetVkDevice(context.device), missShaderModule, nullptr);

    return CreateShaderBindingTables();
}

bool LightBaker::CreateGlobalIlluminationPipeline()
{
    const VkShaderModule raygenShaderModule = context.GenerateShaderModule("assets/shaders/lightmap/"
                                                                           "global_illumination.rgen",
                                                                           EShLangRayGen);
    if (raygenShaderModule == VK_NULL_HANDLE)
    {
        return false;
    }
    const VkShaderModule closestHitShaderModule = context.GenerateShaderModule("assets/shaders/lightmap/"
                                                                               "closesthit.rchit",
                                                                               EShLangClosestHit);
    if (closestHitShaderModule == VK_NULL_HANDLE)
    {
        return false;
    }


    const std::array raygenSpecializationData = {
        lightmapSize.x,
        lightmapSize.y,
        sampleCount,
        SAMPLES_PER_THREAD,
    };
    static constexpr SpecializationMapEntries<uint32_t, uint32_t, uint32_t, uint32_t> RAYGEN_MAP_ENTRIES{};
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
        .size = sizeof(uint32_t) * 2,
    };
    const VkPipelineLayoutCreateInfo layoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptorSetLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &PUSH_CONSTANT_RANGE,
    };
    if (!CheckResult(vkCreatePipelineLayout(lunaGetVkDevice(context.device),
                                            &layoutCreateInfo,
                                            nullptr,
                                            &globalIlluminationPipelineLayout)))
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
        .layout = globalIlluminationPipelineLayout,
    };
    if (!CheckResult(vkCreateRayTracingPipelinesKHR(lunaGetVkDevice(context.device),
                                                    VK_NULL_HANDLE,
                                                    VK_NULL_HANDLE,
                                                    1,
                                                    &pipelineCreateInfo,
                                                    nullptr,
                                                    &globalIlluminationPipeline)))
    {
        return false;
    }

    vkDestroyShaderModule(lunaGetVkDevice(context.device), raygenShaderModule, nullptr);
    vkDestroyShaderModule(lunaGetVkDevice(context.device), closestHitShaderModule, nullptr);

    return true;
}

bool LightBaker::CreateAndWriteDescriptorSet()
{
    static constexpr DescriptorSetLayoutBindings DESCRIPTOR_SET_LAYOUT_BINDINGS{
        std::pair{
            // accelerationStructure
            VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        },
        std::pair{
            // outputLightmap
            VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        },
        std::pair{
            // previousBounceLightmap
            VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
        },
        std::pair{
            // currentBounceLightmap
            VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
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
            VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
        },
        std::pair{
            // lightsData
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        },
        std::pair{
            // emissiveLuxelIndices
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

    static constexpr VkDescriptorSetLayoutCreateInfo DESCRIPTOR_SET_LAYOUT_CREATE_INFO = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = DESCRIPTOR_SET_LAYOUT_BINDINGS.size(),
        .pBindings = DESCRIPTOR_SET_LAYOUT_BINDINGS.data(),
    };
    if (!CheckResult(vkCreateDescriptorSetLayout(lunaGetVkDevice(context.device),
                                                 &DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                                 nullptr,
                                                 &descriptorSetLayout)))
    {
        return false;
    }

    static constexpr std::array DESCRIPTOR_POOL_SIZES = {
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
            .descriptorCount = 1,
        },
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
            .descriptorCount = 3,
        },
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 3,
        },
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 4,
        },
    };
    static constexpr VkDescriptorPoolCreateInfo DESCRIPTOR_POOL_CREATE_INFO = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1,
        .poolSizeCount = DESCRIPTOR_POOL_SIZES.size(),
        .pPoolSizes = DESCRIPTOR_POOL_SIZES.data(),
    };
    if (!CheckResult(vkCreateDescriptorPool(lunaGetVkDevice(context.device),
                                            &DESCRIPTOR_POOL_CREATE_INFO,
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
    if (!CheckResult(vkAllocateDescriptorSets(lunaGetVkDevice(context.device),
                                              &descriptorSetAllocateInfo,
                                              &descriptorSet)))
    {
        return false;
    }

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
    const VkDescriptorBufferInfo emissiveLuxelIndicesBufferInfo = {
        .buffer = lunaGetVkBuffer(emissiveLuxelIndicesBuffer),
        .offset = lunaGetBufferOffset(emissiveLuxelIndicesBuffer),
        .range = lunaGetBufferSize(emissiveLuxelIndicesBuffer),
    };
    const VkDescriptorBufferInfo vertexBufferInfo = {
        .buffer = lunaGetVkBuffer(brushesVertexBuffer),
        .offset = lunaGetBufferOffset(brushesVertexBuffer),
        .range = lunaGetBufferSize(brushesVertexBuffer),
    };
    const VkDescriptorBufferInfo indexBufferInfo = {
        .buffer = lunaGetVkBuffer(brushesIndexBuffer),
        .offset = lunaGetBufferOffset(brushesIndexBuffer),
        .range = lunaGetBufferSize(brushesIndexBuffer),
    };
    const std::array descriptorSetWrites = {
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = 4,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo = &luxelPositionsImageInfo,
        },
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = 5,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo = &luxelNormalsImageInfo,
        },
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = 6,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo = &luxelAlbedosImageInfo,
        },
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = 7,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &lightsBufferInfo,
        },
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = 8,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &emissiveLuxelIndicesBufferInfo,
        },
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = 9,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &vertexBufferInfo,
        },
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = 10,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &indexBufferInfo,
        },
    };
    vkUpdateDescriptorSets(lunaGetVkDevice(context.device),
                           descriptorSetWrites.size(),
                           descriptorSetWrites.data(),
                           0,
                           nullptr);

    tlas.WriteToDescriptorSet(descriptorSet, 0);
    lightmapOne.WriteToDescriptorSet(descriptorSet, 1);
    lightmapTwo.WriteToDescriptorSet(descriptorSet, 2);
    lightmapThree.WriteToDescriptorSet(descriptorSet, 3);

    return true;
}

bool LightBaker::CreateShaderBindingTables()
{
    const LunaBufferCreationInfo shaderBindingTableCreationInfo = {
        .size = context.physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
        .alignment = context.physicalDeviceRayTracingPipelineProperties.shaderGroupBaseAlignment,
        .memoryType = LUNA_MEMORY_TYPE_DEVICE_LOCAL,
        .additionalUsageFlags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR |
                                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    };
    if (!CheckResult(lunaCreateBuffer(context.device,
                                      &shaderBindingTableCreationInfo,
                                      &directLightingRaygenShaderBindingTable)))
    {
        return false;
    }
    if (!CheckResult(lunaCreateBuffer(context.device, &shaderBindingTableCreationInfo, &missShaderBindingTable)))
    {
        return false;
    }
    if (!CheckResult(lunaCreateBuffer(context.device,
                                      &shaderBindingTableCreationInfo,
                                      &globalIlluminationRaygenShaderBindingTable)))
    {
        return false;
    }
    if (!CheckResult(lunaCreateBuffer(context.device, &shaderBindingTableCreationInfo, &closestHitShaderBindingTable)))
    {
        return false;
    }
    if (!CheckResult(lunaCreateBuffer(context.device, &shaderBindingTableCreationInfo, &nullShaderBindingTable)))
    {
        return false;
    }

    std::vector<char> shaderHandles(2 * context.physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize);
    if (!CheckResult(vkGetRayTracingShaderGroupHandlesKHR(lunaGetVkDevice(context.device),
                                                          directLightingPipeline,
                                                          0,
                                                          2,
                                                          shaderHandles.size(),
                                                          shaderHandles.data())))
    {
        return false;
    }

    const LunaBufferWriteInfo raygenShaderBindingTableWriteInfo = {
        .size = context.physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
        .data = shaderHandles.data(),
        .sourceStageMask = VK_PIPELINE_STAGE_2_NONE,
        .sourceAccessMask = VK_ACCESS_2_NONE,
        .destinationStageMask = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
        .destinationAccessMask = VK_ACCESS_2_SHADER_BINDING_TABLE_READ_BIT_KHR,
    };
    if (!CheckResult(lunaWriteDataToBuffer(context.device,
                                           context.commandBuffer,
                                           directLightingRaygenShaderBindingTable,
                                           &raygenShaderBindingTableWriteInfo)))
    {
        return false;
    }

    const LunaBufferWriteInfo secondShaderBindingTableWriteInfo = {
        .size = context.physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
        .data = shaderHandles.data() + context.physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
        .sourceStageMask = VK_PIPELINE_STAGE_2_NONE,
        .sourceAccessMask = VK_ACCESS_2_NONE,
        .destinationStageMask = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
        .destinationAccessMask = VK_ACCESS_2_SHADER_BINDING_TABLE_READ_BIT_KHR,
    };
    if (!CheckResult(lunaWriteDataToBuffer(context.device,
                                           context.commandBuffer,
                                           missShaderBindingTable,
                                           &secondShaderBindingTableWriteInfo)))
    {
        return false;
    }


    if (!CheckResult(vkGetRayTracingShaderGroupHandlesKHR(lunaGetVkDevice(context.device),
                                                          globalIlluminationPipeline,
                                                          0,
                                                          2,
                                                          shaderHandles.size(),
                                                          shaderHandles.data())))
    {
        return false;
    }

    if (!CheckResult(lunaWriteDataToBuffer(context.device,
                                           context.commandBuffer,
                                           globalIlluminationRaygenShaderBindingTable,
                                           &raygenShaderBindingTableWriteInfo)))
    {
        return false;
    }

    if (!CheckResult(lunaWriteDataToBuffer(context.device,
                                           context.commandBuffer,
                                           closestHitShaderBindingTable,
                                           &secondShaderBindingTableWriteInfo)))
    {
        return false;
    }

    std::vector<uint8_t> data(context.physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize);
    const LunaBufferWriteInfo nullShaderBindingTableWriteInfo = {
        .size = context.physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
        .data = data.data(),
        .sourceStageMask = VK_PIPELINE_STAGE_2_NONE,
        .sourceAccessMask = VK_ACCESS_2_NONE,
        .destinationStageMask = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
        .destinationAccessMask = VK_ACCESS_2_SHADER_BINDING_TABLE_READ_BIT_KHR,
    };
    return CheckResult(lunaWriteDataToBuffer(context.device,
                                             context.commandBuffer,
                                             nullShaderBindingTable,
                                             &nullShaderBindingTableWriteInfo));
}

bool LightBaker::SingleBakeIteration(const uint32_t width,
                                     const uint32_t height,
                                     const uint32_t baseLuxelIndex,
                                     const uint32_t baseSampleIndex,
                                     const bool directLighting) const
{
    if (!CheckResult(lunaBeginSingleUseCommandBuffer(context.device, context.commandBuffer)))
    {
        return false;
    }

    const VkCommandBuffer vkCommandBuffer = lunaGetVkCommandBuffer(context.commandBuffer);

    vkCmdBindDescriptorSets(vkCommandBuffer,
                            VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                            directLighting ? directLightingPipelineLayout : globalIlluminationPipelineLayout,
                            0,
                            1,
                            &descriptorSet,
                            0,
                            nullptr);
    if (directLighting)
    {
        vkCmdBindDescriptorSets(vkCommandBuffer,
                                VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                                directLightingPipelineLayout,
                                1,
                                1,
                                &context.texturesDescriptorSet,
                                0,
                                nullptr);
    }
    vkCmdPushConstants(vkCommandBuffer,
                       directLighting ? directLightingPipelineLayout : globalIlluminationPipelineLayout,
                       VK_SHADER_STAGE_RAYGEN_BIT_KHR,
                       0,
                       sizeof(baseLuxelIndex),
                       &baseLuxelIndex);
    if (!directLighting)
    {
        vkCmdPushConstants(vkCommandBuffer,
                           globalIlluminationPipelineLayout,
                           VK_SHADER_STAGE_RAYGEN_BIT_KHR,
                           sizeof(baseLuxelIndex),
                           sizeof(baseSampleIndex),
                           &baseSampleIndex);
    }
    const VkStridedDeviceAddressRegionKHR nullShaderBindingTableAddressRegion = {
        .deviceAddress = lunaGetBufferDeviceAddress(context.device, nullShaderBindingTable),
        .stride = context.physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
        .size = context.physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
    };
    if (directLighting)
    {
        vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, directLightingPipeline);

        const VkStridedDeviceAddressRegionKHR raygenShaderBindingTableAddressRegion = {
            .deviceAddress = lunaGetBufferDeviceAddress(context.device, directLightingRaygenShaderBindingTable),
            .stride = context.physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
            .size = context.physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
        };
        const VkStridedDeviceAddressRegionKHR missShaderBindingTableAddressRegion = {
            .deviceAddress = lunaGetBufferDeviceAddress(context.device, missShaderBindingTable),
            .stride = context.physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
            .size = context.physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
        };
        vkCmdTraceRaysKHR(vkCommandBuffer,
                          &raygenShaderBindingTableAddressRegion,
                          &missShaderBindingTableAddressRegion,
                          &nullShaderBindingTableAddressRegion,
                          &nullShaderBindingTableAddressRegion,
                          width,
                          height,
                          1);
    } else
    {
        vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, globalIlluminationPipeline);

        const VkStridedDeviceAddressRegionKHR raygenShaderBindingTableAddressRegion = {
            .deviceAddress = lunaGetBufferDeviceAddress(context.device, globalIlluminationRaygenShaderBindingTable),
            .stride = context.physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
            .size = context.physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
        };
        const VkStridedDeviceAddressRegionKHR closestHitShaderBindingTableAddressRegion = {
            .deviceAddress = lunaGetBufferDeviceAddress(context.device, closestHitShaderBindingTable),
            .stride = context.physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
            .size = context.physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
        };
        vkCmdTraceRaysKHR(vkCommandBuffer,
                          &raygenShaderBindingTableAddressRegion,
                          &nullShaderBindingTableAddressRegion,
                          &closestHitShaderBindingTableAddressRegion,
                          &nullShaderBindingTableAddressRegion,
                          width,
                          height,
                          1);
    }
    return CheckResult(lunaEndAndSubmitCommandBuffer(context.device, context.commandBuffer, &context.submitInfo));
}
