//
// Created by droc101 on 7/16/25.
//

#include <libassets/asset/LevelAsset.h>
#include <cassert>
#include <fstream>
#include <ios>
#include <libassets/util/AssetReader.h>
#include <libassets/util/DataReader.h>

LevelAsset LevelAsset::CreateFromAsset(const char *assetPath)
{
    DataReader reader;
    [[maybe_unused]] const AssetReader::AssetType assetType = AssetReader::LoadFromFile(assetPath, reader);
    assert(assetType == AssetReader::AssetType::ASSET_TYPE_LEVEL);
    LevelAsset mus;
    mus.levelData.reserve(reader.TotalSize());
    reader.ReadToBuffer<uint8_t>(mus.levelData, reader.TotalSize());
    return mus;
}

void LevelAsset::SaveToBuffer(std::vector<uint8_t> &buffer) const
{
    assert(buffer.empty());
    buffer.resize(levelData.size());
    buffer.insert(buffer.begin(), levelData.begin(), levelData.end());
}

void LevelAsset::SaveAsAsset(const char *assetPath) const
{
    std::vector<uint8_t> buffer;
    SaveToBuffer(buffer);
    AssetReader::SaveToFile(assetPath, buffer, AssetReader::AssetType::ASSET_TYPE_LEVEL);
}


LevelAsset LevelAsset::CreateFromBin(const char *binPath)
{
    std::ifstream file(binPath, std::ios::binary | std::ios::ate);
    const std::ifstream::pos_type fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    LevelAsset mus;
    mus.levelData.resize(fileSize);
    file.read(reinterpret_cast<char*>(mus.levelData.data()), fileSize);
    file.close();
    return mus;
}

void LevelAsset::SaveAsBin(const char *binPath) const
{
    std::ofstream file(binPath);
    file.write(reinterpret_cast<const std::ostream::char_type *>(levelData.data()),
               static_cast<std::streamsize>(levelData.size()));
    file.close();
}

const std::vector<uint8_t> &LevelAsset::GetData() const
{
    return levelData;
}

size_t LevelAsset::GetDataSize() const
{
    return levelData.size();
}
