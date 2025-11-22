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
#include "libassets/asset/LevelMaterialAsset.h"
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

    // TODO sector.IsValid seems to be broken :(
    // for (const Sector &sector: map.sectors)
    // {
    //     if (!sector.IsValid())
    //     {
    //         printf("Compile Error: Invalid Sector\n");
    //         return Error::ErrorCode::INCORRECT_FORMAT;
    //     }
    // }

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
    std::vector<SectorCollisionBuilder> collisionBuilders{};
    for (size_t sectorIndex = 0; sectorIndex < map.sectors.size(); sectorIndex++)
    {
        const Sector &sector = map.sectors[sectorIndex];
        SectorCollisionBuilder builder = SectorCollisionBuilder(sector);
        for (size_t i = 0; i < sector.points.size(); i++)
        {
            const std::array<float, 2> &wallStart = sector.points.at(i);
            const std::array<float, 2> &wallEnd = sector.points.at((i + 1) % sector.points.size());
            const glm::vec2 wallStartV = glm::vec2(wallStart[0], wallStart[1]);
            const glm::vec2 wallEndV = glm::vec2(wallEnd[0], wallEnd[1]);
            bool foundAdjWall = false;
            float adjFloor = 0;
            float adjCeil = 0;
            for (size_t otherSectorIndex = 0; otherSectorIndex < map.sectors.size(); otherSectorIndex++)
            {
                const Sector &otherSector = map.sectors[otherSectorIndex];
                if (sectorIndex == otherSectorIndex)
                {
                    continue;
                }
                for (size_t j = 0; j < otherSector.points.size(); j++)
                {
                    const std::array<float, 2> &otherWallStart = otherSector.points.at(j);
                    const std::array<float, 2> &otherWallEnd = otherSector.points.at((j + 1) %
                                                                                     otherSector.points.size());
                    const glm::vec2 otherWallStartV = glm::vec2(otherWallStart[0], otherWallStart[1]);
                    const glm::vec2 otherWallEndV = glm::vec2(otherWallEnd[0], otherWallEnd[1]);
                    if ((wallStartV == otherWallStartV && wallEndV == otherWallEndV) ||
                        (wallStartV == otherWallEndV && wallEndV == otherWallStartV))
                    {
                        // MATCH FOUND!!!
                        printf("Found adjacent walls %zu,%zu and %zu,%zu\n", sectorIndex, i, otherSectorIndex, j);
                        foundAdjWall = true;
                        adjFloor = otherSector.floorHeight;
                        adjCeil = otherSector.ceilingHeight;
                        break;
                    }
                }
            }

            const WallMaterial &mat = sector.wallMaterials.at(i);
            const LevelMaterialAsset mapMaterial = GetMapMaterial(mat.material);
            if (!mapMaterial.compileInvisible)
            {
                if (foundAdjWall)
                {
                    meshBuilders[mat.material].AddWallWithGap(sector, i, adjFloor, adjCeil);
                } else
                {
                    meshBuilders[mat.material].AddWall(sector, i);
                }
            }

            if (!mapMaterial.compileNoClip)
            {
                if (foundAdjWall)
                {
                    builder.AddWallWithGap(i, adjFloor, adjCeil);
                } else
                {
                    builder.AddWall(i);
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
            builder.NextShape();
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
