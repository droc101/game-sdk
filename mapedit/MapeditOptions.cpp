//
// Created by droc101 on 10/29/26.
//

#include "MapeditOptions.h"
#include <libassets/util/Error.h>
#include <libassets/util/FileIo.h>
#include <libassets/util/Logger.h>
#include <nlohmann/json.hpp>
#include <SDL3/SDL_filesystem.h>
#include <string>

void MapeditOptions::Load()
{
    const std::string path = SDL_GetBasePath() + std::string("mapedit_options.json");
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
        defaultMaterial = savedata.value("default_material", DEFAULT_MATERIAL);
        drawGrid = savedata.value("draw_grid", true);
        drawAxisHelper = savedata.value("draw_axis_helper", true);
        drawWorldBorder = savedata.value("draw_world_border", true);
        drawViewportInfo = savedata.value("draw_viewport_info", true);
        snapToGrid = savedata.value("snap_to_grid", true);
        showSidebar = savedata.value("show_sidebar", true);
        drawModels = savedata.value("draw_models", true);
        culling = savedata.value("culling", true);
        showInternalBuffers = savedata.value("show_internal_buffers", false);
        show2dViewports = savedata.value("show_2d_viewports", true);
        showInvisibleFaces = savedata.value("show_invisible_faces", true);
    }
}

void MapeditOptions::LoadDefault()
{
    defaultMaterial = DEFAULT_MATERIAL;
    drawGrid = true;
    drawAxisHelper = true;
    drawWorldBorder = true;
    drawViewportInfo = true;
    snapToGrid = true;
    showSidebar = true;
    drawModels = true;
    culling = true;
    showInternalBuffers = false;
    show2dViewports = true;
    showInvisibleFaces = true;
}


void MapeditOptions::Save()
{
    const nlohmann::json savedata = {
        {"default_material", defaultMaterial},
        {"draw_grid", drawGrid},
        {"draw_axis_helper", drawAxisHelper},
        {"draw_world_border", drawWorldBorder},
        {"draw_viewport_info", drawViewportInfo},
        {"snap_to_grid", snapToGrid},
        {"show_sidebar", showSidebar},
        {"draw_models", drawModels},
        {"culling", culling},
        {"show_internal_buffers", showInternalBuffers},
        {"show_2d_viewports", show2dViewports},
    };
    const std::string path = SDL_GetBasePath() + std::string("mapedit_options.json");
    const Error::ErrorCode writeError = FileIo::WriteStringToFile(path, savedata.dump(4));
    if (writeError != Error::ErrorCode::OK)
    {
        Logger::Error("Failed to write mapedit options to \"{}\": {}", path, writeError);
    }
}
