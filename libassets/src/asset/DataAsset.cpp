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

const Error &DataAsset::LoadFromBuffer(DataReader &reader)
{
    Reset();
    data = Param::ReadKvList(reader);
    return Errors::OK;
}

const Error &DataAsset::SaveToBuffer(DataWriter &writer) const
{
    if (writer.GetBufferSize() != 0)
    {
        return Error("Buffer not empty");
    }
    Param::WriteKvList(writer, data);
    return Errors::OK;
}

const Error &DataAsset::Import(const std::string &filePath)
{
    std::string jsonString;
    const Error &readError = FileIo::ReadFileToString(filePath, jsonString);
    if (readError.Failed())
    {
        return readError;
    }
    const nlohmann::json json = nlohmann::json::parse(jsonString);
    if (json.is_discarded())
    {
        return Errors::INCORRECT_FORMAT;
    }
    Reset();
    data = Param::KvListFromJson(json);
    return Errors::OK;
}

const Error &DataAsset::Export(const std::string &filePath) const
{
    const std::string json = Param::GenerateKvListJson(data).dump();
    return FileIo::WriteStringToFile(filePath, json);
}

const Error &DataAsset::CreateFromKvlFile(const std::string &kvlPath)
{
    DataReader reader;
    const Error &readError = FileIo::CreateFileDataReader(kvlPath, reader);
    if (readError.Failed())
    {
        return readError;
    }
    KvlFileHeader header{};
    header.magic = reader.Read<uint32_t>();
    header.version = reader.Read<uint16_t>();
    header.checksum = reader.Read<uint16_t>();
    if (header.magic != KVL_MAGIC)
    {
        return Errors::INVALID_HEADER;
    }
    if (header.version != KVL_VERSION)
    {
        return Errors::INCORRECT_VERSION;
    }
    const uint16_t calculatedChecksum = Checksum::Calculate(reader, sizeof(KvlFileHeader));
    if (header.checksum != calculatedChecksum)
    {
        Logger::Error("KvlFile checksum mismatch, expected {}, got {}", header.checksum, calculatedChecksum);
        return Errors::INVALID_BODY;
    }
    Reset();
    data = Param::ReadKvList(reader);
    return Errors::OK;
}

const Error &DataAsset::SaveAsKvlFile(const std::string &kvlFile) const
{
    std::ofstream file;
    const Error &openError = FileIo::OpenFileW(kvlFile, file);
    if (openError.Failed())
    {
        return openError;
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
    return Errors::OK;
}
