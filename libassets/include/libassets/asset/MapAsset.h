//
// Created by droc101 on 7/16/25.
//

#pragma once

#include <cstdint>
#include <libassets/asset/Asset.h>
#include <libassets/type/Actor.h>
#include <libassets/type/Brush.h>
#include <libassets/util/Error.h>
#include <string>
#include <vector>

class MapAsset final: public Asset
{
    public:
        MapAsset();

        /// The brushes in this map
        std::vector<Brush> brushes;
        /// The actors in this map
        std::vector<Actor> actors;

        /// The ID of the icon for this map in Discord Rich Presence
        std::string discordRpcIconId;
        /// The display name of this map in Discord Rich Presence
        std::string discordRpcMapName;

        /// Whether this map has a sky
        bool hasSky;
        /// The texture of the sky in this level
        std::string skyTexture;

        uint8_t lightCubeLuxelsPerUnit;

        static constexpr uint8_t MAP_ASSET_VERSION = 1;
        static constexpr uint8_t MAP_JSON_VERSION = 1;

        static constexpr float MAP_MAX_HALF_EXTENTS = 8192;

        void Reset() override;

        [[nodiscard]] Error::ErrorCode Import(const std::string &filePath) override;
        [[nodiscard]] Error::ErrorCode Export(const std::string &filePath) const override;

        [[nodiscard]] AssetType GetAssetType() const override;
        [[nodiscard]] uint8_t GetAssetTypeVersion() const override;

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
