//
// Created by droc101 on 7/23/25.
//

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <format>
#include <game_sdk/DialogFilters.h>
#include <game_sdk/SDKWindow.h>
#include <game_sdk/SharedMgr.h>
#include <imgui.h>
#include <libassets/asset/ShaderAsset.h>
#include <libassets/util/Error.h>
#include <map>
#include <misc/cpp/imgui_stdlib.h>
#include <string>
#include <utility>
#include <vector>

static std::vector<std::string> files;
static std::vector<ShaderAsset::ShaderKind> types;

static std::string outputFolder;
static bool enableOptimization;

static void SelectCallback(const std::vector<std::string> &paths)
{
    for (const std::string &file: paths)
    {
        if (std::ranges::find(files, file) == files.end())
        {
            files.emplace_back(file);
            ShaderAsset::ShaderKind kind = ShaderAsset::ShaderKind::SHADER_KIND_FRAGMENT;
            if (file.ends_with(".frag") || file.ends_with("_f.glsl"))
            {
                kind = ShaderAsset::ShaderKind::SHADER_KIND_FRAGMENT;
            } else if (file.ends_with(".vert") || file.ends_with("_v.glsl"))
            {
                kind = ShaderAsset::ShaderKind::SHADER_KIND_VERTEX;
            } else if (file.ends_with(".comp") || file.ends_with("_c.glsl"))
            {
                kind = ShaderAsset::ShaderKind::SHADER_KIND_COMPUTE;
            } else if (file.ends_with(".geom") || file.ends_with("_g.glsl"))
            {
                kind = ShaderAsset::ShaderKind::SHADER_KIND_GEOMETRY;
            }
            types.emplace_back(kind);
        }
    }
}

static void OutPathCallback(const std::string &path)
{
    outputFolder = path;
}

static Error::ErrorCode Execute(std::string &errorLog)
{
    if (!std::filesystem::is_directory(outputFolder))
    {
        return Error::ErrorCode::INVALID_DIRECTORY;
    }

    for (size_t i = 0; i < files.size(); i++)
    {
        const std::string &file = files.at(i);
        const std::string filename = std::filesystem::path(file).stem().string();
        const ShaderAsset::ShaderKind kind = types.at(i);
        ShaderAsset shader;
        Error::ErrorCode e = shader.Import(file);
        if (e != Error::ErrorCode::OK)
        {
            return e;
        }
        shader.kind = kind;
        std::string suffix;
        switch (shader.kind)
        {
            case ShaderAsset::ShaderKind::SHADER_KIND_FRAGMENT:
                suffix = "f";
                break;
            case ShaderAsset::ShaderKind::SHADER_KIND_VERTEX:
                suffix = "v";
                break;
            case ShaderAsset::ShaderKind::SHADER_KIND_COMPUTE:
                suffix = "c";
                break;
            case ShaderAsset::ShaderKind::SHADER_KIND_GEOMETRY:
                suffix = "g";
                break;
        }
        e = shader.SaveToAssetEx(std::format("{}/{}_{}.{}",
                                             outputFolder,
                                             filename,
                                             suffix,
                                             ShaderAsset::SHADER_ASSET_EXTENSION),
                                 enableOptimization,
                                 &errorLog,
                                 file);
        if (e != Error::ErrorCode::OK)
        {
            return e;
        }
    }

    return Error::ErrorCode::OK;
}

static void Render()
{
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    constexpr ImGuiWindowFlags WINDOW_FLAGS = ImGuiWindowFlags_NoDecoration |
                                              ImGuiWindowFlags_NoMove |
                                              ImGuiWindowFlags_NoSavedSettings |
                                              ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("shdedit", nullptr, WINDOW_FLAGS);

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Quit", "Alt+F4"))
            {
                SDKWindow::Get().PostQuit();
            }
            ImGui::EndMenu();
        }
        SharedMgr::Get().SharedMenuUI("shdedit");
        ImGui::EndMainMenuBar();
    }

    ImGui::Text("Output Folder");
    ImGui::PushItemWidth(-ImGui::GetStyle().WindowPadding.x - 40);
    ImGui::InputText("##outFolder", &outputFolder);
    ImGui::SameLine();
    if (ImGui::Button("...", ImVec2(40, 0)))
    {
        SDKWindow::Get().OpenFolderDialog(OutPathCallback);
    }

    (void)ImGui::Checkbox("Enable Optimization", &enableOptimization);

    ImGui::Separator();

    ImGui::Text("Source Files");
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - 60 - ImGui::GetStyle().WindowPadding.x, 0));
    ImGui::SameLine();
    if (ImGui::Button("Add##src", ImVec2(60, 0)))
    {
        SDKWindow::Get().OpenMultiFileDialog(SelectCallback, DialogFilters::GLSL_FILTERS);
    }
    if (ImGui::BeginChild("##picker", ImVec2(-1, -32), ImGuiChildFlags_Borders, 0))
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
                const std::map<ShaderAsset::ShaderKind, std::string> typeNames = {
                    {ShaderAsset::ShaderKind::SHADER_KIND_FRAGMENT, "Fragment"},
                    {ShaderAsset::ShaderKind::SHADER_KIND_VERTEX, "Vertex"},
                    {ShaderAsset::ShaderKind::SHADER_KIND_COMPUTE, "Compute"},
                    {ShaderAsset::ShaderKind::SHADER_KIND_GEOMETRY, "Geometry"},
                };

                ImGui::PushItemWidth(150);
                if (ImGui::BeginCombo(std::format("##type{}", i).c_str(), typeNames.at(types.at(i)).c_str()))
                {
                    for (const std::pair<const ShaderAsset::ShaderKind, std::string> &typePair: typeNames)
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

    ImGui::Separator();

    if (ImGui::Button("Compile", ImVec2(-1, 0)))
    {
        if (outputFolder.empty())
        {
            SDKWindow::Get().ErrorMessage("The output directory must be specified.", "Error");
        } else if (files.empty())
        {
            SDKWindow::Get().ErrorMessage("No source files provided", "Error");
        } else
        {
            std::string compileLog;
            Error::ErrorCode e = Execute(compileLog);
            if (e == Error::ErrorCode::OK)
            {
                SDKWindow::Get().InfoMessage("Successfully compiled", "");
            } else
            {
                SDKWindow::Get().ErrorMessage(std::format("Failed to compile shaders: {}\n\nCompiler "
                                                          "log:\n{}\n\nCompilation terminated.",
                                                          e,
                                                          compileLog.empty() ? "(empty)" : compileLog));
            }
        }
    }
    ImGui::End();
}

int main(const int argc, char **argv)
{
    if (!SDKWindow::Get().Init("GAME SDK Shader Editor", {800, 600}))
    {
        return -1;
    }

    SDKWindow::Get().SetWindowIcon("shdedit");

    SDKWindow::Get().MainLoop(Render);

    SDKWindow::Get().Destroy();

    return 0;
}
