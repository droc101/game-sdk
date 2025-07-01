//
// Created by droc101 on 6/29/25.
//

#include "Options.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <SDL3/SDL_filesystem.h>

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
    std::string j;
    file >> j;
    const nlohmann::json savedata = nlohmann::json::parse(j);
    std::string p = savedata["game_path"];
    std::strncpy(gamePath.data(), p.data(), gamePath.size() - 1);
    file.close();
}

void Options::Save()
{
    const nlohmann::json savedata = {{"game_path", std::string(gamePath.data())}};
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
