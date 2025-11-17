//
// Created by droc101 on 7/16/25.
//

#pragma once

#include <array>
#include <cstdint>
#include <libassets/type/Actor.h>
#include <libassets/type/Sector.h>
#include <libassets/util/Error.h>
#include <string>
#include <vector>

class LevelAsset final
{
    public:
        LevelAsset() = default;

        [[nodiscard]] static Error::ErrorCode CreateFromMapSrc(const char *mapSrcPath, LevelAsset &level);

        [[nodiscard]] Error::ErrorCode SaveAsMapSrc(const char *mapSrcPath) const;

        [[nodiscard]] Error::ErrorCode Compile(const char *assetPath) const;

        static constexpr uint8_t LEVEL_ASSET_VERSION = 1;
        static constexpr uint8_t LEVEL_JSON_VERSION = 1;

        static constexpr float LEVEL_MAX_HALF_EXTENTS = 512; // 1024 unit wide level

        std::vector<Sector> sectors{};
        std::vector<Actor> actors{};
        // TODO: player, music, sky, and fog will be controlled by actors

        Actor *GetActor(const std::string &name);

    private:
        Error::ErrorCode SaveToBuffer(std::vector<uint8_t> &buffer) const;
};
