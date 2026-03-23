//
// Created by droc101 on 11/18/25.
//

#include "MapCompileWindow.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <format>
#include <fstream>
#include <game_sdk/DesktopInterface.h>
#include <game_sdk/DialogFilters.h>
#include <game_sdk/Options.h>
#include <game_sdk/SDKWindow.h>
#include <imgui.h>
#include <libassets/util/Error.h>
#include <SDL3/SDL_clipboard.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_process.h>
#include <vector>
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

            std::vector<std::string> arguments = {
                "--map-source=" + MapEditor::mapFile,
                "--assets-dir=" + Options::Get().GetAssetsPath(),
                "--executable-dir=" + Options::Get().GetExecutablePath(),
            };
            if (bakeOnCpu)
            {
                arguments.emplace_back("--bake-on-cpu");
            }
            if (skipLighting)
            {
                arguments.emplace_back("--skip-lighting");
            }

            compilerProcess = DesktopInterface::Get().StartSDLProcess(compilerPath, arguments);

            if (compilerProcess == nullptr)
            {
                log += std::format("Failed to launch compiler: {}\n", SDL_GetError());
                return;
            }

            log = "Compiling map file \"" + MapEditor::mapFile + "\"...\n";

            compilerOutputStream = SDL_GetProcessOutput(compilerProcess);
        }
    }
}

void MapCompileWindow::SaveLog(const std::string &path)
{
    std::ofstream out(path);
    if (!out.is_open())
    {
        SDKWindow::Get().ErrorMessage("Failed to open log file \"" + path + "\".");
        return;
    }
    out << log;
    out.close();
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

    if (compilerProcess == nullptr || compilerOutputStream == nullptr)
    {
        ImGui::Checkbox("Play after compile", &playMap);
        ImGui::SameLine();
        ImGui::Checkbox("CPU light baking", &bakeOnCpu);
        ImGui::SameLine();
        ImGui::Checkbox("Skip Lighting", &skipLighting);

        if (ImGui::Button("Compile"))
        {
            StartCompile();
        }
    } else
    {
        ImGui::ProgressBar(static_cast<float>(ImGui::GetTime()) * -0.5f, ImVec2(-1, 0), "Compiling...");
    }
    if (!log.empty())
    {
        ImGui::Separator();
        ImGui::PushFont(SDKWindow::Get().GetMonospaceFont(), 18);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1, 0.1, 0.1, 1));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0, 0, 0, 0));
        if (ImGui::BeginChild("scrolling",
                              ImVec2(-1, 300),
                              ImGuiChildFlags_Borders,
                              ImGuiWindowFlags_HorizontalScrollbar))
        {
            ImGui::TextUnformatted(log.c_str());
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            {
                ImGui::SetScrollHereY(1.0f);
            }
        }
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::EndChild();
        ImGui::PopFont();
        ImGui::Separator();
        if (ImGui::Button("Copy Output"))
        {
            SDL_SetClipboardText(log.c_str());
        }
        ImGui::SameLine();
        if (ImGui::Button("Save Output"))
        {
            SDKWindow::Get().SaveFileDialog(SaveLog, DialogFilters::logFilters);
        }
    }

    ImGui::End();

    if (compilerProcess != nullptr)
    {
        int exitCode = 0;
        if (SDL_WaitProcess(compilerProcess, false, &exitCode))
        {
            size_t logSize = 0;
            char *output = static_cast<char *>(SDL_ReadProcess(compilerProcess, &logSize, nullptr));
            log += std::string(output);

            log += std::format("\nProcess exited with code {}", exitCode);
            SDL_CloseIO(compilerOutputStream);
            compilerOutputStream = nullptr;
            SDL_DestroyProcess(compilerProcess);
            compilerProcess = nullptr;

            if (exitCode == 0 && playMap)
            {
                const std::string mapName = std::filesystem::path(MapEditor::mapFile).stem().string();
                const std::string mapArg = "--map=" + mapName;
                const std::string gameArg = "--game=" + Options::Get().GetAssetsPath();
                if (!DesktopInterface::Get().ExecuteProcessNonBlocking(Options::Get().gameExecutablePath,
                                                                       {mapArg.c_str(), "--nosteam", gameArg}))
                {
                    log += "Failed to execute game binary";
                }
            }
        }
    }

    if (compilerOutputStream != nullptr)
    {
        std::array<char, 1024> buffer{};
        const size_t bytesRead = SDL_ReadIO(compilerOutputStream, buffer.data(), 1000);
        buffer.at(bytesRead) = 0;
        log += std::string(buffer.data());
        if (bytesRead == 0)
        {
            const SDL_IOStatus status = SDL_GetIOStatus(compilerOutputStream);
            if (status == SDL_IO_STATUS_EOF)
            {
                SDL_CloseIO(compilerOutputStream);
                compilerOutputStream = nullptr;
            } else if (status != SDL_IO_STATUS_NOT_READY) // not ready just means no new output
            {
                printf("Failed to read SDL IOStream: %d \"%s\"\n", status, SDL_GetError());
            }
        }
    }
}
