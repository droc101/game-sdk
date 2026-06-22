//
// Created by NBT22 on 3/11/26.
//

#include "LightBaker.hpp"
#include <cstdint>
#include <libassets/type/MapVertex.h>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include "LevelMeshBuilder.h"
#include "Light.h"
#include "LightBakerGpu.hpp"

bool LightBaker::Bake(const std::unordered_map<std::string, LevelMeshBuilder> &meshBuilders,
                      const std::vector<Light> &lights,
                      const glm::uvec2 &lightmapSize,
                      std::vector<uint16_t> &pixelData)
{
    // This is checks that MapVertex and Light are both POD, which is required to directly write from the pointer to the buffer.
    //  If they are not POD then the data will not be properly packed in memory.
    static_assert(std::is_standard_layout_v<MapVertex> &&
                  std::is_trivially_default_constructible_v<MapVertex> &&
                  std::is_trivially_copyable_v<MapVertex>);
    static_assert(std::is_standard_layout_v<Light> &&
                  std::is_trivially_default_constructible_v<Light> &&
                  std::is_trivially_copyable_v<Light>);

    static constexpr uint64_t RAY_COUNT = (uint64_t{1} << 32);
    static constexpr uint32_t BOUNCE_COUNT = 1;
    static constexpr uint32_t SAMPLE_COUNT = 8192;
    static_assert(RAY_COUNT - 1 == static_cast<uint64_t>(static_cast<double>(RAY_COUNT) - 1),
                  "Ray count must be representable as a double in order to be preserved in the shader!");
    static_assert(RAY_COUNT % (1 << 15) == 0, "Ray count must be a multiple of (1 << 15)!");

    LightBakerGpu baker{};
    if (!baker.IsInitialized())
    {
        return false;
    }
    if (!baker.Bake(meshBuilders, lights, lightmapSize, BOUNCE_COUNT, SAMPLE_COUNT, pixelData))
    {
        return false;
    }

    // TODO: Cleanup
    return true;
}
