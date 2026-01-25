//
// Created by droc101 on 11/18/25.
//

#include "MapCompileWindow.h"
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_messagebox.h>
#include "DesktopInterface.h"
#include "MapEditor.h"
#include "Options.h"

void MapCompileWindow::StartCompile(SDL_Window *window)
{
    if (MapEditor::mapFile.empty())
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "The level must be saved before compiling", window);
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
            const std::string dirArgument = "--assets-dir=" + Options::GetAssetsPath();
            const char *args[4] = {compilerPath.c_str(), srcArgument.c_str(), dirArgument.c_str(), nullptr};
            compilerProcess = SDL_CreateProcess(args, true);
            if (compilerProcess == nullptr)
            {
                log += std::format("Failed to launch compiler: {}\n", SDL_GetError());
            }
            size_t outputSize = 0;
            int exitCode = -1;
            void *output = SDL_ReadProcess(compilerProcess, &outputSize, &exitCode);
            log += std::string(static_cast<char *>(output));
            log += std::format("\nProcess exited with code {}", exitCode);
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
                const std::string gameArg = "--game=" + Options::GetAssetsPath();
                if (!DesktopInterface::ExecuteProcessNonBlocking(Options::gamePath + gameBinary,
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

void MapCompileWindow::Render(SDL_Window *window)
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
    // ImGui::Checkbox("Custom Game Directory", &overrideGameDir);
    // ImGui::BeginDisabled(!overrideGameDir);
    // ImGui::InputText("##customAssets", &gameDir);
    // ImGui::EndDisabled();
    ImGui::Checkbox("Play after compile", &playMap);

    if (ImGui::Button("Compile"))
    {
        StartCompile(window);
    }
    ImGui::EndDisabled();

    ImGui::Separator();
    ImGui::Text("Compiler Output:");
    ImGui::InputTextMultiline("##output",
                              &log,
                              ImVec2(-1, 300),
                              ImGuiInputTextFlags_ReadOnly |
                                      ImGuiInputTextFlags_WordWrap |
                                      ImGuiInputTextFlags_NoHorizontalScroll);

    ImGui::End();
}
