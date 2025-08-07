//
// Created by droc101 on 7/12/25.
//

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <libassets/asset/SoundAsset.h>
#include <libassets/util/Asset.h>
#include <libassets/util/AssetReader.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/Error.h>
#include <ostream>
#include <vector>

Error::ErrorCode SoundAsset::CreateFromAsset(const char *assetPath, SoundAsset &sound)
{
    Asset asset;
    const Error::ErrorCode error = AssetReader::LoadFromFile(assetPath, asset);
    if (error != Error::ErrorCode::OK)
    {
        return error;
    }
    if (asset.type != Asset::AssetType::ASSET_TYPE_WAV)
    {
        return Error::ErrorCode::INCORRECT_FORMAT;
    }
    if (asset.typeVersion != SOUND_ASSET_VERSION)
    {
        return Error::ErrorCode::INCORRECT_VERSION;
    }
    sound = SoundAsset();
    sound.wavData.reserve(asset.reader.TotalSize());
    asset.reader.ReadToBuffer<uint8_t>(sound.wavData, asset.reader.TotalSize());
    return Error::ErrorCode::OK;
}

void SoundAsset::SaveToBuffer(std::vector<uint8_t> &buffer) const
{
    assert(buffer.empty());
    buffer.resize(wavData.size());
    buffer.insert(buffer.begin(), wavData.begin(), wavData.end());
}

Error::ErrorCode SoundAsset::SaveAsAsset(const char *assetPath) const
{
    std::vector<uint8_t> buffer;
    SaveToBuffer(buffer);
    return AssetReader::SaveToFile(assetPath, buffer, Asset::AssetType::ASSET_TYPE_WAV, SOUND_ASSET_VERSION);
}


Error::ErrorCode SoundAsset::CreateFromWAV(const char *wavPath, SoundAsset &sound)
{
    sound = SoundAsset();
    std::ifstream file(wavPath, std::ios::binary | std::ios::ate);
    const std::ifstream::pos_type fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    sound.wavData.resize(fileSize);
    file.read(reinterpret_cast<char *>(sound.wavData.data()), fileSize);
    file.close();
    return Error::ErrorCode::OK;
}

Error::ErrorCode SoundAsset::SaveAsWAV(const char *wavPath) const
{
    std::ofstream file(wavPath);
    if (!file)
    {
        return Error::ErrorCode::CANT_OPEN_FILE;
    }
    file.write(reinterpret_cast<const std::ostream::char_type *>(wavData.data()),
               static_cast<std::streamsize>(wavData.size()));
    file.close();
    return Error::ErrorCode::OK;
}

const std::vector<uint8_t> &SoundAsset::GetData() const
{
    return wavData;
}

size_t SoundAsset::GetDataSize() const
{
    return wavData.size();
}
