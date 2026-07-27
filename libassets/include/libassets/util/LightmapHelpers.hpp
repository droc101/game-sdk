//
// Created by NBT22 on 7/27/26.
//

#pragma once

#include <cstddef>
#include <glm/glm.hpp>
#include <stb_rect_pack.h>

#include "libassets/type/Actor.h"

namespace LightmapHelpers
{
/// Number of pixels around each lightmap rectangle to not use to prevent light spill from texture filtering
static inline constexpr size_t LIGHTMAP_PADDING = 3;
/// The largest size that each side of the lightmap can be
constexpr uint32_t MAX_LIGHTMAP_SIZE = 1 << 14;

bool FitLightmap(std::vector<stbrp_rect> &rects, glm::uvec2 &lightmapSize);

glm::vec2 GetUv(const glm::uvec2 &lightmapSize, const stbrp_rect &rect, const glm::vec2 &position);
} // namespace LightmapHelpers
