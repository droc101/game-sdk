//
// Created by droc101 on 7/16/25.
//

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <libassets/asset/LevelAsset.h>
#include <libassets/type/Actor.h>
#include <libassets/type/Asset.h>
#include <libassets/util/AssetReader.h>
#include <libassets/util/Error.h>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

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
    std::ostringstream ss;
    file.seekg(0, std::ios::beg);
    ss << file.rdbuf();
    const std::string j = ss.str();
    const nlohmann::ordered_json json = nlohmann::ordered_json::parse(j);
    if (json.is_discarded())
    {
        file.close();
        // printf("File %s is not valid JSON\n", path.c_str());
        return Error::ErrorCode::INCORRECT_FORMAT;
    }
    file.close();

    if (json.value("version", 0) != LEVEL_JSON_VERSION)
    {
        return Error::ErrorCode::INCORRECT_VERSION;
    }

    level = LevelAsset();

    const nlohmann::ordered_json jSectors = json.at("sectors");
    for (const nlohmann::ordered_json &sect: jSectors)
    {
        level.sectors.emplace_back(sect);
    }

    const nlohmann::ordered_json jActors = json.at("actors");
    for (const nlohmann::ordered_json &actor: jActors)
    {
        level.actors.emplace_back(actor);
    }
    return Error::ErrorCode::OK;
}

Error::ErrorCode LevelAsset::SaveAsMapSrc(const char *mapSrcPath) const
{
    nlohmann::ordered_json src = nlohmann::ordered_json();
    src["version"] = LEVEL_JSON_VERSION;
    src["sectors"] = nlohmann::ordered_json::array();
    src["actors"] = nlohmann::ordered_json::array();
    for (const Sector &sector: sectors)
    {
        src["sectors"].push_back(sector.GenerateJson());
    }
    for (const Actor &actor: actors)
    {
        src["actors"].push_back(actor.GenerateJson());
    }

    std::ofstream file(mapSrcPath);
    if (!file)
    {
        return Error::ErrorCode::CANT_OPEN_FILE;
    }
    file << src;
    file.close();
    return Error::ErrorCode::OK;
}

Actor *LevelAsset::GetActor(const std::string &name)
{
    for (Actor &a: actors)
    {
        if (a.params.contains("name"))
        {
            const std::string actorName = a.params.at("name").Get<std::string>("");
            if (!actorName.empty() && actorName == name)
            {
                return &a;
            }
        }
    }

    return nullptr;
}
