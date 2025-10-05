//
// Created by droc101 on 10/4/25.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <libassets/util/Error.h>
#include <string>
#include <vector>

class GameConfigAsset final
{
    public:
        GameConfigAsset() = default;

        [[nodiscard]] static Error::ErrorCode CreateFromAsset(const char *assetPath, GameConfigAsset &config);

        [[nodiscard]] Error::ErrorCode SaveAsAsset(const char *assetPath) const;

        static constexpr uint8_t GAME_CONFIG_ASSET_VERSION = 1;

        std::string gameTitle{};
        std::string gameCopyright{};

        size_t discordAppId = 0;
    private:
        void SaveToBuffer(std::vector<uint8_t> &buffer) const;
};
