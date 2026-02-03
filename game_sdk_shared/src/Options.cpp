//
// Created by droc101 on 6/29/25.
//

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <game_sdk/Options.h>
#include <nlohmann/json.hpp>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_stdinc.h>
#include <sstream>
#include <string>

void Options::Load()
{
    char *prefix = SDL_GetPrefPath("Droc101 Development", "GAME SDK");
    const std::string path = prefix + std::string("options.json");
    SDL_free(prefix);
    std::ifstream file(path);
    if (!file.is_open())
    {
        printf("Could not open file %s\n", path.data());
        return;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    const std::string j = ss.str();
    if (j.empty())
    {
        printf("options.json was empty, loading defaults.\n");
        LoadDefault();
        file.close();
        return;
    }
    const nlohmann::json savedata = nlohmann::json::parse(j);
    if (savedata.is_discarded())
    {
        printf("Failed to parse options JSON, loading defaults.\n");
        LoadDefault();
    } else
    {
        gamePath = savedata.value("game_path", std::string());
        overrideAssetsPath = savedata.value("override_assets_path", false);
        assetsPath = savedata.value("assets_path", std::string());
        defaultTexture = savedata.value("default_texture", std::string("texture/level/wall_test.gtex"));
        defaultMaterial = savedata.value("default_material", std::string("material/dev/wall_test.gmtl"));
        theme = savedata.value("theme", Theme::SYSTEM);
    }
    file.close();
}

void Options::LoadDefault()
{
    gamePath = std::string();
    defaultTexture = "texture/level/wall_test.gtex";
    defaultMaterial = "material/dev/wall_test.gmtl";
    overrideAssetsPath = false;
    assetsPath = "";
    theme = Theme::SYSTEM;
}

void Options::Save()
{
    const nlohmann::json savedata = {
        {"game_path", gamePath},
        {"default_texture", defaultTexture},
        {"default_material", defaultMaterial},
        {"theme", theme},
        {"override_assets_path", overrideAssetsPath},
        {"assets_path", assetsPath},
    };
    char *prefix = SDL_GetPrefPath("Droc101 Development", "GAME SDK");
    const std::string path = prefix + std::string("options.json");
    SDL_free(prefix);
    std::ofstream file(path);
    if (!file.is_open())
    {
        printf("Could not open file %s\n", path.data());
        return;
    }
    file << savedata.dump(); // a shift operator should not write to a stream this is not ok
    file.close();
}

std::string Options::GetAssetsPath()
{
    if (overrideAssetsPath)
    {
        return assetsPath;
    }
    return gamePath + "/assets";
}

bool Options::ValidateGamePath()
{
    if (!std::filesystem::is_directory(gamePath))
    {
        return false;
    }
    if (!std::filesystem::is_directory(gamePath + "/assets"))
    {
        return false;
    }
    if (!std::filesystem::is_directory(gamePath + "/bin"))
    {
        return false;
    }
#ifdef WIN32
    if (!std::filesystem::is_regular_file(gamePath + "/game.exe"))
    {
        return false;
    }
#else
    if (!std::filesystem::is_regular_file(gamePath + "/game"))
    {
        return false;
    }
#endif
    return true;
}
