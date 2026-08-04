//
// Created by droc101 on 8/4/26.
//

#include <cstdint>
#include <libassets/asset/Asset.h>
#include <libassets/util/AssetContainer.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <string>
#include <vector>

Error::ErrorCode Asset::LoadFromAsset(const std::string &filePath)
{
    AssetContainer asset;
    const Error::ErrorCode error = AssetContainer::LoadFromFile(filePath, asset);
    if (error != Error::ErrorCode::OK)
    {
        return error;
    }
    if (asset.type != GetAssetType())
    {
        return Error::ErrorCode::INCORRECT_FORMAT;
    }
    if (asset.typeVersion != GetAssetTypeVersion())
    {
        return Error::ErrorCode::INCORRECT_VERSION;
    }
    return LoadFromBuffer(asset.reader);
}

Error::ErrorCode Asset::SaveToAsset(const std::string &filePath) const
{
    DataWriter writer{};
    const Error::ErrorCode writeError = SaveToBuffer(writer);
    if (writeError != Error::ErrorCode::OK)
    {
        return writeError;
    }
    std::vector<uint8_t> data{};
    data.reserve(writer.GetBufferSize());
    writer.CopyToVector(data);
    return AssetContainer::SaveToFile(filePath,
                                      data,
                                      GetAssetType(),
                                      GetAssetTypeVersion(),
                                      AssetContainer::BEST_COMPRESSION);
}

Error::ErrorCode Asset::LoadFromBuffer(DataReader &reader)
{
    (void)reader;
    return Error::ErrorCode::NOT_IMPLEMENTED;
}

Error::ErrorCode Asset::SaveToBuffer(DataWriter &writer) const
{
    (void)writer;
    return Error::ErrorCode::NOT_IMPLEMENTED;
}

Error::ErrorCode Asset::Import(const std::string &filePath)
{
    (void)filePath;
    return Error::ErrorCode::NOT_IMPLEMENTED;
}

Error::ErrorCode Asset::Export(const std::string &filePath) const
{
    (void)filePath;
    return Error::ErrorCode::NOT_IMPLEMENTED;
}
