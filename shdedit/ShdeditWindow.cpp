//
// Created by droc101 on 8/21/26.
//

#include "ShdeditWindow.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <format>
#include <game_sdk/DialogFilters.h>
#include <game_sdk/SharedMgr.h>
#include <game_sdk/Window.h>
#include <imgui.h>
#include <iterator>
#include <libassets/asset/ShaderAsset.h>
#include <libassets/util/Error.h>
#include <libassets/util/SearchPathManager.h>
#include <map>
#include <misc/cpp/imgui_stdlib.h>
#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_video.h>
#include <string>
#include <utility>
#include <vector>

const Window::WindowProperties &ShdeditWindow::GetProperties() const
{
    return properties;
}

void ShdeditWindow::SelectCallback(const std::vector<std::string> &paths)
{
    for (const std::string &file: paths)
    {
        if (std::ranges::find(files, file) == files.end())
        {
            if (file.ends_with(".inc.glsl"))
            {
                SDL_Window *w = GetWindow();
                constexpr std::array<SDL_MessageBoxButtonData, 2> BUTTONS = {
                    SDL_MessageBoxButtonData{
                        .flags = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
                        .buttonID = 0,
                        .text = "Yes",
                    },
                    SDL_MessageBoxButtonData{
                        .flags = SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
                        .buttonID = 1,
                        .text = "No",
                    },
                };
                const std::string message = std::format("The file \"{}\" appears to be an include file which cannot be "
                                                        "compiled standalone. Are you sure you want to add it?",
                                                        file);
                const SDL_MessageBoxData mbox = {
                    .flags = SDL_MESSAGEBOX_WARNING,
                    .window = w,
                    .title = nullptr,
                    .message = message.c_str(),
                    .numbuttons = 2,
                    .buttons = BUTTONS.data(),
                    .colorScheme = nullptr,
                };
                int chosenButton = 0;
                (void)SDL_ShowMessageBox(&mbox, &chosenButton);
                if (chosenButton == 1)
                {
                    continue;
                }
            }

            files.emplace_back(file);
            ShaderAsset::ShaderType kind = ShaderAsset::ShaderType::SHADER_KIND_FRAGMENT;
            if (file.ends_with(".frag") || file.ends_with("_f.glsl"))
            {
                kind = ShaderAsset::ShaderType::SHADER_KIND_FRAGMENT;
            } else if (file.ends_with(".vert") || file.ends_with("_v.glsl"))
            {
                kind = ShaderAsset::ShaderType::SHADER_KIND_VERTEX;
            } else if (file.ends_with(".comp") || file.ends_with("_c.glsl"))
            {
                kind = ShaderAsset::ShaderType::SHADER_KIND_COMPUTE;
            } else if (file.ends_with(".geom") || file.ends_with("_g.glsl"))
            {
                kind = ShaderAsset::ShaderType::SHADER_KIND_GEOMETRY;
            }
            types.emplace_back(kind);
        }
    }
}

void ShdeditWindow::OutPathCallback(const std::string &path)
{
    outputFolder = path;
}

void ShdeditWindow::BasePathCallback(const std::string &path)
{
    sourcesBaseFolder = path;
}

void ShdeditWindow::AddFolderCallback(const std::string &folder)
{
    const std::vector<std::string> vertexSources = SearchPathManager::ScanFolder(folder, ".vert", true);
    const std::vector<std::string> fragmentSources = SearchPathManager::ScanFolder(folder, ".frag", true);
    const std::vector<std::string> computeSources = SearchPathManager::ScanFolder(folder, ".comp", true);
    const std::vector<std::string> geometrySources = SearchPathManager::ScanFolder(folder, ".geom", true);

    std::vector<std::string> sourcePaths{};
    sourcePaths.reserve(vertexSources.size() + fragmentSources.size() + computeSources.size() + geometrySources.size());
    std::ranges::copy(vertexSources, std::back_inserter(sourcePaths));
    std::ranges::copy(fragmentSources, std::back_inserter(sourcePaths));
    std::ranges::copy(computeSources, std::back_inserter(sourcePaths));
    std::ranges::copy(geometrySources, std::back_inserter(sourcePaths));

    for (std::string &path: sourcePaths)
    {
        if (folder.ends_with('/') || folder.ends_with('\\'))
        {
            path = folder + path;
        } else
        {
            path = folder + '/' + path;
        }
    }

    std::ranges::sort(sourcePaths, [](const std::string &a, const std::string &b) {
        return std::filesystem::path(a).filename().string() < std::filesystem::path(b).filename().string();
    });

    SelectCallback(sourcePaths);
}

Error::ErrorCode ShdeditWindow::Execute(std::string &errorLog)
{
    if (!std::filesystem::is_directory(outputFolder))
    {
        return Error::ErrorCode::INVALID_DIRECTORY;
    }

    for (size_t i = 0; i < files.size(); i++)
    {
        const std::string &file = files.at(i);
        const ShaderAsset::ShaderType kind = types.at(i);
        ShaderAsset shader;
        Error::ErrorCode e = shader.Import(file);
        if (e != Error::ErrorCode::OK)
        {
            return e;
        }
        shader.type = kind;
        std::string suffix;
        switch (shader.type)
        {
            case ShaderAsset::ShaderType::SHADER_KIND_FRAGMENT:
                suffix = "f";
                break;
            case ShaderAsset::ShaderType::SHADER_KIND_VERTEX:
                suffix = "v";
                break;
            case ShaderAsset::ShaderType::SHADER_KIND_COMPUTE:
                suffix = "c";
                break;
            case ShaderAsset::ShaderType::SHADER_KIND_GEOMETRY:
                suffix = "g";
                break;
        }

        const std::string filename = std::filesystem::path(file).stem().string();
        std::string outputPath = std::format("{}/{}_{}.{}",
                                             outputFolder,
                                             filename,
                                             suffix,
                                             ShaderAsset::SHADER_ASSET_EXTENSION);
        if (replicateFolderStructure)
        {
            if (!sourcesBaseFolder.empty() && file.starts_with(sourcesBaseFolder))
            {
                std::string relativePath = file.substr(sourcesBaseFolder.length());
                outputPath = std::format("{}/{}_{}.{}",
                                         outputFolder,
                                         relativePath,
                                         suffix,
                                         ShaderAsset::SHADER_ASSET_EXTENSION);
                auto a = outputFolder + "/" + std::filesystem::path(relativePath).parent_path().string();
                std::filesystem::create_directories(std::filesystem::path(a));
                asm("nop");
            }
        }

        e = shader.SaveToAssetEx(outputPath, enableOptimization, &errorLog, file, dumpBinaries);
        if (e != Error::ErrorCode::OK)
        {
            return e;
        }
    }

    return Error::ErrorCode::OK;
}

void ShdeditWindow::Render()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Quit", "Alt+F4"))
            {
                RequestClose();
            }
            ImGui::EndMenu();
        }
        SharedMgr::Get().SharedMenuUI("shdedit");
        ImGui::EndMainMenuBar();
    }

    ImGui::Text("Output folder");
    ImGui::PushItemWidth(-ImGui::GetStyle().WindowPadding.x - 40);
    ImGui::InputText("##outFolder", &outputFolder);
    ImGui::SameLine();
    if (ImGui::Button("...", ImVec2(40, 0)))
    {
        OpenFolderDialog(FILE_DIALOG_CALLBACK(OutPathCallback));
    }

    ImGui::Checkbox("Replicate source code folder structure", &replicateFolderStructure);
    ImGui::BeginDisabled(!replicateFolderStructure);
    ImGui::PushItemWidth(-ImGui::GetStyle().WindowPadding.x - 40);
    ImGui::InputTextWithHint("##sourcesBaseFolder", "Source code folder", &sourcesBaseFolder);
    ImGui::SameLine();
    if (ImGui::Button("...##2", ImVec2(40, 0)))
    {
        OpenFolderDialog(FILE_DIALOG_CALLBACK(BasePathCallback));
    }
    ImGui::EndDisabled();

    ImGui::Dummy({1, 3});
    ImGui::Separator();
    ImGui::Dummy({1, 3});

    (void)ImGui::Checkbox("Enable optimization", &enableOptimization);
    ImGui::SameLine();
    (void)ImGui::Checkbox("Dump SPIR-V binaries", &dumpBinaries);

    ImGui::Dummy({1, 3});
    ImGui::Separator();
    ImGui::Dummy({1, 3});

    ImGui::Text("Source files");
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - 80 - 80 - ImGui::GetStyle().WindowPadding.x * 2, 0));
    ImGui::SameLine();
    if (ImGui::Button("Add##src", ImVec2(80, 0)))
    {
        OpenMultiFileDialog(MULTI_FILE_DIALOG_CALLBACK(SelectCallback), DialogFilters::GLSL_FILTERS);
    }
    ImGui::SameLine();
    if (ImGui::Button("Add Folder##src", ImVec2(80, 0)))
    {
        OpenFolderDialog(FILE_DIALOG_CALLBACK(AddFolderCallback));
    }
    if (ImGui::BeginChild("##picker", ImVec2(-1, -40), ImGuiChildFlags_Borders, 0))
    {
        const ImVec2 availSize = ImGui::GetContentRegionAvail();
        if (ImGui::BeginTable("fileTable", 3, ImGuiTableFlags_ScrollY, availSize))
        {
            ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("",
                                    ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed,
                                    40 + ImGui::GetStyle().WindowPadding.x);
            ImGui::TableHeadersRow();
            for (size_t i = 0; i < files.size(); i++)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::PushItemWidth(-1);

                std::string filename = files.at(i);
                if (replicateFolderStructure)
                {
                    if (filename.starts_with(sourcesBaseFolder) && sourcesBaseFolder.length() != 0)
                    {
                        filename = filename.substr(sourcesBaseFolder.length());
                    } else
                    {
                        ImGui::TextColored({1, 0, 0, 1}, "(outside of source tree)");
                        ImGui::SameLine();
                    }
                } else
                {
                    filename = std::filesystem::path(filename).filename().string();
                }

                ImGui::PushFont(GetMonospaceFont(), 0.0f);
                ImGui::Text("%s", filename.c_str());
                ImGui::PopFont();
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", files.at(i).c_str());
                }
                ImGui::TableNextColumn();
                const std::map<ShaderAsset::ShaderType, std::string> typeNames = {
                    {ShaderAsset::ShaderType::SHADER_KIND_FRAGMENT, "Fragment"},
                    {ShaderAsset::ShaderType::SHADER_KIND_VERTEX, "Vertex"},
                    {ShaderAsset::ShaderType::SHADER_KIND_COMPUTE, "Compute"},
                    {ShaderAsset::ShaderType::SHADER_KIND_GEOMETRY, "Geometry"},
                };

                ImGui::PushItemWidth(150);
                if (ImGui::BeginCombo(std::format("##type{}", i).c_str(), typeNames.at(types.at(i)).c_str()))
                {
                    for (const std::pair<const ShaderAsset::ShaderType, std::string> &typePair: typeNames)
                    {
                        if (ImGui::Selectable(typePair.second.c_str(), types.at(i) == typePair.first))
                        {
                            types.at(i) = typePair.first;
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::TableNextColumn();
                if (ImGui::Button(std::format("Del##src{}", i).c_str(), ImVec2(40, 0)))
                {
                    files.erase(files.begin() + static_cast<ptrdiff_t>(i));
                    types.erase(types.begin() + static_cast<ptrdiff_t>(i));
                }
            }
            ImGui::EndTable();
        }

        ImGui::EndChild();
    }

    ImGui::Dummy({1, 8});

    if (ImGui::Button("Compile", ImVec2(-1, 0)))
    {
        if (outputFolder.empty())
        {
            ErrorMessage("The output directory must be specified.", "Error");
        } else if (files.empty())
        {
            ErrorMessage("No source files provided", "Error");
        } else
        {
            std::string compileLog;
            Error::ErrorCode e = Execute(compileLog);
            if (e == Error::ErrorCode::OK)
            {
                InfoMessage("Successfully compiled", "");
            } else
            {
                ErrorMessage(std::format("Failed to compile shaders: {}\n\nCompiler "
                                         "log:\n{}\n\nCompilation terminated.",
                                         e,
                                         compileLog.empty() ? "(empty)" : compileLog));
            }
        }
    }
}
