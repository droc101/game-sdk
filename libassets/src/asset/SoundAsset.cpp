//
// Created by droc101 on 7/12/25.
//

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <libassets/asset/Asset.h>
#include <libassets/asset/SoundAsset.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <libassets/util/FileIo.h>
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
    return FileIo::ReadFileToBuffer(filePath, wavData);
}

Error::ErrorCode SoundAsset::Export(const std::string &filePath) const
{
    return FileIo::WriteBufferToFile(filePath, wavData);
}

const std::vector<uint8_t> &SoundAsset::GetData() const
{
    return wavData;
}

size_t SoundAsset::GetDataSize() const
{
    return wavData.size();
}
