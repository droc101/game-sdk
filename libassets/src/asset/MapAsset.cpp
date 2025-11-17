//
// Created by droc101 on 7/16/25.
//

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <ios>
#include <libassets/asset/MapAsset.h>
#include <libassets/type/Actor.h>
#include <libassets/type/Asset.h>
#include <libassets/type/Sector.h>
#include <libassets/util/AssetReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <ostream>
#include <ranges>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../../mapcomp/LevelMeshBuilder.h"
#include "libassets/type/WallMaterial.h"

Error::ErrorCode MapAsset::CreateFromMapSrc(const char *mapSrcPath, MapAsset &map)
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

    if (json.value("version", 0) != MAP_JSON_VERSION)
    {
        return Error::ErrorCode::INCORRECT_VERSION;
    }

    map = MapAsset();

    map.discord_rpc_icon_id = json.value("discord_rpc_icon_id", "icon");
    map.discord_rpc_map_name = json.value("discord_rpc_map_name", "Unnamed Map");
    map.sky_texture = json.value("sky_texture", "texture/level/sky_test.gtex");

    const nlohmann::ordered_json jSectors = json.at("sectors");
    for (const nlohmann::ordered_json &sect: jSectors)
    {
        map.sectors.emplace_back(sect);
    }

    const nlohmann::ordered_json jActors = json.at("actors");
    for (const nlohmann::ordered_json &actor: jActors)
    {
        map.actors.emplace_back(actor);
    }
    return Error::ErrorCode::OK;
}

Error::ErrorCode MapAsset::SaveAsMapSrc(const char *mapSrcPath) const
{
    nlohmann::ordered_json src = nlohmann::ordered_json();
    src["version"] = MAP_JSON_VERSION;
    src["discord_rpc_icon_id"] = discord_rpc_icon_id;
    src["discord_rpc_map_name"] = discord_rpc_map_name;
    src["sky_texture"] = sky_texture;
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

Actor *MapAsset::GetActor(const std::string &name)
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
