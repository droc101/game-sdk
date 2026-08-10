//
// Created by droc101 on 8/3/26.
//

#ifndef GAME_SDK_ASSET_H
#define GAME_SDK_ASSET_H

#include <cstdint>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <string>

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
            ASSET_TYPE_FONT = 5,
            ASSET_TYPE_LEVEL_MATERIAL = 7,
            ASSET_TYPE_KV_LIST = 8,
        };

        virtual ~Asset() = default;

        virtual void Reset() = 0;

        [[nodiscard]] virtual Error::ErrorCode LoadFromBuffer(DataReader &reader);
        [[nodiscard]] virtual Error::ErrorCode SaveToBuffer(DataWriter &writer) const;

        [[nodiscard]] virtual Error::ErrorCode LoadFromAsset(const std::string &filePath);
        [[nodiscard]] virtual Error::ErrorCode SaveToAsset(const std::string &filePath) const;

        [[nodiscard]] virtual Error::ErrorCode Import(const std::string &filePath);
        [[nodiscard]] virtual Error::ErrorCode Export(const std::string &filePath) const;

        [[nodiscard]] virtual AssetType GetAssetType() const = 0;
        [[nodiscard]] virtual uint8_t GetAssetTypeVersion() const = 0;
};

#endif //GAME_SDK_ASSET_H
