//
// Created by droc101 on 7/16/25.
//

#include <libassets/asset/LevelAsset.h>
#include <cassert>
#include <fstream>
#include <ios>
#include <libassets/util/AssetReader.h>
#include <libassets/util/DataReader.h>

Error::ErrorCode LevelAsset::CreateFromAsset(const char *assetPath, LevelAsset &level)
{
    Asset asset;
    const Error::ErrorCode e = AssetReader::LoadFromFile(assetPath, asset);
    if (e != Error::ErrorCode::E_OK) return e;
    if (asset.type != Asset::AssetType::ASSET_TYPE_LEVEL) return Error::ErrorCode::E_INCORRECT_FORMAT;
    if (asset.typeVersion != LEVEL_ASSET_VERSION) return Error::ErrorCode::E_INCORRECT_VERSION;
    level = LevelAsset();
    level.levelData.reserve(asset.reader.TotalSize());
    asset.reader.ReadToBuffer<uint8_t>(level.levelData, asset.reader.TotalSize());
    return Error::ErrorCode::E_OK;
}

void LevelAsset::SaveToBuffer(std::vector<uint8_t> &buffer) const
{
    assert(buffer.empty());
    buffer.resize(levelData.size());
    buffer.insert(buffer.begin(), levelData.begin(), levelData.end());
}

Error::ErrorCode LevelAsset::SaveAsAsset(const char *assetPath) const
{
    std::vector<uint8_t> buffer;
    SaveToBuffer(buffer);
    return AssetReader::SaveToFile(assetPath, buffer, Asset::AssetType::ASSET_TYPE_LEVEL, LEVEL_ASSET_VERSION);
}


Error::ErrorCode LevelAsset::CreateFromBin(const char *binPath, LevelAsset &level)
{
    std::ifstream file(binPath, std::ios::binary | std::ios::ate);
    const std::ifstream::pos_type fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    level = LevelAsset();
    level.levelData.resize(fileSize);
    file.read(reinterpret_cast<char *>(level.levelData.data()), fileSize);
    file.close();
    return Error::ErrorCode::E_OK;
}

Error::ErrorCode LevelAsset::SaveAsBin(const char *binPath) const
{
    std::ofstream file(binPath);
    if (!file) return Error::ErrorCode::E_CANT_OPEN_FILE;
    file.write(reinterpret_cast<const std::ostream::char_type *>(levelData.data()),
               static_cast<std::streamsize>(levelData.size()));
    file.close();
    return Error::ErrorCode::E_OK;
}

const std::vector<uint8_t> &LevelAsset::GetData() const
{
    return levelData;
}

size_t LevelAsset::GetDataSize() const
{
    return levelData.size();
}
