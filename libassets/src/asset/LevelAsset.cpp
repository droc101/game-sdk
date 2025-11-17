//
// Created by droc101 on 7/16/25.
//

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <ios>
#include <libassets/asset/LevelAsset.h>
#include <libassets/type/Actor.h>
#include <libassets/type/Asset.h>
#include <libassets/type/Sector.h>
#include <libassets/util/AssetReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <ostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../type/LevelMeshBuilder.h"
#include "libassets/type/WallMaterial.h"

Error::ErrorCode LevelAsset::SaveToBuffer(std::vector<uint8_t> &buffer) const
{
    assert(buffer.empty());

    for (const Sector &sector: sectors)
    {
        if (!sector.IsValid())
        {
            printf("Compile Error: Invalid Sector\n");
            return Error::ErrorCode::INCORRECT_FORMAT;
        }
    }

    DataWriter writer = DataWriter();
    writer.Write<size_t>(actors.size());

    size_t numPlayerActors = 0;
    for (const Actor &actor: actors)
    {
        actor.Write(writer);
        if (actor.className == "player")
        {
            numPlayerActors++;
        }

        bool insideSector = false;
        for (const Sector &sector: sectors)
        {
            const bool insideHorizontally = sector.ContainsPoint({actor.position[0], actor.position[2]});
            const bool insideVertically = actor.position[1] >= sector.floorHeight && actor.position[1] <= sector.ceilingHeight;
            if (insideHorizontally && insideVertically)
            {
                insideSector = true;
                break;
            }
        }
        if (!insideSector)
        {
            printf("Compile Warning: Found an actor that is not inside any sector.\n");
        }
    }

    if (numPlayerActors == 0)
    {
        printf("Compile Warning: There is no player actor, the player will spawn at the origin.\n");
    } else if (numPlayerActors != 1)
    {
        printf("Compile Warning: Multiple player actors are present, only one will function.\n");
    }

    std::unordered_map<std::string, LevelMeshBuilder> meshBuilders{};
    for (const Sector &sector: sectors)
    {
        for (size_t i = 0; i < sector.points.size(); i++)
        {
            const WallMaterial &mat = sector.wallMaterials.at(i);
            if (!meshBuilders.contains(mat.material))
            {
                meshBuilders[mat.material] = LevelMeshBuilder();
            }
            meshBuilders.at(mat.material).AddWall(sector, i);
        }
        if (!meshBuilders.contains(sector.ceilingMaterial.material))
        {
            meshBuilders[sector.ceilingMaterial.material] = LevelMeshBuilder();
        }
        meshBuilders.at(sector.ceilingMaterial.material).AddCeiling(sector);
        if (!meshBuilders.contains(sector.floorMaterial.material))
        {
            meshBuilders[sector.floorMaterial.material] = LevelMeshBuilder();
        }
        meshBuilders.at(sector.floorMaterial.material).AddFloor(sector);
        // TODO collision
        // TODO check material for compileInvisible (how get material???)
    }

    writer.Write<size_t>(meshBuilders.size());
    for (const LevelMeshBuilder &builder: meshBuilders | std::views::values)
    {
        builder.Write(writer);
    }

    writer.CopyToVector(buffer);
    return Error::ErrorCode::OK;
}

Error::ErrorCode LevelAsset::Compile(const char *assetPath) const
{
    std::vector<uint8_t> buffer;
    const Error::ErrorCode e = SaveToBuffer(buffer);
    if (e != Error::ErrorCode::OK)
    {
        return e;
    }
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
