//
// Created by droc101 on 6/29/25.
//

#include "Options.h"
#include <cstdio>
#include <fstream>
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
    const nlohmann::json savedata = nlohmann::json::parse(j);
    if (savedata.is_discarded())
    {
        printf("Failed to parse options JSON, loading defaults.");
        LoadDefault();
    } else
    {
        gamePath = savedata.value("game_path", std::string());
        defaultTexture = savedata.value("default_texture", std::string("texture/level/wall_test.gtex"));
        theme = savedata.value("theme", Theme::SYSTEM);
    }
    file.close();
}

void Options::LoadDefault()
{
    gamePath = std::string();
    defaultTexture = "texture/level/wall_test.gtex";
    theme = Theme::SYSTEM;
}

void Options::Save()
{
    const nlohmann::json savedata = {
        {"game_path", gamePath},
        {"default_texture", defaultTexture},
        {"theme", theme},
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
