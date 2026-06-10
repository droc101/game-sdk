//
// Created by droc101 on 7/16/25.
//

#pragma once

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

        /// The sectors in this map
        std::vector<Sector> sectors{};
        /// The actors in this map
        std::vector<Actor> actors{};

        /// The ID of the icon for this map in Discord Rich Presence
        std::string discordRpcIconId = "logo";
        /// The display name of this map in Discord Rick Presence
        std::string discordRpcMapName = "Unnamed Map";

        /// Whether this map has a sky
        bool hasSky = true;
        /// The texture of the sky in this level
        std::string skyTexture = "texture/level/sky_test.gtex";

        uint8_t lightCubeLuxelsPerUnit = 4;

        static constexpr uint8_t MAP_ASSET_VERSION = 1;
        static constexpr uint8_t MAP_JSON_VERSION = 1;

        static constexpr float MAP_MAX_HALF_EXTENTS = 512; // 1024 unit wide map

        /**
         * Create a MapAsset from a JSON source file
         * @param mapSrcPath The path to the source file
         * @param map The MapAsset to populate
         */
        [[nodiscard]] static Error::ErrorCode CreateFromMapSrc(const char *mapSrcPath, MapAsset &map);

        /**
         * Save this MapAsset as JSON
         * @param mapSrcPath The path to the JSON file
         */
        [[nodiscard]] Error::ErrorCode SaveAsMapSrc(const char *mapSrcPath) const;

        /**
         * Get an actor by name
         * @param name The actor name
         * @note This will only return the first actor with this name
         */
        Actor *GetActor(const std::string &name);

        /**
         * Get a list of actor names in this level, without repeats
         */
        [[nodiscard]] std::vector<std::string> GetUniqueActorNames() const;
};
