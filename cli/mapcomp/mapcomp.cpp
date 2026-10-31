//
// Created by droc101 on 11/17/25.
//

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <libassets/asset/DataAsset.h>
#include <libassets/util/ArgumentParser.h>
#include <libassets/util/Error.h>
#include <libassets/util/Logger.h>
#include <libassets/util/SearchPathManager.h>
#include <vector>
#include "MapCompiler.h"

int main(const int argc, const char **argv)
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
    const ArgumentParser args = ArgumentParser(argc, argv);
    Logger::ansi = !args.HasFlag("--no-ansi");
    Logger::verbose = args.HasFlag("--verbose");

    Logger::Info("GAME SDK Map Compiler");

    if (args.HasFlag("--help") || args.HasFlag("-h"))
    {
        printf("Usage: mapcomp [options]\n");
        printf("\n-- Compiler Options --\n");
        printf("--map-source=/path/to/map.json.................Path to the map source to compile\n");
        printf("--map-sources-dir=/path/to/map/sources/dir.....Path to a directory containing map sources to "
               "compile\n");
        printf("--assets-dir=/path/to/game/assets..............Path to the directory containing game.gkvl\n");
        printf("--executable-dir=/path/to/game/dir.............Path to the directory containing the game executable\n");
        printf("--skip-lighting................................Skip light baking\n");
        printf("--fast-compression.............................Compress maps faster, but with worse compression\n");
        printf("--low-resolution...............................Limit the number of units per luxel\n");
        printf("--bounces=N....................................Number of GI bounces\n");
        printf("--samples=N....................................Number of GI samples\n");
        printf("--no-ansi......................................Disable ANSI output\n");
        printf("--verbose......................................Enable verbose output\n");
        return 0;
    }

    if (!(args.HasFlagWithValue("--map-source") || args.HasFlagWithValue("--map-sources-dir")))
    {
        Logger::Error("--map-source not specified!");
        return 1;
    }

    if (!args.HasFlagWithValue("--assets-dir"))
    {
        Logger::Error("--assets-dir not specified!");
        return 1;
    }

    if (!args.HasFlagWithValue("--executable-dir"))
    {
        Logger::Error("--executable-dir not specified!");
        return 1;
    }

    const std::string gameConfigPath = args.GetFlagValue("--assets-dir") + "/game.gkvl";
    DataAsset gameConfig{};
    const Error::ErrorCode e = gameConfig.LoadFromAsset(gameConfigPath);
    if (e != Error::ErrorCode::OK)
    {
        Logger::Error("Failed to open {}: {}", gameConfigPath.c_str(), Error::ErrorString(e).c_str());
        return 1;
    }

    const std::string assetsPath = args.GetFlagValue("--assets-dir");

    uint32_t bounces = 3;
    uint32_t samples = 8192;

    if (args.HasFlagWithValue("--bounces"))
    {
        bounces = std::strtol(args.GetFlagValue("--bounces").c_str(), nullptr, 10);
    }

    if (args.HasFlagWithValue("--samples"))
    {
        samples = std::strtol(args.GetFlagValue("--samples").c_str(), nullptr, 10);
    }

    MapCompiler::MapCompilerSettings settings = {
        .assetsDirectory = assetsPath,
        .executableDirectory = args.GetFlagValue("--executable-dir"),
        .gameConfigParentDirectory = std::filesystem::path(assetsPath).parent_path().string(),
        .gameConfig = gameConfig,
        .skipLighting = args.HasFlag("--skip-lighting"),
        .lowResolutionLightmap = args.HasFlag("--low-resolution"),
        .fastCompression = args.HasFlag("--fast-compression"),
        .bounceCount = bounces,
        .sampleCount = samples,
    };

    MapCompiler compiler = MapCompiler(settings);

    if (args.HasFlagWithValue("--map-source"))
    {
        const Error::ErrorCode mapLoadResult = compiler.LoadMapSource(args.GetFlagValue("--map-source"));
        if (mapLoadResult != Error::ErrorCode::OK)
        {
            Logger::Error("Failed to load map source: {}", Error::ErrorString(mapLoadResult).c_str());
            return 1;
        }

        const Error::ErrorCode mapCompileResult = compiler.Compile();
        if (mapCompileResult != Error::ErrorCode::OK)
        {
            Logger::Error("Failed to compile map: {}", Error::ErrorString(mapCompileResult).c_str());
            return 1;
        }
    } else
    {
        const bool returnOnError = args.HasFlag("--break-on-error");
        const std::vector<std::string> maps = SearchPathManager::ScanFolder(args.GetFlagValue("--map-sources-dir"),
                                                                            ".json",
                                                                            true);
        for (const std::string &map: maps)
        {
            Logger::Info("Compiling map \"{}\"...", map);
            const Error::ErrorCode mapLoadResult = compiler.LoadMapSource(args.GetFlagValue("--map-sources-dir") +
                                                                          "/" +
                                                                          map);
            if (mapLoadResult != Error::ErrorCode::OK)
            {
                Logger::Error("Failed to load map source: {}", Error::ErrorString(mapLoadResult).c_str());
                if (returnOnError)
                {
                    return 1;
                }
                continue;
            }

            const Error::ErrorCode mapCompileResult = compiler.Compile();
            if (mapCompileResult != Error::ErrorCode::OK)
            {
                Logger::Error("Failed to compile map: {}", Error::ErrorString(mapCompileResult).c_str());
                if (returnOnError)
                {
                    return 1;
                }
            }
        }
    }

    return 0;
}
