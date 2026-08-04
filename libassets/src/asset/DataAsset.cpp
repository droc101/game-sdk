//
// Created by droc101 on 1/20/26.
//

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <ios>
#include <libassets/asset/Asset.h>
#include <libassets/asset/DataAsset.h>
#include <libassets/type/Param.h>
#include <libassets/util/Checksum.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <libassets/util/Logger.h>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

Asset::AssetType DataAsset::GetAssetType() const
{
    return AssetType::ASSET_TYPE_KV_LIST;
}

uint8_t DataAsset::GetAssetTypeVersion() const
{
    return DATA_ASSET_VERSION;
}

Error::ErrorCode DataAsset::LoadFromBuffer(DataReader &reader)
{
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
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        return Error::ErrorCode::CANT_OPEN_FILE;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    const std::string j = ss.str();
    const nlohmann::json json = nlohmann::json::parse(j);
    if (json.is_discarded())
    {
        file.close();
        return Error::ErrorCode::INCORRECT_FORMAT;
    }
    data = Param::KvListFromJson(json);
    return Error::ErrorCode::OK;
}

Error::ErrorCode DataAsset::Export(const std::string &filePath) const
{
    std::ofstream file(filePath);
    if (!file)
    {
        return Error::ErrorCode::CANT_OPEN_FILE;
    }
    file << Param::GenerateKvListJson(data).dump(); // evil syntax >:(
    file.close();
    return Error::ErrorCode::OK;
}

Error::ErrorCode DataAsset::CreateFromKvlFile(const char *kvlPath)
{
    std::FILE *file = std::fopen(kvlPath, "rb");
    if (file == nullptr)
    {
        return Error::ErrorCode::FILE_NOT_FOUND;
    }
    fseek(file, 0, SEEK_END);
    const size_t dataSize = ftell(file);
    if (dataSize < sizeof(KvlFileHeader))
    {
        return Error::ErrorCode::INVALID_HEADER;
    }
    std::vector<uint8_t> dataBuffer(dataSize);
    fseek(file, 0, SEEK_SET);
    fread(dataBuffer.data(), 1, dataSize, file);
    fclose(file);
    DataReader reader = DataReader(dataBuffer);
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
    data = Param::ReadKvList(reader);
    return Error::ErrorCode::OK;
}

Error::ErrorCode DataAsset::SaveAsKvlFile(const char *kvlFile) const
{
    FILE *file = fopen(kvlFile, "wb");
    if (file == nullptr)
    {
        Logger::Error("Unable to open file for writing");
        return Error::ErrorCode::CANT_OPEN_FILE;
    }
    DataWriter writer{};
    Param::WriteKvList(writer, data);
    const KvlFileHeader header = {
        .magic = KVL_MAGIC,
        .version = KVL_VERSION,
        .checksum = Checksum::Calculate(writer),
    };
    fwrite(&header, sizeof(KvlFileHeader), 1, file);
    std::vector<uint8_t> bytes{};
    writer.CopyToVector(bytes);
    fwrite(bytes.data(), 1, bytes.size(), file);
    fclose(file);
    return Error::ErrorCode::OK;
}
