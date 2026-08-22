//
// Created by droc101 on 8/21/26.
//

#include "CompileProgressWindow.h"
#include <array>
#include <cstddef>
#include <format>
#include <game_sdk/DesktopInterface.h>
#include <game_sdk/DialogFilters.h>
#include <game_sdk/Options.h>
#include <game_sdk/Window.h>
#include <imgui.h>
#include <libassets/util/Error.h>
#include <libassets/util/FileIo.h>
#include <libassets/util/Logger.h>
#include <SDL3/SDL_clipboard.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_process.h>
#include <SDL3/SDL_properties.h>
#include <vector>
#include "MapEditor.h"

CompileProgressWindow::CompileProgressWindow(const CompileOptions &opts)
{
    this->opts = opts;
}

const Window::WindowProperties &CompileProgressWindow::GetProperties() const
{
    return properties;
}

bool CompileProgressWindow::Init()
{
    const Error::ErrorCode errorCode = MapEditor::map.Export(MapEditor::mapFile);
    if (errorCode != Error::ErrorCode::OK)
    {
        log += std::format("Failed to save the level!{}\n", Error::ErrorString(errorCode));
        return false;
    }
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
        "--no-ansi",
    };

    if (opts.fastCompile)
    {
        arguments.emplace_back("--fast");
    }

    if (opts.skipLighting)
    {
        arguments.emplace_back("--skip-lighting");
    }
    if (opts.verbose)
    {
        arguments.emplace_back("--verbose");
    }

    compilerProcess = DesktopInterface::Get().StartSDLProcess(compilerPath, arguments);

    if (compilerProcess == nullptr)
    {
        log += std::format("Failed to launch compiler: {}\n", SDL_GetError());
        return false;
    }

    log = "Compiling map file \"" + MapEditor::mapFile + "\"...\n";

    const SDL_PropertiesID processProps = SDL_GetProcessProperties(compilerProcess);
    if (processProps != 0)
    {
        compilerOutputStream = static_cast<SDL_IOStream *>(SDL_GetPointerProperty(processProps,
                                                                                  SDL_PROP_PROCESS_STDOUT_POINTER,
                                                                                  nullptr));
        compilerErrorStream = static_cast<SDL_IOStream *>(SDL_GetPointerProperty(processProps,
                                                                                 SDL_PROP_PROCESS_STDERR_POINTER,
                                                                                 nullptr));
        SDL_DestroyProperties(processProps);
    } else
    {
        Logger::Error("Failed to get compiler properties, output will not be shown.");
    }

    return true;
}

void CompileProgressWindow::SaveLog(const std::string &path) const
{
    FileIo::WriteStringToFile(path, log);
}

void CompileProgressWindow::ProcessIOStream(SDL_IOStream **stream)
{
    if (*stream != nullptr)
    {
        std::array<char, 1024> buffer{};
        const size_t bytesRead = SDL_ReadIO(*stream, buffer.data(), 1000);
        buffer.at(bytesRead) = 0;
        log += std::string(buffer.data());
        if (bytesRead == 0)
        {
            const SDL_IOStatus status = SDL_GetIOStatus(*stream);
            if (status == SDL_IO_STATUS_EOF)
            {
                (void)SDL_CloseIO(*stream);
                *stream = nullptr;
            } else if (status != SDL_IO_STATUS_NOT_READY) // not ready just means no new output
            {
                Logger::Error("Failed to read SDL IOStream: {} \"{}\"", static_cast<int>(status), SDL_GetError());
            }
        }
    }
}

void CompileProgressWindow::FinishIOSteam(SDL_IOStream **stream)
{
    SDL_IOStatus status = SDL_GetIOStatus(*stream);
    while (status != SDL_IO_STATUS_EOF && status != SDL_IO_STATUS_ERROR)
    {
        std::array<char, 1024> buffer = {0};
        (void)SDL_ReadIO(*stream, &buffer, 1000);
        log += std::string(buffer.data());
        status = SDL_GetIOStatus(*stream);
    }
    (void)SDL_CloseIO(*stream);
    *stream = nullptr;
}

void CompileProgressWindow::ProcessCompilerOutput()
{
    if (compilerProcess != nullptr)
    {
        int exitCode = 0;
        if (SDL_WaitProcess(compilerProcess, false, &exitCode))
        {
            FinishIOSteam(&compilerOutputStream);
            FinishIOSteam(&compilerErrorStream);

            log += std::format("\nProcess exited with code {}", exitCode);

            SDL_DestroyProcess(compilerProcess);
            compilerProcess = nullptr;

            if (exitCode == 0 && opts.playMap)
            {
                const std::string mapName = std::filesystem::path(MapEditor::mapFile).stem().string();
                const std::vector<std::string> arguments = {
                    "--map=" + mapName,
                    "--game=" + Options::Get().GetAssetsPath(),
                    "--nosteam",
                    "--show-console",
                };
                if (!DesktopInterface::Get().ExecuteProcessNonBlocking(Options::Get().gameExecutablePath, arguments))
                {
                    log += "Failed to execute game binary";
                }
            }
        }
    }
    ProcessIOStream(&compilerOutputStream);
    ProcessIOStream(&compilerErrorStream);
}

void CompileProgressWindow::Render()
{
    ImGui::PushFont(GetMonospaceFont(), 18);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1, 0.1, 0.1, 1));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0, 0, 0, 0));
    if (ImGui::BeginChild("scrolling",
                          ImVec2(-1, ImGui::GetWindowSize().y - 70),
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
    if (compilerProcess != nullptr || compilerOutputStream != nullptr || compilerErrorStream != nullptr)
    {
        ImGui::ProgressBar(static_cast<float>(ImGui::GetTime()) * -0.5f, ImVec2(-108, 0), "Compiling...");
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0)))
        {
            (void)SDL_KillProcess(compilerProcess, false);
        }
    } else
    {
        if (ImGui::Button("Copy Output"))
        {
            (void)SDL_SetClipboardText(log.c_str());
        }
        ImGui::SameLine();
        if (ImGui::Button("Save Output"))
        {
            SaveFileDialog(FILE_DIALOG_CALLBACK(SaveLog), DialogFilters::LOG_FILTERS);
        }
        ImGui::SameLine();
        if (ImGui::Button("OK") || ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteGlobal))
        {
            RequestClose();
        }
    }

    ProcessCompilerOutput();
}
