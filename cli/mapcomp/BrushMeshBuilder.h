//
// Created by droc101 on 11/16/25.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <libassets/asset/LevelMaterialAsset.h>
#include <libassets/type/Brush.h>
#include <libassets/type/MapVertex.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/SearchPathManager.h>
#include <string>
#include <vector>
#include "LightmapPacker.h"

class BrushMeshBuilder
{
        friend class LightmapPacker;

    public:

        BrushMeshBuilder(const SearchPathManager &pathManager, const std::string &materialPath);

        void AddFace(const Brush &brush, size_t faceIndex);

        /**
         * Write this mesh to a DataWriter
         * @param writer The DataWriter to write to
         */
        void Write(DataWriter &writer) const;

        /**
         * Check if this builder is empty
         */
        [[nodiscard]] bool IsEmpty() const;

        [[nodiscard]] const std::vector<MapVertex> &GetVertices() const
        {
            return vertices;
        }

        [[nodiscard]] const std::vector<uint32_t> &GetIndices() const
        {
            return indices;
        }

        [[nodiscard]] const std::string &GetMaterialPath() const;

    private:
        std::vector<MapVertex> vertices{};
        std::vector<uint32_t> indices{};
        std::vector<LightmapPacker::FaceData> faceDatas{};
        uint32_t currentIndex = 0;
        SearchPathManager pathManager;
        std::string materialPath;
        LevelMaterialAsset materialAsset{};
};
