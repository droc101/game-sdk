//
// Created by droc101 on 6/29/25.
//

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <game_sdk/Options.h>
#include <nlohmann/json.hpp>
#include <SDL3/SDL_filesystem.h>
#include <sstream>
#include <string>

Options &Options::Get()
{
    static Options optionsSingleton{};

    return optionsSingleton;
}

void Options::Load()
{
    const std::string path = SDL_GetBasePath() + std::string("sdk_options.json");
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
        gameExecutablePath = savedata.value("game_executable_path", "");
        gameConfigPath = savedata.value("game_config_path", "");
        defaultTexture = savedata.value("default_texture", "texture/level/uvtest.gtex");
        defaultMaterial = savedata.value("default_material", "material/dev/uv_test.gmtl");
        theme = savedata.value("theme", Theme::SYSTEM);
    }
    file.close();
}

void Options::LoadDefault()
{
    gameExecutablePath = "";
    defaultTexture = "texture/level/uvtest.gtex";
    defaultMaterial = "material/dev/uv_test.gmtl";
    gameConfigPath = "";
    theme = Theme::SYSTEM;
}

void Options::Save()
{
    const nlohmann::json savedata = {
        {"game_executable_path", gameExecutablePath},
        {"default_texture", defaultTexture},
        {"default_material", defaultMaterial},
        {"theme", theme},
        {"game_config_path", gameConfigPath},
    };
    const std::string path = SDL_GetBasePath() + std::string("sdk_options.json");
    std::ofstream file(path);
    if (!file.is_open())
    {
        printf("Could not open file %s\n", path.data());
        return;
    }
    file << savedata.dump(4); // a shift operator should not write to a stream this is not ok
    file.close();
}

std::string Options::GetAssetsPath() const
{
    const std::filesystem::path path = std::filesystem::path(gameConfigPath);
    return path.parent_path().string();
}

std::string Options::GetExecutablePath() const
{
    const std::filesystem::path path = std::filesystem::path(gameExecutablePath);
    return path.parent_path().string();
}

bool Options::ValidateGamePath() const
{
    if (!std::filesystem::is_regular_file(gameExecutablePath))
    {
        return false;
    }
    if (!std::filesystem::is_regular_file(gameConfigPath))
    {
        return false;
    }

    return true;
}
