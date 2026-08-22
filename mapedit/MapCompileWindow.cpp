//
// Created by droc101 on 11/18/25.
//

#include "MapCompileWindow.h"
#include <format>
#include <imgui.h>
#include <memory>
#include "CompileProgressWindow.h"
#include "game_sdk/WindowManager.h"
#include "MapEditor.h"

void MapCompileWindow::StartCompile()
{
    if (MapEditor::mapFile.empty())
    {
        ErrorMessage("The level must be saved before compiling");
    } else
    {
        WindowManager::Get().AddWindow(std::make_shared<CompileProgressWindow>(opts));
    }
}

const Window::WindowProperties &MapCompileWindow::GetProperties() const
{
    return properties;
}

void MapCompileWindow::Render()
{
    ImGui::SeparatorText("Compile Mode");
    if (ImGui::RadioButton("Full Compile", !opts.fastCompile))
    {
        opts.fastCompile = false;
    }
    if (ImGui::RadioButton("Fast Compile", opts.fastCompile))
    {
        opts.fastCompile = true;
    }
    ImGui::SeparatorText("Lighting Options");
    ImGui::Checkbox("Skip lighting", &opts.skipLighting);
    ImGui::SeparatorText("Debug Options");
    ImGui::Checkbox("Verbose Logging", &opts.verbose);
    ImGui::SeparatorText("Game Options");
    ImGui::Checkbox("Play after compile", &opts.playMap);
    ImGui::Dummy(ImVec2(-1, 8));
    if (ImGui::Button("Compile"))
    {
        StartCompile();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
    {
        closeRequest = true;
    }
}
