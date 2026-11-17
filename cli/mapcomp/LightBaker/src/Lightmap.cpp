//
// Created by NBT22 on 11/16/26.
//

#include "Lightmap.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <libassets/util/LightmapHelpers.hpp>
#include <luna/lunaBuffer.h>
#include <luna/lunaDevice.h>
#include <luna/lunaTypes.h>
#include <vector>
#include <volk.h>
#include <vulkan/vulkan_core.h>
#include "VulkanContext.hpp"

Lightmap::Lightmap(const VulkanContext &context, const glm::uvec2 size): context(context), size(size)
{
    const LunaBufferCreationInfo lightmapCreationInfo = {
        .size = size.x * size.y * sizeof(float16_t) * 4,
        .alignment = context.physicalDeviceProperties.properties.limits.minTexelBufferOffsetAlignment,
        .memoryType = LUNA_MEMORY_TYPE_HOST_CACHED,
        .additionalUsageFlags = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
    };
    if (!VulkanContext::CheckResult(lunaCreateBuffer(context.device, &lightmapCreationInfo, &buffer)))
    {
        buffer = LUNA_NULL_HANDLE;
        return;
    }
    if (!VulkanContext::CheckResult(lunaFillBuffer(context.device, context.commandBuffer, buffer, 0)))
    {
        lunaDestroyBuffer(buffer);
        buffer = LUNA_NULL_HANDLE;
        return;
    }
    const VkBufferViewCreateInfo bufferViewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
        .buffer = lunaGetVkBuffer(buffer),
        .format = VK_FORMAT_R16G16B16A16_SFLOAT,
        .offset = lunaGetBufferOffset(buffer),
        .range = lunaGetBufferSize(buffer),
    };
    if (!VulkanContext::CheckResult(vkCreateBufferView(lunaGetVkDevice(context.device),
                                                       &bufferViewCreateInfo,
                                                       nullptr,
                                                       &bufferView)))
    {
        lunaDestroyBuffer(buffer);
        buffer = LUNA_NULL_HANDLE;
        bufferView = LUNA_NULL_HANDLE;
        return;
    }
}

void Lightmap::WriteToDescriptorSet(const VkDescriptorSet &descriptorSet, const uint32_t bindingIndex) const
{
    const VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptorSet,
        .dstBinding = bindingIndex,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
        .pTexelBufferView = &bufferView,
    };
    vkUpdateDescriptorSets(lunaGetVkDevice(context.device), 1, &write, 0, nullptr);
}

void Lightmap::AddPadding(std::vector<uint16_t> &output) const
{
    static_assert(2 <= LightmapHelpers::LIGHTMAP_PADDING,
                  "Padding does not work correctly with fewer than two luxels of padding.");

    static constexpr bool OVERWRITE_EDGE = false;

    lunaDeviceWaitIdle(context.device);

    float16_t *lightmapData = static_cast<float16_t *>(lunaGetBufferDataPointer(buffer));

    output.clear();
    output.resize(size.x * size.y * 4);
    std::copy_n(reinterpret_cast<const uint16_t *>(lightmapData), output.size(), output.data());
    float16_t *outputData = reinterpret_cast<float16_t *>(output.data());

    const uint32_t lightmapWidthChannelCount = size.x * 4;
    for (uint32_t i = 3; i < lightmapWidthChannelCount * (size.y - LightmapHelpers::LIGHTMAP_PADDING); i += 4)
    {
        float16_t *edgeLuxel; // NOLINT(*-init-variables)
        std::array<float16_t, 4> color; // NOLINT(*-pro-type-member-init)
        const float16_t currentLuxelAlpha = lightmapData[i];
        if (IsZero(currentLuxelAlpha) &&
            NonZero(lightmapData[i + LightmapHelpers::LIGHTMAP_PADDING * lightmapWidthChannelCount]))
        {
            // We've detected that there is a luxel with vertical padding that potentially ends at our current luxel

            bool hasFullPadding = true;
            for (uint32_t j = LightmapHelpers::LIGHTMAP_PADDING - 1; j > 0; j--)
            {
                if (NonZero(lightmapData[i + j * lightmapWidthChannelCount]))
                {
                    hasFullPadding = false;
                    break;
                }
            }

            if (hasFullPadding)
            {
                edgeLuxel = &outputData[i + LightmapHelpers::LIGHTMAP_PADDING * lightmapWidthChannelCount - 3];

                // Check the alpha of the luxel two rows down
                if (NonZero(lightmapData[i + (LightmapHelpers::LIGHTMAP_PADDING + 2) * lightmapWidthChannelCount]) &&
                    OVERWRITE_EDGE)
                {
                    // There are at least three used luxels.
                    // If there are two or fewer luxels we just use the luxel at the edge,
                    //  otherwise we pull the luxel inset one from the edge to reduce the effects of light leakage.

                    std::copy_n(&lightmapData[i +
                                              (LightmapHelpers::LIGHTMAP_PADDING + 1) * lightmapWidthChannelCount -
                                              3],
                                4,
                                color.data());
                    assert(NonZero(color.at(3))); // Internal state check.

                    std::copy_n(color.data(), 4, edgeLuxel);
                } else
                {
                    std::copy_n(&lightmapData[i + LightmapHelpers::LIGHTMAP_PADDING * lightmapWidthChannelCount - 3],
                                4,
                                color.data());
                }
                for (uint32_t j = LightmapHelpers::LIGHTMAP_PADDING; j > 0; j--)
                {
                    std::copy_n(color.data(), 4, edgeLuxel - j * lightmapWidthChannelCount);
                }
            }
        } else if (NonZero(currentLuxelAlpha) && IsZero(lightmapData[i + lightmapWidthChannelCount]))
        {
            // We've detected that our current luxel is the bottom edge

            edgeLuxel = &outputData[i - 3];

            if (i > 2 * lightmapWidthChannelCount &&
                NonZero(lightmapData[i - 2 * lightmapWidthChannelCount]) &&
                OVERWRITE_EDGE)
            {
                // There are at least three used luxels.
                // If there are two or fewer luxels we just use the luxel at the edge,
                //  otherwise we pull the luxel inset one from the edge to reduce the effects of light leakage.

                std::copy_n(&lightmapData[i - lightmapWidthChannelCount - 3], 4, color.data());
                assert(NonZero(color.at(4))); // Internal state check.

                std::copy_n(color.data(), 4, edgeLuxel);
            } else
            {
                std::copy_n(&lightmapData[i - 3], 4, color.data());
            }
            // We assume that there is sufficient padding, to save time on checking
            for (uint32_t j = LightmapHelpers::LIGHTMAP_PADDING; j > 0; j--)
            {
                std::copy_n(color.data(), 4, edgeLuxel + j * lightmapWidthChannelCount);
            }
        }

        if (IsZero(currentLuxelAlpha) && NonZero(lightmapData[i + LightmapHelpers::LIGHTMAP_PADDING * 4]))
        {
            // We've detected that there is a luxel with horizontal padding that potentially ends at our current luxel

            bool hasFullPadding = true;
            for (uint32_t j = LightmapHelpers::LIGHTMAP_PADDING - 1; j > 0; j--)
            {
                if (NonZero(lightmapData[i + j * 4]))
                {
                    hasFullPadding = false;
                    break;
                }
            }

            if (hasFullPadding)
            {
                bool topCorner = IsZero(lightmapData[i +
                                                     LightmapHelpers::LIGHTMAP_PADDING * 4 -
                                                     lightmapWidthChannelCount]);
                if (topCorner)
                {
                    for (uint32_t j = LightmapHelpers::LIGHTMAP_PADDING; j > 0; j--)
                    {
                        for (uint32_t k = 0; k <= LightmapHelpers::LIGHTMAP_PADDING; k++)
                        {
                            if (NonZero(lightmapData[i - j * lightmapWidthChannelCount + k * 4]))
                            {
                                topCorner = false;
                                break;
                            }
                        }
                    }
                }
                bool bottomCorner = IsZero(lightmapData[i +
                                                        LightmapHelpers::LIGHTMAP_PADDING * 4 +
                                                        lightmapWidthChannelCount]);
                if (bottomCorner)
                {
                    for (uint32_t j = LightmapHelpers::LIGHTMAP_PADDING; j > 0; j--)
                    {
                        for (uint32_t k = 0; k <= LightmapHelpers::LIGHTMAP_PADDING; k++)
                        {
                            if (NonZero(lightmapData[i + j * lightmapWidthChannelCount + k * 4]))
                            {
                                bottomCorner = false;
                                break;
                            }
                        }
                    }
                }

                edgeLuxel = &outputData[i + LightmapHelpers::LIGHTMAP_PADDING * 4 - 3];

                // Check the alpha of the luxel two columns over
                if (NonZero(lightmapData[i + (LightmapHelpers::LIGHTMAP_PADDING + 2) * 4]) && OVERWRITE_EDGE)
                {
                    // There are at least three used luxels.
                    // If there are two or fewer luxels we just use the luxel at the edge,
                    //  otherwise we pull the luxel inset one from the edge to reduce the effects of light leakage.

                    std::copy_n(&lightmapData[i + (LightmapHelpers::LIGHTMAP_PADDING + 1) * 4 - 3], 4, color.data());
                    assert(NonZero(color.at(4))); // Internal state check.

                    std::copy_n(color.data(), 4, edgeLuxel);
                } else
                {
                    std::copy_n(&lightmapData[i + LightmapHelpers::LIGHTMAP_PADDING * 4 - 3], 4, color.data());
                }
                for (uint32_t j = LightmapHelpers::LIGHTMAP_PADDING; j > 0; j--)
                {
                    std::copy_n(color.data(), 4, edgeLuxel - j * 4);
                    if (topCorner)
                    {
                        for (uint32_t k = LightmapHelpers::LIGHTMAP_PADDING; k > 0; k--)
                        {
                            std::copy_n(color.data(), 4, edgeLuxel - j * lightmapWidthChannelCount - k * 4);
                        }
                    }
                    if (bottomCorner)
                    {
                        for (uint32_t k = LightmapHelpers::LIGHTMAP_PADDING; k > 0; k--)
                        {
                            std::copy_n(color.data(), 4, edgeLuxel + j * lightmapWidthChannelCount - k * 4);
                        }
                    }
                }
            }
        } else if (NonZero(currentLuxelAlpha) && IsZero(lightmapData[i + 4]))
        {
            // We've detected that our current luxel is the right edge

            bool topCorner = IsZero(lightmapData[i +
                                                 LightmapHelpers::LIGHTMAP_PADDING * 4 -
                                                 lightmapWidthChannelCount]);
            if (topCorner)
            {
                for (uint32_t j = LightmapHelpers::LIGHTMAP_PADDING; j > 0; j--)
                {
                    for (uint32_t k = 0; k <= LightmapHelpers::LIGHTMAP_PADDING; k++)
                    {
                        if (NonZero(lightmapData[i - j * lightmapWidthChannelCount + k * 4]))
                        {
                            topCorner = false;
                            break;
                        }
                    }
                }
            }
            bool bottomCorner = IsZero(lightmapData[i +
                                                    LightmapHelpers::LIGHTMAP_PADDING * 4 +
                                                    lightmapWidthChannelCount]);
            if (bottomCorner)
            {
                for (uint32_t j = LightmapHelpers::LIGHTMAP_PADDING; j > 0; j--)
                {
                    for (uint32_t k = 0; k <= LightmapHelpers::LIGHTMAP_PADDING; k++)
                    {
                        if (NonZero(lightmapData[i + j * lightmapWidthChannelCount + k * 4]))
                        {
                            bottomCorner = false;
                            break;
                        }
                    }
                }
            }

            edgeLuxel = &outputData[i - 3];

            if (i > 2 * 4 && NonZero(lightmapData[i - 2 * 4]) && OVERWRITE_EDGE)
            {
                // There are at least three used luxels.
                // If there are two or fewer luxels we just use the luxel at the edge,
                //  otherwise we pull the luxel inset one from the edge to reduce the effects of light leakage.

                std::copy_n(&lightmapData[i - 4 - 3], 4, color.data());
                assert(NonZero(color.at(4))); // Internal state check.

                std::copy_n(color.data(), 4, edgeLuxel);
            } else
            {
                std::copy_n(&lightmapData[i - 3], 4, color.data());
            }
            // We assume that there is sufficient padding, to save time on checking
            for (uint32_t j = LightmapHelpers::LIGHTMAP_PADDING; j > 0; j--)
            {
                std::copy_n(color.data(), 4, edgeLuxel + j * 4);
                if (topCorner)
                {
                    for (uint32_t k = LightmapHelpers::LIGHTMAP_PADDING; k > 0; k--)
                    {
                        std::copy_n(color.data(), 4, edgeLuxel - j * lightmapWidthChannelCount + k * 4);
                    }
                }
                if (bottomCorner)
                {
                    for (uint32_t k = LightmapHelpers::LIGHTMAP_PADDING; k > 0; k--)
                    {
                        std::copy_n(color.data(), 4, edgeLuxel + j * lightmapWidthChannelCount + k * 4);
                    }
                }
            }
        }
    }
}
