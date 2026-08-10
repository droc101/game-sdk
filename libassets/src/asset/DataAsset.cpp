//
// Created by droc101 on 1/20/26.
//

#include <cassert>
#include <cstdint>
#include <fstream>
#include <libassets/asset/Asset.h>
#include <libassets/asset/DataAsset.h>
#include <libassets/type/Param.h>
#include <libassets/util/Checksum.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <libassets/util/FileIo.h>
#include <libassets/util/Logger.h>
#include <ostream>
#include <string>
#include <vector>

DataAsset::DataAsset()
{
    Reset();
}

Asset::AssetType DataAsset::GetAssetType() const
{
    return AssetType::ASSET_TYPE_KV_LIST;
}

uint8_t DataAsset::GetAssetTypeVersion() const
{
    return DATA_ASSET_VERSION;
}

void DataAsset::Reset()
{
    data = {};
}

Error::ErrorCode DataAsset::LoadFromBuffer(DataReader &reader)
{
    Reset();
    data = Param::ReadKvList(reader);
    return Error::ErrorCode::OK;
}

Error::ErrorCode DataAsset::SaveToBuffer(DataWriter &writer) const
{
    if (writer.GetBufferSize() != 0)
    {
        return Error::ErrorCode::UNKNOWN;
    }
    Param::WriteKvList(writer, data);
    return Error::ErrorCode::OK;
}

Error::ErrorCode DataAsset::Import(const std::string &filePath)
{
    std::string jsonString;
    const Error::ErrorCode readError = FileIo::ReadFileToString(filePath, jsonString);
    if (readError != Error::ErrorCode::OK)
    {
        return readError;
    }
    const nlohmann::json json = nlohmann::json::parse(jsonString);
    if (json.is_discarded())
    {
        return Error::ErrorCode::INCORRECT_FORMAT;
    }
    Reset();
    data = Param::KvListFromJson(json);
    return Error::ErrorCode::OK;
}

Error::ErrorCode DataAsset::Export(const std::string &filePath) const
{
    const std::string json = Param::GenerateKvListJson(data).dump();
    const Error::ErrorCode writeError = FileIo::WriteStringToFile(filePath, json);
    return writeError;
}

Error::ErrorCode DataAsset::CreateFromKvlFile(const std::string &kvlPath)
{
    DataReader reader;
    const Error::ErrorCode readError = FileIo::CreateFileDataReader(kvlPath, reader);
    if (readError != Error::ErrorCode::OK)
    {
        return readError;
    }
    KvlFileHeader header{};
    header.magic = reader.Read<uint32_t>();
    header.version = reader.Read<uint16_t>();
    header.checksum = reader.Read<uint16_t>();
    if (header.magic != KVL_MAGIC)
    {
        return Error::ErrorCode::INVALID_HEADER;
    }
    if (header.version != KVL_VERSION)
    {
        return Error::ErrorCode::INCORRECT_VERSION;
    }
    const uint16_t calculatedChecksum = Checksum::Calculate(reader, sizeof(KvlFileHeader));
    if (header.checksum != calculatedChecksum)
    {
        Logger::Error("KvlFile checksum mismatch, expected {}, got {}", header.checksum, calculatedChecksum);
        return Error::ErrorCode::INVALID_BODY;
    }
    Reset();
    data = Param::ReadKvList(reader);
    return Error::ErrorCode::OK;
}

Error::ErrorCode DataAsset::SaveAsKvlFile(const std::string &kvlFile) const
{
    std::ofstream file;
    const Error::ErrorCode openError = FileIo::OpenFileW(kvlFile, file);
    if (openError != Error::ErrorCode::OK)
    {
        return Error::ErrorCode::OK;
    }
    DataWriter writer{};
    Param::WriteKvList(writer, data);
    const KvlFileHeader header = {
        .magic = KVL_MAGIC,
        .version = KVL_VERSION,
        .checksum = Checksum::Calculate(writer),
    };
    file.write(reinterpret_cast<const std::ostream::char_type *>(&header), sizeof(KvlFileHeader));
    std::vector<uint8_t> bytes{};
    writer.CopyToVector(bytes);
    file.write(reinterpret_cast<const std::ostream::char_type *>(bytes.data()), bytes.size());
    file.close();
    return Error::ErrorCode::OK;
}
