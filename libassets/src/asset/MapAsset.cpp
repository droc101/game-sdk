//
// Created by droc101 on 7/16/25.
//

#include <algorithm>
#include <cstdint>
#include <libassets/asset/Asset.h>
#include <libassets/asset/MapAsset.h>
#include <libassets/type/Actor.h>
#include <libassets/type/Brush.h>
#include <libassets/type/Param.h>
#include <libassets/util/Error.h>
#include <libassets/util/FileIo.h>
#include <string>
#include <unordered_map>
#include <vector>

MapAsset::MapAsset()
{
    Reset();
}

Asset::AssetType MapAsset::GetAssetType() const
{
    return AssetType::ASSET_TYPE_LEVEL;
}

uint8_t MapAsset::GetAssetTypeVersion() const
{
    return MAP_ASSET_VERSION;
}

void MapAsset::Reset()
{
    brushes = {};
    actors = {};
    discordRpcIconId = "logo";
    discordRpcMapName = "Unnamed Map";
    hasSky = true;
    skyTexture = "texture/level/sky_test.gtex";
    lightCubeLuxelsPerUnit = 4;
}

Error::ErrorCode MapAsset::Import(const std::string &filePath)
{
    std::string jsonString;
    const Error::ErrorCode readError = FileIo::ReadFileToString(filePath, jsonString);
    if (readError != Error::ErrorCode::OK)
    {
        return readError;
    }
    const nlohmann::ordered_json json = nlohmann::ordered_json::parse(jsonString);
    if (json.is_discarded())
    {
        return Error::ErrorCode::INCORRECT_FORMAT;
    }
    if (json.value("version", 0) != MAP_JSON_VERSION)
    {
        return Error::ErrorCode::INCORRECT_VERSION;
    }

    Reset();

    discordRpcIconId = json.value("discord_rpc_icon_id", "icon");
    discordRpcMapName = json.value("discord_rpc_map_name", "Unnamed Map");

    hasSky = json.value("has_sky", true);
    skyTexture = json.value("sky_texture", "texture/level/sky_test.gtex");

    lightCubeLuxelsPerUnit = json.value("light_cube_luxels_per_unit", 4);

    const nlohmann::ordered_json &jsonBrushes = json.value("brushes", nlohmann::ordered_json());
    for (const nlohmann::ordered_json &brush: jsonBrushes)
    {
        brushes.emplace_back(brush);
    }

    const nlohmann::ordered_json &jsonActors = json.at("actors");
    for (const nlohmann::ordered_json &actor: jsonActors)
    {
        actors.emplace_back(actor);
    }
    return Error::ErrorCode::OK;
}

Error::ErrorCode MapAsset::Export(const std::string &filePath) const
{
    nlohmann::ordered_json src = nlohmann::ordered_json();
    src["version"] = MAP_JSON_VERSION;
    src["discord_rpc_icon_id"] = discordRpcIconId;
    src["discord_rpc_map_name"] = discordRpcMapName;
    src["has_sky"] = hasSky;
    src["sky_texture"] = skyTexture;
    src["light_cube_luxels_per_unit"] = lightCubeLuxelsPerUnit;
    src["brushes"] = nlohmann::ordered_json::array();
    src["actors"] = nlohmann::ordered_json::array();
    for (const Brush &brush: brushes)
    {
        src["brushes"].push_back(brush.GenerateJson());
    }
    for (const Actor &actor: actors)
    {
        src["actors"].push_back(actor.GenerateJson());
    }

    return FileIo::WriteStringToFile(filePath, src.dump(4));
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

std::vector<std::string> MapAsset::GetUniqueActorNames() const
{
    std::vector<std::string> actorNames{};
    for (const Actor &levelActor: actors)
    {
        if (!levelActor.params.contains("name"))
        {
            continue;
        }
        const Param &p = levelActor.params.at("name");
        if (p.GetType() != Param::ParamType::PARAM_TYPE_STRING)
        {
            continue;
        }
        const std::string actorName = p.Get<std::string>("");
        if (std::ranges::find(actorNames, actorName) != actorNames.end())
        {
            continue;
        }
        if (actorName.empty())
        {
            continue;
        }
        actorNames.push_back(p.Get<std::string>(""));
    }
    return actorNames;
}
