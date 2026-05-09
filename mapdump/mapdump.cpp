//
// Created by droc101 on 3/16/26.
//

#include <array>
#include <cstdint>
#include <cstdio>
#include <format>
#include <fstream>
#include <half.h>
#include <ImathConfig.h>
#include <ImfChannelList.h>
#include <ImfFrameBuffer.h>
#include <ImfHeader.h>
#include <ImfOutputFile.h>
#include <ImfPixelType.h>
#include <libassets/asset/MapAsset.h>
#include <libassets/type/Asset.h>
#include <libassets/type/IOConnection.h>
#include <libassets/type/MapVertex.h>
#include <libassets/type/Param.h>
#include <libassets/util/ArgumentParser.h>
#include <libassets/util/AssetReader.h>
#include <libassets/util/Error.h>
#include <libassets/util/Logger.h>
#include <OpenEXRConfig.h>
#include <vector>

using namespace OPENEXR_IMF_NAMESPACE;
using namespace IMATH_NAMESPACE;

int main(const int argc, const char **argv)
{
    Logger::Info("GAME SDK Map Dumper");
    const ArgumentParser args = ArgumentParser(argc, argv);

    if (args.HasFlag("--help") || args.HasFlag("-h"))
    {
        printf("Usage: mapdump [options]\n");
        printf("\n-- Required Options --\n");
        printf("--map=/path/to/map.gmap.......................The path to the GMAP file to dump.\n");
        printf("\n-- Dumping Options --\n");
        printf("All dumping options take an optional value specifying what file to save to.\n");
        printf("--dump-visual-model[=visual.obj]..............Dump the visual model with texture UVs.\n");
        printf("--dump-lightmap-model[=visual_lightmap.obj]...Dump the visual model with lightmap UVs.\n");
        printf("--dump-collision-model[=collision.obj]........Dump the collision model.\n");
        printf("--dump-lightmap[=lightmap.exr]................Dump the HDR lightmap texture as an EXR\n");
        return 0;
    }

    if (!args.HasFlagWithValue("--map"))
    {
        Logger::Error("Missing --map argument!");
        return 1;
    }

    Logger::Info("Loading map...");

    Asset asset;
    const Error::ErrorCode e = AssetReader::LoadFromFile(args.GetFlagValue("--map").c_str(), asset);
    if (e != Error::ErrorCode::OK)
    {
        Logger::Error("Failed to open map file: {}", Error::ErrorString(e).c_str());
        return 1;
    }

    if (asset.type != Asset::AssetType::ASSET_TYPE_LEVEL || asset.typeVersion != MapAsset::MAP_ASSET_VERSION)
    {
        Logger::Error("This is not a map file");
        return 1;
    }

    if (asset.reader.Read<uint8_t>() != 0) // render sky
    {
        std::string skyTex{};
        asset.reader.ReadStringWithSize(skyTex);
    }
    std::string discordRpcIcon{};
    asset.reader.ReadStringWithSize(discordRpcIcon);
    std::string discordRpcName{};
    asset.reader.ReadStringWithSize(discordRpcName);

    Logger::Info("Map name: {}", discordRpcName.c_str());

    const size_t numActors = asset.reader.Read<size_t>();
    for (size_t i = 0; i < numActors; i++)
    {
        std::string className{};
        asset.reader.ReadStringWithSize(className);

        asset.reader.Seek(sizeof(float) * 3 * 2); // position then rotation

        const size_t numIoConnections = asset.reader.Read<size_t>();
        for (size_t j = 0; j < numIoConnections; j++)
        {
            const IOConnection conn = IOConnection(asset.reader);
        }

        const KvList params = Param::ReadKvList(asset.reader);
    }

    Logger::Info("{} actor(s)", numActors);

    std::vector<MapVertex> mapVerts{};
    std::vector<uint32_t> mapIndices{};
    size_t indexCounter = 0;

    const size_t numMapModels = asset.reader.Read<size_t>();
    for (size_t i = 0; i < numMapModels; i++)
    {
        std::string materialName{};
        asset.reader.ReadStringWithSize(materialName);
        const uint32_t numVerts = asset.reader.Read<uint32_t>();
        for (size_t j = 0; j < numVerts; j++)
        {
            MapVertex v{};
            v.position = asset.reader.ReadVec3();
            v.uv = asset.reader.ReadVec2();
            v.lightmapUv = asset.reader.ReadVec2();
            mapVerts.push_back(v);
        }
        const uint32_t numIndices = asset.reader.Read<uint32_t>();
        for (size_t j = 0; j < numIndices; j++)
        {
            mapIndices.push_back(asset.reader.Read<uint32_t>() + indexCounter);
        }
        indexCounter += numVerts;
    }

    Logger::Info("{} model(s)", numMapModels);

    std::vector<std::array<glm::vec3, 3>> collisionTriangles{};

    const size_t numCollisionModels = asset.reader.Read<size_t>();
    for (size_t i = 0; i < numCollisionModels; i++)
    {
        const glm::vec3 position = asset.reader.ReadVec3();
        const size_t numSubShapes = asset.reader.Read<size_t>();
        for (size_t j = 0; j < numSubShapes; j++)
        {
            const size_t numTriangles = asset.reader.Read<size_t>();
            collisionTriangles.reserve(numTriangles);
            for (size_t k = 0; k < numTriangles; k++)
            {
                std::array<glm::vec3, 3> triangle{};
                triangle.at(0) = asset.reader.ReadVec3() + position;
                triangle.at(1) = asset.reader.ReadVec3() + position;
                triangle.at(2) = asset.reader.ReadVec3() + position;
                collisionTriangles.push_back(triangle);
            }
        }
    }

    Logger::Info("{} collision model(s)", numCollisionModels);

    const size_t lightmapWidth = asset.reader.Read<size_t>();
    const size_t lightmapHeight = asset.reader.Read<size_t>();
    std::vector<uint16_t> pixels{};
    for (size_t i = 0; i < lightmapWidth * lightmapHeight * 4; i++)
    {
        pixels.push_back(asset.reader.Read<uint16_t>());
    }

    Logger::Info("{}x{} lightmap", lightmapWidth, lightmapHeight);

    if (args.HasFlag("--dump-visual-model"))
    {
        std::string modelPath = "visual.obj";
        if (args.HasFlagWithValue("--dump-visual-model"))
        {
            modelPath = args.GetFlagValue("--dump-visual-model");
        }
        Logger::Info("Exporting Visual Model to \"{}\"...", modelPath.c_str());
        std::ofstream visualMesh = std::ofstream(modelPath);
        if (!visualMesh.is_open())
        {
            Logger::Error("Failed to open output file!");
            return 1;
        }
        visualMesh << "# Generated by GAME SDK\n\n";
        for (const MapVertex &vertex: mapVerts)
        {
            visualMesh << std::format("v {} {} {}\n", vertex.position.x, vertex.position.y, vertex.position.z);
            visualMesh << std::format("vt {} {}\n", vertex.uv.x, vertex.uv.y);
        }

        visualMesh << "\n\n";

        for (size_t m = 0; m < mapIndices.size(); m += 3)
        {
            const uint32_t index0 = mapIndices.at(m + 0) + 1;
            const uint32_t index1 = mapIndices.at(m + 1) + 1;
            const uint32_t index2 = mapIndices.at(m + 2) + 1;
            visualMesh << std::format("f {0}/{0} {1}/{1} {2}/{2}\n", index0, index1, index2);
        }
        visualMesh.close();
    }

    if (args.HasFlag("--dump-lightmap-model"))
    {
        std::string modelPath = "visual_lightmap.obj";
        if (args.HasFlagWithValue("--dump-lightmap-model"))
        {
            modelPath = args.GetFlagValue("--dump-lightmap-model");
        }
        Logger::Info("Exporting Lightmap Model to \"{}\"...", modelPath.c_str());
        std::ofstream visualLightmapMesh = std::ofstream(modelPath);
        if (!visualLightmapMesh.is_open())
        {
            Logger::Error("Failed to open output file!");
            return 1;
        }
        visualLightmapMesh << "# Generated by GAME SDK\n\n";
        for (const MapVertex &vertex: mapVerts)
        {
            visualLightmapMesh << std::format("v {} {} {}\n", vertex.position.x, vertex.position.y, vertex.position.z);
            visualLightmapMesh << std::format("vt {} {}\n", vertex.lightmapUv.x, 1.0 - vertex.lightmapUv.y);
        }

        visualLightmapMesh << "\n\n";

        for (size_t m = 0; m < mapIndices.size(); m += 3)
        {
            const uint32_t index0 = mapIndices.at(m + 0) + 1;
            const uint32_t index1 = mapIndices.at(m + 1) + 1;
            const uint32_t index2 = mapIndices.at(m + 2) + 1;
            visualLightmapMesh << std::format("f {0}/{0} {1}/{1} {2}/{2}\n", index0, index1, index2);
        }
        visualLightmapMesh.close();
    }

    if (args.HasFlag("--dump-collision-model"))
    {
        std::string modelPath = "collision.obj";
        if (args.HasFlagWithValue("--dump-collision-model"))
        {
            modelPath = args.GetFlagValue("--dump-collision-model");
        }
        Logger::Info("Exporting Collision Model to \"{}\"...", modelPath.c_str());
        std::ofstream collisionMesh = std::ofstream(modelPath);
        if (!collisionMesh.is_open())
        {
            Logger::Error("Failed to open output file!");
            return 1;
        }
        collisionMesh << "# Generated by GAME SDK\n\n";
        size_t vertCounter = 1; // obj is 1 based
        for (const std::array<glm::vec3, 3> &triangle: collisionTriangles)
        {
            collisionMesh << std::format("v {} {} {}\n", triangle.at(0).x, triangle.at(0).y, triangle.at(0).z);
            collisionMesh << std::format("v {} {} {}\n", triangle.at(1).x, triangle.at(1).y, triangle.at(1).z);
            collisionMesh << std::format("v {} {} {}\n", triangle.at(2).x, triangle.at(2).y, triangle.at(2).z);
            collisionMesh << std::format("f {0} {1} {2}\n", vertCounter, vertCounter + 1, vertCounter + 2);
            vertCounter += 3;
        }
        collisionMesh.close();
    }

    if (args.HasFlag("--dump-lightmap"))
    {
        std::string lightmapPath = "lightmap.exr";
        if (args.HasFlagWithValue("--dump-lightmap"))
        {
            lightmapPath = args.GetFlagValue("--dump-lightmap");
        }
        Logger::Info("Exporting Lightmap to \"{}\"...", lightmapPath.c_str());
        Header header = Header(static_cast<int>(lightmapWidth), static_cast<int>(lightmapHeight));
        header.channels().insert("R", Channel(HALF));
        header.channels().insert("G", Channel(HALF));
        header.channels().insert("B", Channel(HALF));
        header.channels().insert("A", Channel(HALF));

        FrameBuffer framebuffer;
        constexpr size_t PIXEL_SIZE = sizeof(half) * 4;
        char *base = reinterpret_cast<char *>(pixels.data());

        framebuffer.insert("R", Slice(HALF, base + sizeof(half) * 0, PIXEL_SIZE, PIXEL_SIZE * lightmapWidth));

        framebuffer.insert("G", Slice(HALF, base + sizeof(half) * 1, PIXEL_SIZE, PIXEL_SIZE * lightmapWidth));

        framebuffer.insert("B", Slice(HALF, base + sizeof(half) * 2, PIXEL_SIZE, PIXEL_SIZE * lightmapWidth));

        framebuffer.insert("A", Slice(HALF, base + sizeof(half) * 3, PIXEL_SIZE, PIXEL_SIZE * lightmapWidth));

        OutputFile file(lightmapPath.c_str(), header);
        file.setFrameBuffer(framebuffer);
        file.writePixels(static_cast<int>(lightmapHeight));
    }

    Logger::Info("Done");

    return 0;
}
