//
// Created by droc101 on 10/4/25.
//

#include <cstddef>
#include <cstdint>
#include <libassets/asset/GameConfigAsset.h>
#include <libassets/type/Asset.h>
#include <libassets/util/AssetReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <vector>

Error::ErrorCode GameConfigAsset::CreateFromAsset(const char *assetPath, GameConfigAsset &config)
{
    Asset asset;
    const Error::ErrorCode error = AssetReader::LoadFromFile(assetPath, asset);
    if (error != Error::ErrorCode::OK)
    {
        return error;
    }
    if (asset.type != Asset::AssetType::ASSET_TYPE_GAME_CONFIG)
    {
        return Error::ErrorCode::INCORRECT_FORMAT;
    }
    if (asset.typeVersion != GAME_CONFIG_ASSET_VERSION)
    {
        return Error::ErrorCode::INCORRECT_VERSION;
    }
    config = GameConfigAsset();
    asset.reader.ReadStringWithSize(config.gameTitle);
    asset.reader.ReadStringWithSize(config.gameCopyright);
    config.discordAppId = asset.reader.Read<size_t>();
    return Error::ErrorCode::OK;
}

void GameConfigAsset::SaveToBuffer(std::vector<uint8_t> &buffer) const
{
    DataWriter writer{};
    writer.WriteString(gameTitle);
    writer.WriteString(gameCopyright);
    writer.Write<size_t>(discordAppId);
    writer.CopyToVector(buffer);
}

Error::ErrorCode GameConfigAsset::SaveAsAsset(const char *assetPath) const
{
    std::vector<uint8_t> buffer;
    SaveToBuffer(buffer);
    return AssetReader::SaveToFile(assetPath,
                                   buffer,
                                   Asset::AssetType::ASSET_TYPE_GAME_CONFIG,
                                   GAME_CONFIG_ASSET_VERSION);
}
