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

const Error &Asset::LoadFromAsset(const std::string &filePath)
{
    AssetContainer asset;
    const Error &error = AssetContainer::LoadFromFile(filePath, asset);
    if (error.Failed())
    {
        return error;
    }
    if (asset.type != GetAssetType())
    {
        return Errors::INCORRECT_FORMAT;
    }
    if (asset.typeVersion != GetAssetTypeVersion())
    {
        return Errors::INCORRECT_VERSION;
    }
    return LoadFromBuffer(asset.reader);
}

const Error &Asset::SaveToAsset(const std::string &filePath) const
{
    DataWriter writer{};
    const Error &writeError = SaveToBuffer(writer);
    if (writeError.Failed())
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

const Error &Asset::LoadFromBuffer(DataReader &reader)
{
    (void)reader;
    return Errors::NOT_IMPLEMENTED;
}

const Error &Asset::SaveToBuffer(DataWriter &writer) const
{
    (void)writer;
    return Errors::NOT_IMPLEMENTED;
}

const Error &Asset::Import(const std::string &filePath)
{
    (void)filePath;
    return Errors::NOT_IMPLEMENTED;
}

const Error &Asset::Export(const std::string &filePath) const
{
    (void)filePath;
    return Errors::NOT_IMPLEMENTED;
}
