//
// Created by droc101 on 11/17/25.
//

#include <cstdio>
#include <libassets/util/Error.h>
#include "ArgumentParser.h"
#include "MapCompiler.h"

int main(const int argc, const char **argv)
{
    printf("GAME SDK Map Compiler\n");
    const ArgumentParser args = ArgumentParser(argc, argv);

    if (!args.hasFlagWithValue("--map-source"))
    {
        printf("[ERROR] --map-source not specified!\n");
        return 1;
    }

    if (!args.hasFlagWithValue("--assets-dir"))
    {
        printf("[ERROR] --assets-dir not specified!\n");
        return 1;
    }

    MapCompiler compiler = MapCompiler(args.getValue("--assets-dir"));

    const Error::ErrorCode mapLoadResult = compiler.LoadMapSource(args.getValue("--map-source"));
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

    return 0;
}
