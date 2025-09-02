//
// Created by droc101 on 8/9/25.
//

#include "BatchCompileWindow.h"
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_video.h>
#include <cstdio>
#include <format>
#include <imgui.h>
#include <libassets/util/Error.h>
#include <misc/cpp/imgui_stdlib.h>
#include <cstddef>
#include <SDL3/SDL_dialog.h>
#include <array>
#include <libassets/asset/ShaderAsset.h>
#include <algorithm>
#include <filesystem>
#include <string>
#include <SDL3/SDL_messagebox.h>
#include "DialogFilters.h"

void BatchCompileWindow::Show()
{
    visible = true;
    files.clear();
    types.clear();
    targetOpenGL = false;
    outputFolder = "";
}

void BatchCompileWindow::selectCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    while (*fileList != nullptr)
    {
        const std::string file = std::string(*fileList);
        if (std::ranges::find(files, file) == files.end())
        {
            files.emplace_back(file);
            ShaderAsset::ShaderType t = ShaderAsset::ShaderType::SHADER_TYPE_FRAG;
            if (file.ends_with(".frag") || file.ends_with("_f.glsl"))
            {
                t = ShaderAsset::ShaderType::SHADER_TYPE_FRAG;
            } else if (file.ends_with(".vert") || file.ends_with("_v.glsl"))
            {
                t = ShaderAsset::ShaderType::SHADER_TYPE_VERT;
            }
            types.emplace_back(t);
        }
        fileList++;
    }
}

void BatchCompileWindow::outPathCallback(void * /*userdata*/, const char *const *filelist, int /*filter*/)
{
    if (filelist == nullptr || filelist[0] == nullptr)
    {
        return;
    }
    outputFolder = filelist[0];
}

Error::ErrorCode BatchCompileWindow::Execute()
{
    if (!std::filesystem::is_directory(outputFolder))
    {
        return Error::ErrorCode::INVALID_DIRECTORY;
    }

    const ShaderAsset::ShaderPlatform plat = targetOpenGL
                                                 ? ShaderAsset::ShaderPlatform::PLATFORM_OPENGL
                                                 : ShaderAsset::ShaderPlatform::PLATFORM_VULKAN;

    for (size_t i = 0; i < files.size(); i++)
    {
        const std::string &file = files.at(i);
        const std::filesystem::path path = std::filesystem::path(file);
        const ShaderAsset::ShaderType type = types.at(i);
        ShaderAsset shd;
        Error::ErrorCode e = ShaderAsset::CreateFromGlsl(file.c_str(), shd);
        if (e != Error::ErrorCode::OK)
        {
            return e;
        }
        shd.platform = plat;
        shd.type = type;
        std::string suffix = type == ShaderAsset::ShaderType::SHADER_TYPE_FRAG ? "_f" : "_v";
        if (path.stem().string().ends_with("_f") || path.stem().string().ends_with("_v"))
        {
            suffix = "";
        }
        e = shd.SaveAsAsset((outputFolder + "/" + path.stem().string() + suffix + ".gshd").c_str());
        if (e != Error::ErrorCode::OK)
        {
            return e;
        }
    }

    return Error::ErrorCode::OK;
}


void BatchCompileWindow::Render(SDL_Window *window)
{
    if (visible)
    {
        ImGui::OpenPopup("Batch Compile");
        ImGui::SetNextWindowSize(ImVec2(600, -1), ImGuiCond_Appearing);
        constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse |
                                                 ImGuiWindowFlags_NoSavedSettings |
                                                 ImGuiWindowFlags_NoResize;
        if (ImGui::BeginPopupModal("Batch Compile", &visible, windowFlags))
        {
            ImGui::Text("Output Folder");
            ImGui::PushItemWidth(-ImGui::GetStyle().WindowPadding.x - 40);
            ImGui::InputText("##outFolder", &outputFolder);
            ImGui::SameLine();
            if (ImGui::Button("...", ImVec2(40, 0)))
            {
                SDL_ShowOpenFolderDialog(outPathCallback, nullptr, window, nullptr, false);
            }

            ImGui::Text("Rendering API");
            if (ImGui::RadioButton("Vulkan", !targetOpenGL))
            {
                targetOpenGL = false;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("OpenGL", targetOpenGL))
            {
                targetOpenGL = true;
            }

            ImGui::Text("Source Files");
            ImGui::SameLine();
            const float sz = ImGui::GetContentRegionAvail().x;
            ImGui::Dummy(ImVec2(sz - 60 - ImGui::GetStyle().WindowPadding.x, 0));
            ImGui::SameLine();
            if (ImGui::Button("Add", ImVec2(60, 0)))
            {
                SDL_ShowOpenFileDialog(selectCallback,
                                       nullptr,
                                       window,
                                       DialogFilters::glslFilters.data(),
                                       DialogFilters::glslFilters.size(),
                                       nullptr,
                                       true);
            }
            if (ImGui::BeginChild("##picker", ImVec2(-1, 250), ImGuiChildFlags_Border, 0))
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
                        ImGui::InputText(std::format("##path_{}", i).c_str(), &files.at(i));
                        ImGui::TableNextColumn();
                        if (ImGui::RadioButton(std::format("Fragment##{}", i).c_str(),
                                               types.at(i) == ShaderAsset::ShaderType::SHADER_TYPE_FRAG))
                        {
                            types.at(i) = ShaderAsset::ShaderType::SHADER_TYPE_FRAG;
                        }
                        ImGui::SameLine();
                        if (ImGui::RadioButton(std::format("Vertex##{}", i).c_str(),
                                               types.at(i) == ShaderAsset::ShaderType::SHADER_TYPE_VERT))
                        {
                            types.at(i) = ShaderAsset::ShaderType::SHADER_TYPE_VERT;
                        }
                        ImGui::TableNextColumn();
                        if (ImGui::Button(std::format("Del##{}", i).c_str(), ImVec2(40, 0)))
                        {
                            files.erase(files.begin() + static_cast<ptrdiff_t>(i));
                            files.erase(files.begin() + static_cast<ptrdiff_t>(i));
                            types.erase(types.begin() + static_cast<ptrdiff_t>(i));
                            types.erase(types.begin() + static_cast<ptrdiff_t>(i));
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
                    if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                                  "Error",
                                                  std::format("Failed to compile shaders!\n{}", e).c_str(),
                                                  window))
                    {
                        printf("Error: SDL_ShowSimpleMessageBox(): %s\n", SDL_GetError());
                    }
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
