//
// Created by droc101 on 7/16/25.
//

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <libassets/asset/LevelAsset.h>
#include <libassets/type/Asset.h>
#include <libassets/util/AssetReader.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/Error.h>
#include <ostream>
#include <vector>
#include <string>
#include <libassets/type/Actor.h>

void LevelAsset::SaveToBuffer(std::vector<uint8_t> &buffer) const
{
    assert(buffer.empty());
}

Error::ErrorCode LevelAsset::Compile(const char *assetPath) const
{
    std::vector<uint8_t> buffer;
    SaveToBuffer(buffer);
    return AssetReader::SaveToFile(assetPath, buffer, Asset::AssetType::ASSET_TYPE_LEVEL, LEVEL_ASSET_VERSION);
}


Error::ErrorCode LevelAsset::CreateFromMapSrc(const char *mapSrcPath, LevelAsset &level)
{
    std::ifstream file(mapSrcPath, std::ios::binary | std::ios::ate);
    const std::ifstream::pos_type fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    level = LevelAsset();
    file.close();
    return Error::ErrorCode::OK;
}

Error::ErrorCode LevelAsset::SaveAsMapSrc(const char *mapSrcPath) const
{
    std::ofstream file(mapSrcPath);
    if (!file)
    {
        return Error::ErrorCode::CANT_OPEN_FILE;
    }
    file.close();
    return Error::ErrorCode::OK;
}

Actor *LevelAsset::GetActor(const std::string &name)
{
    for (Actor &a: actors)
    {
        if (a.params.contains("name"))
        {
            std::string actorName = a.params.at("name").Get<std::string>("");
            if (!actorName.empty() && actorName == name)
            {
                return &a;
            }
        }
    }

    return nullptr;
}

