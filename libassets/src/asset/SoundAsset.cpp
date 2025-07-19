//
// Created by droc101 on 7/12/25.
//

#include <cassert>
#include <libassets/asset/SoundAsset.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <libassets/util/DataReader.h>
#include <libassets/util/AssetReader.h>

Error::ErrorCode SoundAsset::CreateFromAsset(const char *assetPath, SoundAsset &sound)
{
    Asset asset;
    const Error::ErrorCode e = AssetReader::LoadFromFile(assetPath, asset);
    assert(e == Error::ErrorCode::E_OK);
    assert(asset.type == Asset::AssetType::ASSET_TYPE_WAV);
    const uint32_t payloadSize = asset.reader.TotalSize() - (sizeof(uint32_t) * 4);
    sound.wavData.reserve(payloadSize);
    asset.reader.ReadToBuffer<uint8_t>(sound.wavData, payloadSize);
    return Error::ErrorCode::E_OK;
}

void SoundAsset::SaveToBuffer(std::vector<uint8_t> &buffer) const
{
    assert(buffer.empty());
    buffer.resize(wavData.size());
    buffer.insert(buffer.begin(), wavData.begin(), wavData.end());
    buffer.push_back(wavData.size());
    buffer.push_back(0);
    buffer.push_back(0);
    buffer.push_back(0);
}

Error::ErrorCode SoundAsset::SaveAsAsset(const char *assetPath) const
{
    std::vector<uint8_t> buffer;
    SaveToBuffer(buffer);
    return AssetReader::SaveToFile(assetPath, buffer, Asset::AssetType::ASSET_TYPE_WAV);
}


Error::ErrorCode SoundAsset::CreateFromWAV(const char *wavPath, SoundAsset &sound)
{
    std::ifstream file(wavPath, std::ios::binary | std::ios::ate);
    const std::ifstream::pos_type fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    sound.wavData.resize(fileSize);
    file.read(reinterpret_cast<char *>(sound.wavData.data()), fileSize);
    file.close();
    return Error::ErrorCode::E_OK;
}

Error::ErrorCode SoundAsset::SaveAsWAV(const char *wavPath) const
{
    std::ofstream file(wavPath);
    if (!file)
    {
        return Error::ErrorCode::E_CANT_OPEN_FILE;
    }
    file.write(reinterpret_cast<const std::ostream::char_type *>(wavData.data()),
               static_cast<std::streamsize>(wavData.size()));
    file.close();
    return Error::ErrorCode::E_OK;
}

const std::vector<uint8_t> &SoundAsset::GetData() const
{
    return wavData;
}

size_t SoundAsset::GetDataSize() const
{
    return wavData.size();
}
