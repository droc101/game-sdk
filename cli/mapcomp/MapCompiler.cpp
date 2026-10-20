//
// Created by droc101 on 11/17/25.
//

#include "MapCompiler.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <libassets/asset/Asset.h>
#include <libassets/asset/LevelMaterialAsset.h>
#include <libassets/asset/MapAsset.h>
#include <libassets/type/Actor.h>
#include <libassets/type/ActorDefinition.h>
#include <libassets/type/BoundingBox.h>
#include <libassets/type/Brush.h>
#include <libassets/type/MapVertex.h>
#include <libassets/util/AssetContainer.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <libassets/util/Logger.h>
#include <ranges>
#include <string>
#include <unordered_map>
#include <vector>
#include "BrushCollisionBuilder.h"
#include "BrushMeshBuilder.h"
#include "Light.h"
#include "LightBaker.hpp"

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
    return map.Import(mapSourceFile);
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
    Logger::Info("Saving map to \"{}\"", outPath);
    return AssetContainer::SaveToFile(outPath,
                                      buffer,
                                      Asset::AssetType::ASSET_TYPE_LEVEL,
                                      MapAsset::MAP_ASSET_VERSION,
                                      settings.fastCompile ? AssetContainer::FASTEST_COMPRESSION
                                                           : AssetContainer::BEST_COMPRESSION);
}

LevelMaterialAsset MapCompiler::GetMapMaterial(const std::string &path) const
{
    LevelMaterialAsset mapMaterial;
    const std::string absPath = pathManager.GetAssetPath(path);
    const Error::ErrorCode e = mapMaterial.LoadFromAsset(absPath);
    if (e != Error::ErrorCode::OK)
    {
        Logger::Error("Failed to load material \"{}\": {}", path.c_str(), Error::ErrorString(e));
    }
    return mapMaterial;
}


Error::ErrorCode MapCompiler::SaveToBuffer(std::vector<uint8_t> &buffer)
{
    assert(buffer.empty());

    Logger::Info("Found {} brushes", map.brushes.size());
    Logger::Info("Found {} actors", map.actors.size());

    for (size_t i = 0; i < map.brushes.size(); i++)
    {
        const Brush &brush = map.brushes.at(i);
        if (!brush.IsValid())
        {
            if (brush.editorName.empty())
            {
                Logger::Error("Brush {} has an invalid shape!", i);
            } else
            {
                Logger::Error("Brush {} \"{}\" has an invalid shape!", i, brush.editorName);
            }
            return Error::ErrorCode::INCORRECT_FORMAT;
        }
    }

    DataWriter writer = DataWriter();

    writer.Write<bool>(map.hasSky);
    if (map.hasSky)
    {
        writer.WriteString(map.skyTexture);
    }

    writer.WriteString(map.discordRpcIconId);
    writer.WriteString(map.discordRpcMapName);

    Logger::Info("Compiling actors...");

    bool hasDirectionalLight{};
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
            if (actor.className == "light_directional")
            {
                if (hasDirectionalLight)
                {
                    Logger::Error("Found more than one directional light!");
                    return Error::ErrorCode::INCORRECT_FORMAT;
                }
                hasDirectionalLight = true;
            }
            lights.emplace_back(actor);
        }

        if (!def.editorOnly)
        {
            actorsToWrite.push_back(actor);
        }

        if (def.Extends("player"))
        {
            Logger::Info("Found player spawnpoint at {} {} {}", actor.position.x, actor.position.y, actor.position.z);
            numPlayerActors++;
        }

        // TODO warn about any actors not inside the level (needs bsp/vis)
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
        for (Brush &brush: map.brushes)
        {
            for (Brush::Face &face: brush.faces)
            {
                face.unitsPerLuxel = std::max<float>(face.unitsPerLuxel, FAST_COMPILE_MIN_UNITS_PER_LUXEL);
            }
        }
    }

    std::vector<BrushMeshBuilder> mapMeshBuilders{};
    std::vector<BrushCollisionBuilder> collisionBuilders{};
    for (size_t brushIndex = 0; brushIndex < map.brushes.size(); brushIndex++)
    {
        const Brush &brush = map.brushes.at(brushIndex);
        BrushCollisionBuilder collisionBuilder = BrushCollisionBuilder(brush);
        std::unordered_map<std::string, BrushMeshBuilder> sectorMeshBuilders{};

        for (size_t i = 0; i < brush.faces.size(); i++)
        {
            const Brush::Face &face = brush.faces.at(i);
            const LevelMaterialAsset mapMaterial = GetMapMaterial(face.material);
            if (!mapMaterial.compileInvisible)
            {
                if (!sectorMeshBuilders.contains(face.material))
                {
                    sectorMeshBuilders.emplace(face.material, BrushMeshBuilder(pathManager, face.material));
                }
                sectorMeshBuilders.at(face.material).AddFace(brush, i);
            }

            if (!mapMaterial.compileNoClip)
            {
                collisionBuilder.AddFace(i);
            }
        }

        for (const BrushMeshBuilder &b: sectorMeshBuilders | std::views::values)
        {
            mapMeshBuilders.push_back(b);
        }
        collisionBuilders.push_back(collisionBuilder);
    }

    std::erase_if(mapMeshBuilders, [](const BrushMeshBuilder &item) -> bool { return item.IsEmpty(); });

    Logger::Info("Level has {} visual meshes", mapMeshBuilders.size());
    Logger::Info("Level has {} physics meshes", collisionBuilders.size());
    Logger::Info("Level has {} lights", lights.size());

    const bool skipLighting = lights.empty() || settings.skipLighting;

    glm::uvec2 lightmapSize{1};
    if (!skipLighting && !LightmapPacker::CalculateLightmapUvs(lightmapSize, mapMeshBuilders, pathManager))
    {
        return Error::ErrorCode::LIGHTMAP_TOO_LARGE;
    }

    writer.Write<size_t>(mapMeshBuilders.size());
    for (const BrushMeshBuilder &builder: mapMeshBuilders)
    {
        builder.Write(writer);
    }

    writer.Write<size_t>(collisionBuilders.size());
    for (BrushCollisionBuilder &builder: collisionBuilders)
    {
        builder.Write(writer);
    }

    std::vector<MapVertex> vertices{};
    for (const BrushMeshBuilder &builder: mapMeshBuilders)
    {
        vertices.insert(vertices.end(), builder.GetVertices().begin(), builder.GetVertices().end());
    }

    writer.Write<float>(2 * glm::length(BoundingBox(vertices).extents));

    std::vector<uint16_t> lightmapPixels = {0x3c00, 0x3c00, 0x3c00, 0x3c00}; // float16 1.0
    std::vector<uint16_t> indirectLightingLightmapPixels = {0, 0, 0, 0};
    if (!skipLighting)
    {
        Logger::Info("Baking lightmap...");
        if (!LightBaker::Bake(mapMeshBuilders, lights, lightmapSize, lightmapPixels, indirectLightingLightmapPixels))
        {
            return Error::ErrorCode::UNKNOWN;
        }
    } else
    {
        Logger::Info("Using fullbright lightmap");
    }

    Logger::Info("Finalizing Map...");
    writer.Write<size_t>(lightmapSize.x);
    writer.Write<size_t>(lightmapSize.y);
    writer.WriteBuffer(lightmapPixels);
    writer.WriteBuffer(indirectLightingLightmapPixels);
    if (settings.skipLighting)
    {
        writer.Write<uint32_t>(0); // number of lights
    } else
    {
        writer.Write<uint32_t>(lights.size());
        for (const Light &light: lights)
        {
            light.Write(writer);
        }
    }
    writer.CopyToVector(buffer);
    return Error::ErrorCode::OK;
}

bool MapCompiler::SectorFloorCeilingCompare(const std::array<float, 2> &a, const std::array<float, 2> &b)
{
    return a.at(0) < b.at(0) && a.at(1) < b.at(1);
}
