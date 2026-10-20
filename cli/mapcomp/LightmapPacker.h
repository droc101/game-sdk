//
// Created by droc101 on 10/20/26.
//

#ifndef GAME_SDK_LIGHTMAPPACKER_H
#define GAME_SDK_LIGHTMAPPACKER_H

#include <cstdint>
#include <glm/vec2.hpp>
#include <libassets/type/Brush.h>
#include <libassets/util/SearchPathManager.h>
#include <stb_rect_pack.h>
#include <vector>

class BrushMeshBuilder;

class LightmapPacker
{
    public:
        struct FaceData
        {
                std::vector<uint32_t> indices;
                std::vector<glm::vec2> positionsInRect;
                stbrp_rect rect;
        };

        LightmapPacker() = delete;

        static bool CalculateLightmapUvs(glm::uvec2 &lightmapSize,
                                         std::vector<BrushMeshBuilder> &meshBuilders,
                                         const SearchPathManager &pathMgr);

        static FaceData CalculateFaceData(const Brush::Face &face,
                                          const Brush::TriangulatedFace &triangulatedFace,
                                          const std::vector<uint32_t> &offsetIndices);
};


#endif //GAME_SDK_LIGHTMAPPACKER_H
