//
// Created by droc101 on 8/9/25.
//

#include "BatchDecompileWindow.h"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <format>
#include <game_sdk/DialogFilters.h>
#include <game_sdk/SDKWindow.h>
#include <imgui.h>
#include <libassets/asset/ShaderAsset.h>
#include <libassets/util/Error.h>
#include <misc/cpp/imgui_stdlib.h>
#include <string>

void BatchDecompileWindow::Show()
{
    visible = true;
    files.clear();
    outputFolder = "";
}

void BatchDecompileWindow::selectCallback(const std::vector<std::string> &paths)
{
    for (const std::string &file: paths)
    {
        if (std::ranges::find(files, file) == files.end())
        {
            files.emplace_back(file);
        }
    }
}

void BatchDecompileWindow::outPathCallback(const std::string &path)
{
    outputFolder = path;
}

Error::ErrorCode BatchDecompileWindow::Execute()
{
    if (!std::filesystem::is_directory(outputFolder))
    {
        return Error::ErrorCode::INVALID_DIRECTORY;
    }

    for (const std::string &file: files)
    {
        const std::string filename = std::filesystem::path(file).filename().string();
        ShaderAsset shd;
        Error::ErrorCode e = ShaderAsset::CreateFromAsset(file.c_str(), shd);
        if (e != Error::ErrorCode::OK)
        {
            return e;
        }
        if (shd.type == ShaderAsset::ShaderType::SHADER_TYPE_FRAGMENT)
        {
            const size_t suffixPosition = filename.rfind("_f." + ShaderAsset::SHADER_ASSET_EXTENSION);
            e = shd.SaveAsGlsl(std::format("{}/{}.frag", outputFolder, filename.substr(0, suffixPosition)).c_str());
        } else if (shd.type == ShaderAsset::ShaderType::SHADER_TYPE_VERTEX)
        {
            const size_t suffixPosition = filename.rfind("_v." + ShaderAsset::SHADER_ASSET_EXTENSION);
            e = shd.SaveAsGlsl(std::format("{}/{}.vert", outputFolder, filename.substr(0, suffixPosition)).c_str());
        } else if (shd.type == ShaderAsset::ShaderType::SHADER_TYPE_COMPUTE)
        {
            const size_t suffixPosition = filename.rfind("_c." + ShaderAsset::SHADER_ASSET_EXTENSION);
            e = shd.SaveAsGlsl(std::format("{}/{}.comp", outputFolder, filename.substr(0, suffixPosition)).c_str());
        }
        if (e != Error::ErrorCode::OK)
        {
            return e;
        }
    }

    return Error::ErrorCode::OK;
}


void BatchDecompileWindow::Render()
{
    if (visible)
    {
        ImGui::OpenPopup("Batch Decompile");
        ImGui::SetNextWindowSize(ImVec2(600, -1), ImGuiCond_Appearing);
        constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse |
                                                 ImGuiWindowFlags_NoSavedSettings |
                                                 ImGuiWindowFlags_NoResize;
        if (ImGui::BeginPopupModal("Batch Decompile", &visible, windowFlags))
        {
            ImGui::Text("Output Folder");
            ImGui::PushItemWidth(-ImGui::GetStyle().WindowPadding.x - 40);
            ImGui::InputText("##outFolder", &outputFolder);
            ImGui::SameLine();
            if (ImGui::Button("...", ImVec2(40, 0)))
            {
                SDKWindow::OpenFolderDialog(outPathCallback);
            }

            ImGui::Text("Compiled Shader Files");
            ImGui::SameLine();
            const float sz = ImGui::GetContentRegionAvail().x;
            ImGui::Dummy(ImVec2(sz - 60 - ImGui::GetStyle().WindowPadding.x, 0));
            ImGui::SameLine();
            if (ImGui::Button("Add", ImVec2(60, 0)))
            {
                SDKWindow::OpenMultiFileDialog(selectCallback, DialogFilters::gshdFilters);
            }
            if (ImGui::BeginChild("##picker", ImVec2(-1, 250), ImGuiChildFlags_Borders, 0))
            {
                const ImVec2 availSize = ImGui::GetContentRegionAvail();
                if (ImGui::BeginTable("fileTable", 2, ImGuiTableFlags_ScrollY, availSize))
                {
                    ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("",
                                            ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed,
                                            40 + ImGui::GetStyle().WindowPadding.x);
                    ImGui::TableHeadersRow();
                    for (size_t i = 0; i < files.size(); i++)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::PushItemWidth(-1);
                        ImGui::InputText(std::format("##path_{}", i).c_str(), &files.at(i));
                        ImGui::TableNextColumn();
                        if (ImGui::Button(std::format("Del##{}", i).c_str(), ImVec2(40, 0)))
                        {
                            files.erase(files.begin() + static_cast<ptrdiff_t>(i));
                            files.erase(files.begin() + static_cast<ptrdiff_t>(i));
                        }
                    }
                    ImGui::EndTable();
                }

                ImGui::EndChild();
            }

            const float sizeX = ImGui::GetContentRegionAvail().x;

            ImGui::Dummy(ImVec2(sizeX - 120 - ImGui::GetStyle().WindowPadding.x * 2, 0));
            ImGui::SameLine();
            if (ImGui::Button("OK", ImVec2(60, 0)))
            {
                Error::ErrorCode e = Execute();
                if (e == Error::ErrorCode::OK)
                {
                    visible = false;
                } else
                {
                    SDKWindow::ErrorMessage(std::format("Failed to decompile shaders!\n{}", e));
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(60, 0)))
            {
                visible = false;
            }

            ImGui::EndPopup();
        }
    }
}
