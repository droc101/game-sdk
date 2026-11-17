//
// Created by NBT22 on 11/16/26.
//

#pragma once

#include <cstdint>
#include <glm/vec2.hpp>
#include <limits>
#include <luna/lunaTypes.h>
#include <vector>
#include <vulkan/vulkan_core.h>
#include "VulkanContext.hpp"

class Lightmap
{
    public:
        /// @warning The @c context reference must outlive the constructed @c Lightmap
        Lightmap(const VulkanContext &context, glm::uvec2 size);

        void WriteToDescriptorSet(const VkDescriptorSet &descriptorSet, uint32_t bindingIndex) const;

        void AddPadding(std::vector<uint16_t> &output) const;

    private:
        using float16_t = _Float16; // NOLINT(*-identifier-naming)

        static constexpr bool IsZero(const float16_t value)
        {
            return value == 0 || (value < std::numeric_limits<float16_t>::epsilon() &&
                                  -std::numeric_limits<float16_t>::epsilon() < value);
        }
        static constexpr bool NonZero(const float16_t value)
        {
            // First check if it's one since this function almost always takes a value that is either zero or one
            return value == 1 ||
                   std::numeric_limits<float16_t>::epsilon() < value ||
                   value < -std::numeric_limits<float16_t>::epsilon();
        }

        const VulkanContext &context;
        glm::uvec2 size;
        LunaBuffer buffer{};
        VkBufferView bufferView{};
};
