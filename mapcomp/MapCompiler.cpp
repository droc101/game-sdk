//
// Created by droc101 on 11/17/25.
//

#include "MapCompiler.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <glm/detail/func_geometric.inl>
#include <glm/vec2.hpp>
#include <libassets/asset/LevelMaterialAsset.h>
#include <libassets/asset/MapAsset.h>
#include <libassets/type/Actor.h>
#include <libassets/type/Asset.h>
#include <libassets/type/Sector.h>
#include <libassets/type/WallMaterial.h>
#include <libassets/util/AssetReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "LevelMeshBuilder.h"
#include "SectorCollisionBuilder.h"

MapCompiler::MapCompiler(const std::string &assetsDirectory)
{
    this->assetsDirectory = assetsDirectory;
}

Error::ErrorCode MapCompiler::LoadMapSource(const std::string &mapSourceFile)
{
    mapBasename = std::filesystem::path(mapSourceFile).stem().string();
    return MapAsset::CreateFromMapSrc(mapSourceFile.c_str(), map);
}

Error::ErrorCode MapCompiler::Compile() const
{
    std::vector<uint8_t> buffer;
    const Error::ErrorCode e = SaveToBuffer(buffer);
    if (e != Error::ErrorCode::OK)
    {
        return e;
    }

    const std::string outPath = assetsDirectory + "/map/" + mapBasename + ".gmap";
    printf("Saving map to \"%s\"\n", outPath.c_str());

    return AssetReader::SaveToFile(outPath.c_str(),
                                   buffer,
                                   Asset::AssetType::ASSET_TYPE_LEVEL,
                                   MapAsset::MAP_ASSET_VERSION);
}

LevelMaterialAsset MapCompiler::GetMapMaterial(const std::string &path) const
{
    LevelMaterialAsset mapMaterial;
    const Error::ErrorCode e = LevelMaterialAsset::CreateFromAsset((assetsDirectory + "/" + path).c_str(), mapMaterial);
    if (e != Error::ErrorCode::OK)
    {
        printf("Compile Warning: Failed to load material \"%s\": %s\n", path.c_str(), Error::ErrorString(e).c_str());
    }
    return mapMaterial;
}


Error::ErrorCode MapCompiler::SaveToBuffer(std::vector<uint8_t> &buffer) const
{
    assert(buffer.empty());

    printf("Found %zu sectors\n", map.sectors.size());
    printf("Found %zu actors\n", map.actors.size());

    for (const Sector &sector: map.sectors)
    {
        if (!sector.IsValid())
        {
            printf("Compile Error: Invalid Sector\n");
            return Error::ErrorCode::INCORRECT_FORMAT;
        }
    }

    DataWriter writer = DataWriter();

    writer.WriteString(map.sky_texture);
    writer.WriteString(map.discord_rpc_icon_id);
    writer.WriteString(map.discord_rpc_map_name);

    writer.Write<size_t>(map.actors.size());

    size_t numPlayerActors = 0;
    for (const Actor &actor: map.actors)
    {
        actor.Write(writer);
        if (actor.className == "player")
        {
            printf("Found player spawnpoint at %f %f %f\n", actor.position[0], actor.position[1], actor.position[2]);
            numPlayerActors++;
        }

        bool insideSector = false;
        for (const Sector &sector: map.sectors)
        {
            const bool insideHorizontally = sector.ContainsPoint({actor.position[0], actor.position[2]});
            const bool insideVertically = actor.position[1] >= sector.floorHeight &&
                                          actor.position[1] <= sector.ceilingHeight;
            if (insideHorizontally && insideVertically)
            {
                insideSector = true;
                break;
            }
        }
        if (!insideSector)
        {
            printf("Compile Warning: Found an actor of type \"%s\" that is not inside any sector.\n",
                   actor.className.c_str());
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
    std::vector<SectorCollisionBuilder> collisionBuilders{};
    for (size_t sectorIndex = 0; sectorIndex < map.sectors.size(); sectorIndex++)
    {
        const Sector &sector = map.sectors[sectorIndex];
        SectorCollisionBuilder builder = SectorCollisionBuilder(sector);
        for (size_t i = 0; i < sector.points.size(); i++)
        {
            const glm::vec2 &wallStart = sector.points.at(i);
            const glm::vec2 &wallEnd = sector.points.at((i + 1) % sector.points.size());
            std::vector<std::array<float, 2>> gaps{};
            for (size_t otherSectorIndex = 0; otherSectorIndex < map.sectors.size(); otherSectorIndex++)
            {
                const Sector &otherSector = map.sectors[otherSectorIndex];

                if ((otherSector.ceilingHeight < sector.floorHeight &&
                     otherSector.floorHeight < sector.floorHeight) ||
                    (otherSector.ceilingHeight > sector.ceilingHeight && otherSector.floorHeight > sector.ceilingHeight))
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
                    if ((wallStart == otherWallStart && wallEnd == otherWallEnd) ||
                        (wallStart == otherWallEnd && wallEnd == otherWallStart))
                    {
                        // MATCH FOUND!!!
                        gaps.push_back({otherSector.floorHeight, otherSector.ceilingHeight});
                        break; // break here because only 1 wall per (well-formed) sector can overlap
                    }
                }
            }

            std::vector<std::array<float, 2>> solidSegments{};
            if (!gaps.empty())
            {
                std::ranges::sort(gaps, SectorFloorCeilingCompare);
                float firstFloor = gaps.at(0).at(0);
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
                float lastCeil = gaps.at(gaps.size() - 1).at(1);
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
            meshBuilders[sector.ceilingMaterial.material].AddCeiling(sector);
        }
        if (!ceilingMaterial.compileNoClip)
        {
            builder.AddCeiling();
        }

        const LevelMaterialAsset floorMaterial = GetMapMaterial(sector.floorMaterial.material);
        if (!floorMaterial.compileInvisible)
        {
            meshBuilders[sector.floorMaterial.material].AddFloor(sector);
        }
        if (!floorMaterial.compileNoClip)
        {
            // builder.NextShape();
            builder.AddFloor();
        }

        collisionBuilders.push_back(builder);
    }

    std::erase_if(meshBuilders,
                  [](const std::pair<std::string, LevelMeshBuilder> &item) -> bool { return item.second.IsEmpty(); });

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

    writer.CopyToVector(buffer);
    return Error::ErrorCode::OK;
}

bool MapCompiler::SectorFloorCeilingCompare(const std::array<float, 2> &a, const std::array<float, 2> &b)
{
    return a.at(0) < b.at(0) && a.at(1) < b.at(1);
}
