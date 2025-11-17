//
// Created by droc101 on 11/16/25.
//

#pragma once
#include <cstddef>
#include <cstdint>
#include <libassets/asset/LevelMaterialAsset.h>
#include <libassets/type/ModelVertex.h>
#include <libassets/type/Sector.h>
#include <vector>


class LevelMeshBuilder
{
    public:
        LevelMeshBuilder() = default;

        void AddWall(const Sector &sector, size_t wallIndex);
        void AddFloor(const Sector &sector);
        void AddCeiling(const Sector &sector);

        void Write(DataWriter &writer) const;

    private:
        std::vector<ModelVertex> vertices{};
        std::vector<uint32_t> indices{};
        uint32_t currentIndex = 0;

        void AddSectorBase(const Sector &sector, bool isFloor);
};
