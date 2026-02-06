//
// Created by droc101 on 11/18/25.
//

#include "MapCompileWindow.h"
#include <game_sdk/DesktopInterface.h>
#include <game_sdk/Options.h>
#include <game_sdk/SDKWindow.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <SDL3/SDL_filesystem.h>
#include "MapEditor.h"

void MapCompileWindow::StartCompile()
{
    if (MapEditor::mapFile.empty())
    {
        SDKWindow::Get().ErrorMessage("The level must be saved before compiling");
    } else
    {
        const Error::ErrorCode errorCode = MapEditor::map.SaveAsMapSrc(MapEditor::mapFile.c_str());
        if (errorCode != Error::ErrorCode::OK)
        {
            log += std::format("Failed to save the level!{}\n", Error::ErrorString(errorCode));

        } else
        {
            log = "";
            std::string compilerPath = SDL_GetBasePath();
            compilerPath += "mapcomp";
#ifdef WIN32
            compilerPath += ".exe";
#endif

            const std::string srcArgument = "--map-source=" + MapEditor::mapFile;
            const std::string dirArgument = "--assets-dir=" + Options::Get().GetAssetsPath();
            const char *args[4] = {compilerPath.c_str(), srcArgument.c_str(), dirArgument.c_str(), nullptr};
            compilerProcess = SDL_CreateProcess(args, true);
            if (compilerProcess == nullptr)
            {
                log += std::format("Failed to launch compiler: {}\n", SDL_GetError());
                return;
            }
            size_t outputSize = 0;
            int exitCode = -1;
            char *output = static_cast<char *>(SDL_ReadProcess(compilerProcess, &outputSize, &exitCode));
            log += std::string(output);
            SDL_free(output);
            log += std::format("\nProcess exited with code {}", exitCode);
            SDL_DestroyProcess(compilerProcess);
            compilerProcess = nullptr;
            if (exitCode == 0 && playMap)
            {
#ifdef WIN32
                const std::string gameBinary = "/game.exe";
#else
                const std::string gameBinary = "/game";
#endif
                const std::string mapName = std::filesystem::path(MapEditor::mapFile).stem().string();
                const std::string mapArg = "--map=" + mapName;
                const std::string gameArg = "--game=" + Options::Get().GetAssetsPath();
                if (!DesktopInterface::Get().ExecuteProcessNonBlocking(Options::Get().gamePath + gameBinary,
                                                                 {mapArg.c_str(), "--nosteam", gameArg}))
                {
                    log += "Failed to execute game binary";
                }
            }
        }
    }
}

void MapCompileWindow::Show()
{
    visible = true;
}

void MapCompileWindow::Render()
{
    if (!visible)
    {
        return;
    }
    ImGui::SetNextWindowSize(ImVec2(500, -1));
    ImGui::Begin("Compile Map",
                 &visible,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking);

    ImGui::BeginDisabled(compilerProcess != nullptr);
    ImGui::Checkbox("Play after compile", &playMap);

    if (ImGui::Button("Compile"))
    {
        StartCompile();
    }
    ImGui::EndDisabled();

    if (ImGui::CollapsingHeader("Compiler Output", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::InputTextMultiline("##output",
                                  &log,
                                  ImVec2(-1, 300),
                                  ImGuiInputTextFlags_ReadOnly |
                                          ImGuiInputTextFlags_WordWrap |
                                          ImGuiInputTextFlags_NoHorizontalScroll);
    }

    ImGui::End();
}
