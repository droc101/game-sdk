//
// Created by NBT22 on 7/27/26.
//

#define STB_RECT_PACK_IMPLEMENTATION
#include <libassets/util/LightmapHelpers.hpp>

namespace LightmapHelpers
{
bool FitLightmap(std::vector<stbrp_rect> &rects, glm::uvec2 &lightmapSize)
{
    lightmapSize.x = 1 << 4;
    lightmapSize.y = 1 << 4;

    stbrp_context context{};
    std::vector<stbrp_node> nodes{};
    bool wasHeightChangedLast = true;
    while (true)
    {
        nodes.clear();
        nodes.resize(lightmapSize.x * 2);
        stbrp_init_target(&context,
                          static_cast<int>(lightmapSize.x),
                          static_cast<int>(lightmapSize.y),
                          nodes.data(),
                          static_cast<int>(nodes.size()));

        if (stbrp_pack_rects(&context, rects.data(), static_cast<int>(rects.size())) == 0)
        {
            if (lightmapSize.x == MAX_LIGHTMAP_SIZE && lightmapSize.y == MAX_LIGHTMAP_SIZE)
            {
                return false;
            }
            if (wasHeightChangedLast)
            {
                lightmapSize.x = lightmapSize.x << 1;
                wasHeightChangedLast = false;
            } else
            {
                lightmapSize.y = lightmapSize.y << 1;
                wasHeightChangedLast = true;
            }
        } else
        {
            break;
        }
    }
    return true;
}

glm::vec2 GetUv(const glm::uvec2 &lightmapSize, const stbrp_rect &rect, const glm::vec2 &position)
{
    return {
        (static_cast<float>(rect.x + LIGHTMAP_PADDING) +
         position.x * static_cast<float>(rect.w - 2 * LIGHTMAP_PADDING)) /
                static_cast<float>(lightmapSize.x),
        (static_cast<float>(rect.y + LIGHTMAP_PADDING) +
         position.y * static_cast<float>(rect.h - 2 * LIGHTMAP_PADDING)) /
                static_cast<float>(lightmapSize.y),
    };
}
} // namespace LightmapHelpers
