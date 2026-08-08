//
// Created by droc101 on 6/29/25.
//

#include <filesystem>
#include <game_sdk/Options.h>
#include <libassets/util/FileIo.h>
#include <libassets/util/Logger.h>
#include <nlohmann/json.hpp>
#include <SDL3/SDL_filesystem.h>
#include <string>

Options &Options::Get()
{
    static Options optionsSingleton{};

    return optionsSingleton;
}

void Options::Load()
{
    const std::string path = SDL_GetBasePath() + std::string("sdk_options.json");
    std::string jsonString;
    Error::ErrorCode readError = FileIo::ReadFileToString(path, jsonString);
    if (readError != Error::ErrorCode::OK)
    {
        Logger::Error("Could not open options file {}: {}", path.data(), readError);
        LoadDefault();
        return;
    }
    if (jsonString.empty())
    {
        Logger::Warning("options.json was empty, loading defaults.");
        LoadDefault();
        return;
    }
    const nlohmann::json savedata = nlohmann::json::parse(jsonString);
    if (savedata.is_discarded())
    {
        Logger::Error("Failed to parse options JSON, loading defaults.");
        LoadDefault();
    } else
    {
        gameExecutablePath = savedata.value("game_executable_path", "");
        gameConfigPath = savedata.value("game_config_path", "");
        defaultTexture = savedata.value("default_texture", DEFAULT_TEXTURE);
        defaultMaterial = savedata.value("default_material", DEFAULT_MATERIAL);
        theme = savedata.value("theme", Theme::SYSTEM);
    }
}

void Options::LoadDefault()
{
    gameExecutablePath = "";
    defaultTexture = DEFAULT_TEXTURE;
    defaultMaterial = DEFAULT_MATERIAL;
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
    const Error::ErrorCode writeError = FileIo::WriteStringToFile(path, savedata.dump(4));
    if (writeError != Error::ErrorCode::OK)
    {
        Logger::Error("Failed to write options to \"{}\": {}", path, writeError);
    }
}

std::string Options::GetAssetsPath() const
{
    // TODO: Fix this breaking the ability to launch GAME from the SDK on Windows,
    //  without causing a std::filesystem_error when compiling maps on Linux
    const std::filesystem::path path = std::filesystem::path(gameConfigPath).lexically_normal();
    return path.parent_path().string();
}

std::string Options::GetGameConfigParentPath() const
{
    const std::filesystem::path path = std::filesystem::path(gameConfigPath);
    return path.parent_path().parent_path().string();
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
