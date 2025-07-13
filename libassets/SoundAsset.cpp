//
// Created by droc101 on 7/12/25.
//

#include "include/libassets/SoundAsset.h"
#include <filesystem>
#include <fstream>
#include <iterator>
#include "include/libassets/DataReader.h"

SoundAsset SoundAsset::CreateFromAsset(const char *assetPath)
{
    DataReader reader;
    [[maybe_unused]] const AssetReader::AssetType assetType = AssetReader::LoadFromFile(assetPath, reader);
    assert(assetType == AssetReader::AssetType::ASSET_TYPE_WAV);
    SoundAsset mus;
    const uint32_t payloadSize = reader.TotalSize() - (sizeof(uint32_t) * 4);
    mus.wavData.reserve(payloadSize);
    reader.ReadToBuffer<uint8_t>(mus.wavData, payloadSize);
    return mus;
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

void SoundAsset::SaveAsAsset(const char *assetPath) const
{
    std::vector<uint8_t> buffer;
    SaveToBuffer(buffer);
    AssetReader::SaveToFile(assetPath, buffer, AssetReader::AssetType::ASSET_TYPE_WAV);
}


SoundAsset SoundAsset::CreateFromWAV(const char *wavPath)
{
    std::ifstream file(wavPath, std::ios::binary | std::ios::ate);
    const std::ifstream::pos_type fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    SoundAsset mus;
    mus.wavData.resize(fileSize);
    file.read(reinterpret_cast<char*>(mus.wavData.data()), fileSize);
    file.close();
    return mus;
}

void SoundAsset::SaveAsWAV(const char *wavPath) const
{
    std::ofstream file(wavPath);
    file.write(reinterpret_cast<const std::ostream::char_type *>(wavData.data()), static_cast<std::streamsize>(wavData.size()));
    file.close();
}

std::vector<uint8_t> SoundAsset::GetData() const
{
    return wavData;
}

size_t SoundAsset::GetDataSize() const
{
    return wavData.size();
}

