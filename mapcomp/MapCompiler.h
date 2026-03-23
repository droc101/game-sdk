//
// Created by droc101 on 11/17/25.
//

#pragma once

#include <array>
#include <cstdint>
#include <libassets/asset/DataAsset.h>
#include <libassets/asset/LevelMaterialAsset.h>
#include <libassets/asset/MapAsset.h>
#include <libassets/util/ActorDefinitionManager.h>
#include <libassets/util/Error.h>
#include <libassets/util/SearchPathManager.h>
#include <string>
#include <vector>

class MapCompiler
{
    public:
        struct MapCompilerSettings
        {
            std::string assetsDirectory;
            std::string executableDirectory;
            std::string gameConfigParentDirectory;
            DataAsset gameConfig;
            bool bakeLightsOnCpu;
            bool skipLighting;
        };

        /**
         * Create a map compiler with a given assets directory
         */
        explicit MapCompiler(MapCompilerSettings &settings);

        /**
         * Load a map source file
         * @return Error code
         */
        [[nodiscard]] Error::ErrorCode LoadMapSource(const std::string &mapSourceFile);

        /**
         * Compile the loaded map
         * @return Error code
         */
        [[nodiscard]] Error::ErrorCode Compile() const;

    private:
        MapCompilerSettings settings;
        std::string mapBasename;
        MapAsset map;
        SearchPathManager pathManager;
        ActorDefinitionManager defManager;

        Error::ErrorCode SaveToBuffer(std::vector<uint8_t> &buffer) const;

        [[nodiscard]] LevelMaterialAsset GetMapMaterial(const std::string &path) const;

        [[nodiscard]] static bool SectorFloorCeilingCompare(const std::array<float, 2> &a,
                                                            const std::array<float, 2> &b);
};
