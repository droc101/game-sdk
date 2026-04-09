//
// Created by droc101 on 11/17/25.
//

#include "MapCompiler.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <glm/detail/func_geometric.inl>
#include <glm/vec2.hpp>
#include <libassets/asset/LevelMaterialAsset.h>
#include <libassets/asset/MapAsset.h>
#include <libassets/type/Actor.h>
#include <libassets/type/ActorDefinition.h>
#include <libassets/type/Asset.h>
#include <libassets/type/Sector.h>
#include <libassets/type/WallMaterial.h>
#include <libassets/util/AssetReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <libassets/util/Logger.h>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "LevelMeshBuilder.h"
#include "Light.h"
#include "LightBaker.hpp"
#include "SectorCollisionBuilder.h"

MapCompiler::MapCompiler(MapCompilerSettings &settings)
{
    this->settings = settings;
    this->pathManager = SearchPathManager(settings.gameConfig,
                                          settings.executableDirectory,
                                          settings.gameConfigParentDirectory);
    Error::ErrorCode e = Error::ErrorCode::UNKNOWN;
    this->defManager = ActorDefinitionManager(this->pathManager, e);
    if (e != Error::ErrorCode::OK)
    {
        Logger::Error("Failed to load actor definitions");
    }
}

Error::ErrorCode MapCompiler::LoadMapSource(const std::string &mapSourceFile)
{
    mapBasename = std::filesystem::path(mapSourceFile).stem().string();
    return MapAsset::CreateFromMapSrc(mapSourceFile.c_str(), map);
}

Error::ErrorCode MapCompiler::Compile()
{
    std::vector<uint8_t> buffer;
    const Error::ErrorCode e = SaveToBuffer(buffer);
    if (e != Error::ErrorCode::OK)
    {
        return e;
    }

    const std::string outPath = settings.assetsDirectory + "/map/" + mapBasename + ".gmap";
    Logger::Info("Saving map to \"{}\"", outPath.c_str());
    return AssetReader::SaveToFile(outPath.c_str(),
                                   buffer,
                                   Asset::AssetType::ASSET_TYPE_LEVEL,
                                   MapAsset::MAP_ASSET_VERSION,
                                   settings.fastCompile ? AssetReader::FASTEST_COMPRESSION
                                                        : AssetReader::BEST_COMPRESSION);
}

LevelMaterialAsset MapCompiler::GetMapMaterial(const std::string &path) const
{
    LevelMaterialAsset mapMaterial;
    const std::string absPath = pathManager.GetAssetPath(path);
    const Error::ErrorCode e = LevelMaterialAsset::CreateFromAsset(absPath.c_str(), mapMaterial);
    if (e != Error::ErrorCode::OK)
    {
        Logger::Error("Failed to load material \"{}\": {}", path.c_str(), Error::ErrorString(e).c_str());
    }
    return mapMaterial;
}


Error::ErrorCode MapCompiler::SaveToBuffer(std::vector<uint8_t> &buffer)
{
    assert(buffer.empty());

    Logger::Info("Found {} sectors", map.sectors.size());
    Logger::Info("Found {} actors", map.actors.size());

    for (size_t i = 0; i < map.sectors.size(); i++)
    {
        const Sector &sector = map.sectors.at(i);
        if (!sector.IsValid())
        {
            if (sector.name.empty())
            {
                Logger::Error("Sector {} has an invalid shape!", i);
            } else
            {
                Logger::Error("Sector {} \"{}\" has an invalid shape!", i, sector.name.c_str());
            }
            return Error::ErrorCode::INCORRECT_FORMAT;
        }
    }

    DataWriter writer = DataWriter();

    writer.Write<bool>(map.has_sky);
    if (map.has_sky)
    {
        writer.WriteString(map.sky_texture);
    }

    writer.WriteString(map.discord_rpc_icon_id);
    writer.WriteString(map.discord_rpc_map_name);

    Logger::Info("Compiling actors...");

    std::vector<Light> lights{};

    size_t numPlayerActors = 0;
    std::vector<Actor> actorsToWrite{};
    for (const Actor &actor: map.actors)
    {
        if (!defManager.HasActorClass(actor.className))
        {
            Logger::Warning("Skipping unknown actor class \"{}\"...", actor.className.c_str());
            continue;
        }

        const ActorDefinition &def = defManager.GetActorDefinition(actor.className);
        if (def.isVirtual)
        {
            Logger::Warning("Skipping virtual actor class \"{}\"...", actor.className.c_str());
            continue;
        }

        if (def.Extends("light"))
        {
            lights.emplace_back(actor);
            continue;
        }

        actorsToWrite.push_back(actor);
        if (def.Extends("player"))
        {
            Logger::Info("Found player spawnpoint at {} {} {}", actor.position.x, actor.position.y, actor.position.z);
            numPlayerActors++;
        }

        bool insideSector = false;
        for (const Sector &sector: map.sectors)
        {
            const bool insideHorizontally = sector.ContainsPoint({actor.position.x, actor.position.z});
            const bool insideVertically = actor.position.y >= sector.floorHeight &&
                                          actor.position.y <= sector.ceilingHeight;
            if (insideHorizontally && insideVertically)
            {
                insideSector = true;
                break;
            }
        }
        if (!insideSector)
        {
            Logger::Warning("Found an actor of type \"{}\" that is not inside any sector.", actor.className.c_str());
        }
    }

    if (numPlayerActors == 0)
    {
        Logger::Warning("There is no player actor, the player will spawn at the origin.");
    } else if (numPlayerActors != 1)
    {
        Logger::Warning("Multiple player actors are present, only one will function.");
    }

    writer.Write<size_t>(actorsToWrite.size());
    for (const Actor &actor: actorsToWrite)
    {
        actor.Write(writer);
    }

    Logger::Info("Compiling Sectors...");

    if (settings.fastCompile)
    {
        for (Sector &sector: map.sectors)
        {
            sector.ceilingMaterial.luxelsPerUnit = std::min<uint8_t>(sector.ceilingMaterial.luxelsPerUnit, FAST_COMPILE_MAX_LUXELS_PER_UNIT);
            sector.floorMaterial.luxelsPerUnit = std::min<uint8_t>(sector.floorMaterial.luxelsPerUnit, FAST_COMPILE_MAX_LUXELS_PER_UNIT);
            for (WallMaterial &mat: sector.wallMaterials)
            {
                mat.luxelsPerUnit = std::min<uint8_t>(mat.luxelsPerUnit, FAST_COMPILE_MAX_LUXELS_PER_UNIT);
            }
        }
    }

    std::unordered_map<std::string, LevelMeshBuilder> meshBuilders{};
    std::vector<SectorCollisionBuilder> collisionBuilders{};
    for (size_t sectorIndex = 0; sectorIndex < map.sectors.size(); sectorIndex++)
    {
        const Sector &sector = map.sectors.at(sectorIndex);
        std::vector<const Sector *> overlappingCeilings{};
        std::vector<const Sector *> overlappingFloors{};
        SectorCollisionBuilder builder = SectorCollisionBuilder(sector);
        for (size_t i = 0; i < sector.points.size(); i++)
        {
            const glm::vec2 &wallStart = sector.points.at(i);
            const glm::vec2 &wallEnd = sector.points.at((i + 1) % sector.points.size());
            std::vector<std::array<float, 2>> gaps{};
            for (size_t otherSectorIndex = 0; otherSectorIndex < map.sectors.size(); otherSectorIndex++)
            {
                const Sector &otherSector = map.sectors.at(otherSectorIndex);

                if (sectorIndex != otherSectorIndex)
                {
                    if (sector.floorHeight == otherSector.ceilingHeight)
                    {
                        Logger::Verbose("Sector {}'s floor is overlapping with sector {}'s ceiling",
                                        sectorIndex,
                                        otherSectorIndex);
                        overlappingCeilings.push_back(&otherSector);
                    } else if (sector.ceilingHeight == otherSector.floorHeight)
                    {
                        Logger::Verbose("Sector {}'s ceiling is overlapping with sector {}'s floor",
                                        sectorIndex,
                                        otherSectorIndex);
                        overlappingFloors.push_back(&otherSector);
                    }
                }

                if ((otherSector.ceilingHeight < sector.floorHeight && otherSector.floorHeight < sector.floorHeight) ||
                    (otherSector.ceilingHeight > sector.ceilingHeight &&
                     otherSector.floorHeight > sector.ceilingHeight))
                {
                    continue; // Other sector is completely above or below this one, do not consider it
                }

                for (size_t j = 0; j < otherSector.points.size(); j++)
                {
                    if (sectorIndex == otherSectorIndex && i == j)
                    {
                        continue; // same sector and wall
                    }
                    const glm::vec2 &otherWallStart = otherSector.points.at(j);
                    const glm::vec2 &otherWallEnd = otherSector.points.at((j + 1) % otherSector.points.size());
                    // TODO: This should probably use an epsilon
                    if ((wallStart == otherWallStart && wallEnd == otherWallEnd) ||
                        (wallStart == otherWallEnd && wallEnd == otherWallStart))
                    {
                        // MATCH FOUND!!!
                        Logger::Verbose("Found overlapping walls: {}[{}] and {}[{}]",
                                        sectorIndex,
                                        i,
                                        otherSectorIndex,
                                        j);
                        gaps.push_back({otherSector.floorHeight, otherSector.ceilingHeight});
                        break; // break here because only 1 wall per (well-formed) sector can overlap
                    }
                }
            }

            std::vector<std::array<float, 2>> solidSegments{};
            if (!gaps.empty())
            {
                std::ranges::sort(gaps, SectorFloorCeilingCompare);
                const float firstFloor = gaps.at(0).at(0);
                if (sector.floorHeight < firstFloor)
                {
                    solidSegments.push_back({sector.floorHeight, firstFloor});
                }
                for (size_t gapIndex = 0; gapIndex < gaps.size(); gapIndex++)
                {
                    if (gapIndex < gaps.size() - 1)
                    {
                        const float adjCeil = gaps.at(gapIndex).at(1);
                        const float nextFloor = gaps.at(gapIndex + 1).at(0);
                        solidSegments.push_back({adjCeil, nextFloor});
                    }
                }
                const float lastCeil = gaps.at(gaps.size() - 1).at(1);
                if (sector.ceilingHeight > lastCeil)
                {
                    solidSegments.push_back({lastCeil, sector.ceilingHeight});
                }
            } else
            {
                solidSegments.push_back({sector.floorHeight, sector.ceilingHeight});
            }

            const WallMaterial &mat = sector.wallMaterials.at(i);
            const LevelMaterialAsset mapMaterial = GetMapMaterial(mat.material);
            if (!mapMaterial.compileInvisible)
            {
                for (const std::array<float, 2> &segment: solidSegments)
                {
                    meshBuilders[mat.material].AddWall(sector, i, segment.at(0), segment.at(1));
                }
            }

            if (!mapMaterial.compileNoClip)
            {
                for (const std::array<float, 2> &segment: solidSegments)
                {
                    builder.AddWall(i, segment.at(0), segment.at(1));
                }
            }
        }


        const LevelMaterialAsset ceilingMaterial = GetMapMaterial(sector.ceilingMaterial.material);
        if (!ceilingMaterial.compileInvisible)
        {
            meshBuilders[sector.ceilingMaterial.material].AddCeiling(sector, overlappingFloors);
        }
        if (!ceilingMaterial.compileNoClip)
        {
            builder.AddCeiling(overlappingFloors);
        }

        const LevelMaterialAsset floorMaterial = GetMapMaterial(sector.floorMaterial.material);
        if (!floorMaterial.compileInvisible)
        {
            meshBuilders[sector.floorMaterial.material].AddFloor(sector, overlappingCeilings);
        }
        if (!floorMaterial.compileNoClip)
        {
            // builder.NextShape();
            builder.AddFloor(overlappingCeilings);
        }

        collisionBuilders.push_back(builder);
    }

    std::erase_if(meshBuilders,
                  [](const std::pair<std::string, LevelMeshBuilder> &item) -> bool { return item.second.IsEmpty(); });

    Logger::Info("Level has {} visual meshes", meshBuilders.size());
    Logger::Info("Level has {} physics meshes", collisionBuilders.size());
    Logger::Info("Level has {} lights", lights.size());

    glm::uvec2 lightmapSize{};
    if (!LevelMeshBuilder::CalculateLightmapUvs(lightmapSize, meshBuilders, pathManager))
    {
        return Error::ErrorCode::LIGHTMAP_TOO_LARGE;
    }

    writer.Write<size_t>(meshBuilders.size());
    for (const std::pair<const std::string, LevelMeshBuilder> &builder: meshBuilders)
    {
        builder.second.Write(writer, builder.first);
    }

    writer.Write<size_t>(collisionBuilders.size());
    for (SectorCollisionBuilder &builder: collisionBuilders)
    {
        builder.Write(writer);
    }

    std::vector<uint8_t> pixels = {0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c}; // float16 1.0
    if (!lights.empty() && !settings.skipLighting)
    {
        Logger::Info("Baking lightmap...");
        if (!LightBaker::bake(meshBuilders, lights, pixels, lightmapSize, settings.bakeLightsOnCpu))
        {
            return Error::ErrorCode::UNKNOWN;
        }
        writer.Write<size_t>(lightmapSize.x);
        writer.Write<size_t>(lightmapSize.y);
        writer.WriteBuffer(pixels);

    } else
    {
        Logger::Info("Using fullbright lightmap");
        writer.Write<size_t>(1);
        writer.Write<size_t>(1);
        writer.WriteBuffer(pixels);
    }

    uint16_t numPointLights = 0;
    std::vector<Light> pointLights{};
    for (const Light &light: lights)
    {
        if (light.type == Light::Type::Point)
        {
            numPointLights++;
            pointLights.push_back(light);
        }
    }
    writer.Write<uint16_t>(numPointLights);
    for (const Light &pointLight: pointLights)
    {
        writer.WriteVec3(pointLight.position);
        writer.WriteVec3(pointLight.color);
        writer.Write<float>(pointLight.brightnessScale);
        writer.Write<float>(pointLight.range);
        writer.Write<float>(pointLight.attenuation);
    }

    writer.CopyToVector(buffer);
    return Error::ErrorCode::OK;
}

bool MapCompiler::SectorFloorCeilingCompare(const std::array<float, 2> &a, const std::array<float, 2> &b)
{
    return a.at(0) < b.at(0) && a.at(1) < b.at(1);
}
