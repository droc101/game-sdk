//
// Created by droc101 on 11/17/25.
//

#include <cstdio>
#include <libassets/asset/DataAsset.h>
#include <libassets/util/ArgumentParser.h>
#include <libassets/util/Error.h>
#include <libassets/util/SearchPathManager.h>
#include <vector>
#include "MapCompiler.h"

int main(const int argc, const char **argv)
{
    setvbuf(stdout, nullptr, _IONBF, 0);

    printf("GAME SDK Map Compiler\n");
    const ArgumentParser args = ArgumentParser(argc, argv);

    if (!(args.HasFlagWithValue("--map-source") || args.HasFlagWithValue("--map-sources-dir")))
    {
        printf("[ERROR] --map-source not specified!\n");
        return 1;
    }

    if (!args.HasFlagWithValue("--assets-dir"))
    {
        printf("[ERROR] --assets-dir not specified!\n");
        return 1;
    }

    if (!args.HasFlagWithValue("--executable-dir"))
    {
        printf("[ERROR] --executable-dir not specified!\n");
        return 1;
    }

    const std::string gameConfigPath = args.GetFlagValue("--assets-dir") + "/game.gkvl";
    DataAsset gameConfig{};
    const Error::ErrorCode e = DataAsset::CreateFromAsset(gameConfigPath.c_str(), gameConfig);
    if (e != Error::ErrorCode::OK)
    {
        printf("[ERROR] Failed to open %s: %s", gameConfigPath.c_str(), Error::ErrorString(e).c_str());
        return 1;
    }

    const std::string assetsPath = args.GetFlagValue("--assets-dir");

    MapCompiler::MapCompilerSettings settings = {
        .assetsDirectory = assetsPath,
        .executableDirectory = args.GetFlagValue("--executable-dir"),
        .gameConfigParentDirectory = std::filesystem::path(assetsPath).parent_path().string(),
        .gameConfig = gameConfig,
        .bakeLightsOnCpu = args.HasFlag("--bake-on-cpu"),
        .skipLighting = args.HasFlag("--skip-lighting"),
    };

    MapCompiler compiler = MapCompiler(settings);

    if (args.HasFlagWithValue("--map-source"))
    {
        const Error::ErrorCode mapLoadResult = compiler.LoadMapSource(args.GetFlagValue("--map-source"));
        if (mapLoadResult != Error::ErrorCode::OK)
        {
            printf("[ERROR] Failed to load map source: %s\n", Error::ErrorString(mapLoadResult).c_str());
            return 1;
        }

        const Error::ErrorCode mapCompileResult = compiler.Compile();
        if (mapCompileResult != Error::ErrorCode::OK)
        {
            printf("[ERROR] Failed to compile map: %s\n", Error::ErrorString(mapCompileResult).c_str());
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
            const Error::ErrorCode mapLoadResult = compiler.LoadMapSource(args.GetFlagValue("--map-sources-dir") +
                                                                          "/" +
                                                                          map);
            if (mapLoadResult != Error::ErrorCode::OK)
            {
                printf("[ERROR] Failed to load map source: %s\n", Error::ErrorString(mapLoadResult).c_str());
                if (returnOnError)
                {
                    return 1;
                }
                continue;
            }

            const Error::ErrorCode mapCompileResult = compiler.Compile();
            if (mapCompileResult != Error::ErrorCode::OK)
            {
                printf("[ERROR] Failed to compile map: %s\n", Error::ErrorString(mapCompileResult).c_str());
                if (returnOnError)
                {
                    return 1;
                }
                continue;
            }
        }
    }

    return 0;
}
