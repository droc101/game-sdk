//
// Created by droc101 on 7/18/25.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <libassets/util/DataReader.h>
#include "libassets/libassets.h"

class Asset
{
    public:
        enum class AssetType : uint8_t
        {
            ASSET_TYPE_TEXTURE = 0,
            ASSET_TYPE_WAV = 1,
            ASSET_TYPE_LEVEL = 2,
            ASSET_TYPE_SHADER = 3,
            ASSET_TYPE_MODEL = 4,
            ASSET_TYPE_FONT = 5
        };

        static constexpr uint8_t ASSET_CONTAINER_VERSION = 2;

        static constexpr uint32_t ASSET_CONTAINER_MAGIC = 0x454D4147; // "GAME"

        static constexpr size_t ASSET_HEADER_SIZE = sizeof(uint32_t) + (sizeof(uint8_t) * 3) + (sizeof(size_t) * 2);

        Asset() = default;



        uint8_t containerVersion{};

        AssetType type{};

        uint8_t typeVersion{};

        size_t size{};

        DataReader reader{};
};
