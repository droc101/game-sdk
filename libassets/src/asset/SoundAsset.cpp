//
// Created by droc101 on 7/12/25.
//

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <libassets/asset/Asset.h>
#include <libassets/asset/SoundAsset.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <ostream>
#include <string>
#include <vector>

SoundAsset::SoundAsset()
{
    Reset();
}

Asset::AssetType SoundAsset::GetAssetType() const
{
    return AssetType::ASSET_TYPE_WAV;
}

uint8_t SoundAsset::GetAssetTypeVersion() const
{
    return SOUND_ASSET_VERSION;
}

void SoundAsset::Reset()
{
    wavData = {};
}

Error::ErrorCode SoundAsset::LoadFromBuffer(DataReader &reader)
{
    Reset();
    wavData.reserve(reader.TotalSize());
    reader.ReadToVector<uint8_t>(wavData, reader.TotalSize());
    return Error::ErrorCode::OK;
}

Error::ErrorCode SoundAsset::SaveToBuffer(DataWriter &writer) const
{
    writer.WriteBuffer(wavData);
    return Error::ErrorCode::OK;
}

Error::ErrorCode SoundAsset::Import(const std::string &filePath)
{
    Reset();
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    const std::ifstream::pos_type fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    wavData.resize(fileSize);
    file.read(reinterpret_cast<char *>(wavData.data()), fileSize);
    file.close();
    return Error::ErrorCode::OK;
}

Error::ErrorCode SoundAsset::Export(const std::string &filePath) const
{
    std::ofstream file(filePath);
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
