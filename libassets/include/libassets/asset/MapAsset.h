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

class MapAsset final
{
    public:
        MapAsset() = default;

        [[nodiscard]] static Error::ErrorCode CreateFromMapSrc(const char *mapSrcPath, MapAsset &map);

        [[nodiscard]] Error::ErrorCode SaveAsMapSrc(const char *mapSrcPath) const;

        static constexpr uint8_t MAP_ASSET_VERSION = 1;
        static constexpr uint8_t MAP_JSON_VERSION = 1;

        static constexpr float MAP_MAX_HALF_EXTENTS = 512; // 1024 unit wide map

        std::vector<Sector> sectors{};
        std::vector<Actor> actors{};
        std::string discord_rpc_icon_id = "logo";
        std::string discord_rpc_map_name = "Unnamed Map";
        std::string sky_texture = "texture/level/sky_test.gtex";
        // TODO: music and fog will be controlled by actors

        Actor *GetActor(const std::string &name);

        [[nodiscard]] std::vector<std::string> GetUniqueActorNames() const;
};
