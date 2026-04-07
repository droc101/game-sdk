//
// Created by NBT22 on 3/11/26.
//

#include "LightBaker.hpp"
#include <cassert>
#include <cstdint>
#include <glm/geometric.hpp>
#include <libassets/type/MapVertex.h>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include "LevelMeshBuilder.h"
#include "Light.h"
#include "LightBakerCpu.hpp"
#include "LightBakerGpu.hpp"

bool LightBaker::bake(const std::unordered_map<std::string, LevelMeshBuilder> &meshBuilders,
                      const std::vector<Light> &lights,
                      std::vector<uint8_t> &pixelData,
                      const glm::uvec2 &lightmapSize,
                      const bool useCpu)
{
    // This is checks that MapVertex and Light are both POD, which is required to directly write from the pointer to the buffer.
    //  If they are not POD then the data will not be properly packed in memory.
    static_assert(std::is_standard_layout_v<MapVertex> &&
                  std::is_trivially_default_constructible_v<MapVertex> &&
                  std::is_trivially_copyable_v<MapVertex>);
    static_assert(std::is_standard_layout_v<Light> &&
                  std::is_trivially_default_constructible_v<Light> &&
                  std::is_trivially_copyable_v<Light>);

    if (useCpu)
    {
        LightBakerCpu::bake(meshBuilders, lights, pixelData, lightmapSize);
    } else
    {
        LightBakerGpu baker = LightBakerGpu();
        if (!baker.isInitialized())
        {
            return false;
        }
        if (!baker.bake(meshBuilders, lights, lightmapSize, pixelData))
        {
            return false;
        }
    }

    // TODO: Cleanup
    return true;
}
