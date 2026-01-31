//
// Created by droc101 on 1/20/26.
//

#include <cassert>
#include <cstdint>
#include <fstream>
#include <ios>
#include <libassets/asset/DataAsset.h>
#include <libassets/type/Asset.h>
#include <libassets/type/Param.h>
#include <libassets/util/AssetReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
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
    if (asset.type != Asset::AssetType::ASSET_TYPE_WAV)
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
    return AssetReader::SaveToFile(assetPath, buffer, Asset::AssetType::ASSET_TYPE_WAV, DATA_ASSET_VERSION);
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
