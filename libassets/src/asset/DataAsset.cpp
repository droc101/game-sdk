//
// Created by droc101 on 1/20/26.
//

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <ios>
#include <libassets/asset/DataAsset.h>
#include <libassets/type/Asset.h>
#include <libassets/type/Param.h>
#include <libassets/util/AssetReader.h>
#include <libassets/util/Checksum.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <libassets/util/Logger.h>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

Error::ErrorCode DataAsset::CreateFromAsset(const char *assetPath, DataAsset &dataAsset)
{
    Asset asset;
    const Error::ErrorCode error = AssetReader::LoadFromFile(assetPath, asset);
    if (error != Error::ErrorCode::OK)
    {
        return error;
    }
    if (asset.type != Asset::AssetType::ASSET_TYPE_KV_LIST)
    {
        return Error::ErrorCode::INCORRECT_FORMAT;
    }
    if (asset.typeVersion != DATA_ASSET_VERSION)
    {
        return Error::ErrorCode::INCORRECT_VERSION;
    }
    dataAsset = DataAsset();
    dataAsset.data = Param::ReadKvList(asset.reader);
    return Error::ErrorCode::OK;
}

Error::ErrorCode DataAsset::CreateFromKvlFile(const char *kvlPath, DataAsset &dataAsset)
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
    std::vector<uint8_t> data(dataSize);
    fseek(file, 0, SEEK_SET);
    fread(data.data(), 1, dataSize, file);
    fclose(file);
    DataReader reader = DataReader(data);
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
    uint16_t calculatedChecksum = Checksum::Calculate(reader, sizeof(KvlFileHeader));
    if (header.checksum != calculatedChecksum)
    {
        Logger::Error("KvlFile checksum mismatch, expected {}, got {}", header.checksum, calculatedChecksum);
        return Error::ErrorCode::INVALID_BODY;
    }
    dataAsset = DataAsset();
    dataAsset.data = Param::ReadKvList(reader);
    return Error::ErrorCode::OK;
}

void DataAsset::SaveToBuffer(std::vector<uint8_t> &buffer) const
{
    assert(buffer.empty());
    DataWriter w{};
    Param::WriteKvList(w, data);
    w.CopyToVector(buffer);
}

Error::ErrorCode DataAsset::SaveAsAsset(const char *assetPath) const
{
    std::vector<uint8_t> buffer;
    SaveToBuffer(buffer);
    return AssetReader::SaveToFile(assetPath,
                                   buffer,
                                   Asset::AssetType::ASSET_TYPE_KV_LIST,
                                   DATA_ASSET_VERSION,
                                   AssetReader::BEST_COMPRESSION);
}


Error::ErrorCode DataAsset::CreateFromJson(const char *jsonPath, DataAsset &dataAsset)
{
    dataAsset = DataAsset();
    std::ifstream file(jsonPath);
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
    dataAsset.data = Param::KvListFromJson(json);
    return Error::ErrorCode::OK;
}

Error::ErrorCode DataAsset::SaveAsJson(const char *jsonPath) const
{
    std::ofstream file(jsonPath);
    if (!file)
    {
        return Error::ErrorCode::CANT_OPEN_FILE;
    }
    file << Param::GenerateKvListJson(data).dump(); // evil syntax >:(
    file.close();
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
