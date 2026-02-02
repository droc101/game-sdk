//
// Created by droc101 on 11/17/25.
//

#pragma once

#include <libassets/asset/MapAsset.h>
#include <libassets/util/Error.h>
#include <string>

#include "libassets/asset/LevelMaterialAsset.h"

class MapCompiler
{
    public:
        explicit MapCompiler(const std::string &assetsDirectory);

        Error::ErrorCode LoadMapSource(const std::string &mapSourceFile);

        Error::ErrorCode Compile() const;

    private:
        std::string assetsDirectory;
        std::string mapBasename;
        MapAsset map;

        Error::ErrorCode SaveToBuffer(std::vector<uint8_t> &buffer) const;

        LevelMaterialAsset GetMapMaterial(const std::string &path) const;

        [[nodiscard]] static bool SectorFloorCeilingCompare(const std::array<float, 2> &a, const std::array<float, 2> &b);
};
