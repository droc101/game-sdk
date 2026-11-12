//
// Created by droc101 on 11/18/25.
//

#include "MapCompileWindow.h"
#include <game_sdk/Window.h>
#include <game_sdk/WindowManager.h>
#include <imgui.h>
#include <memory>
#include "CompileProgressWindow.h"
#include "MapEditor.h"

void MapCompileWindow::StartCompile()
{
    if (MapEditor::mapFile.empty())
    {
        ErrorMessage("The level must be saved before compiling");
    } else
    {
        WindowManager::Get().AddModalWindow<CompileProgressWindow>(opts);
    }
}

const Window::WindowProperties &MapCompileWindow::GetProperties() const
{
    return properties;
}

void MapCompileWindow::Render()
{
    ImGui::SeparatorText("Compile Mode");
    if (ImGui::RadioButton("Full (Slow!)", opts.compileMode == CompileProgressWindow::CompileMode::FULL))
    {
        opts.compileMode = CompileProgressWindow::CompileMode::FULL;
    }
    if (ImGui::RadioButton("Normal", opts.compileMode == CompileProgressWindow::CompileMode::NORMAL))
    {
        opts.compileMode = CompileProgressWindow::CompileMode::NORMAL;
    }
    if (ImGui::RadioButton("Fast", opts.compileMode == CompileProgressWindow::CompileMode::FAST))
    {
        opts.compileMode = CompileProgressWindow::CompileMode::FAST;
    }
    if (ImGui::RadioButton("Skip Lighting", opts.compileMode == CompileProgressWindow::CompileMode::SKIP_LIGHTING))
    {
        opts.compileMode = CompileProgressWindow::CompileMode::SKIP_LIGHTING;
    }
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
        RequestClose();
    }
}
