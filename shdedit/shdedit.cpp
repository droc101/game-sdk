//
// Created by droc101 on 7/23/25.
//

#include <array>
#include <cstdio>
#include <format>
#include <game_sdk/DesktopInterface.h>
#include <game_sdk/DialogFilters.h>
#include <game_sdk/SDKWindow.h>
#include <game_sdk/SharedMgr.h>
#include <imgui.h>
#include <libassets/asset/ShaderAsset.h>
#include <libassets/util/Error.h>
#include <misc/cpp/imgui_stdlib.h>
#include <string>
#include "BatchCompileWindow.h"
#include "BatchDecompileWindow.h"

static ShaderAsset shader{};

static void openGshd(const std::string &path)
{
    const Error::ErrorCode errorCode = ShaderAsset::CreateFromAsset(path.c_str(), shader);
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to open the shader!\n{}", errorCode));
        return;
    }
}

static void importGlsl(const std::string &path)
{
    const Error::ErrorCode errorCode = ShaderAsset::CreateFromGlsl(path.c_str(), shader);
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to import the shader!\n{}", errorCode));
        return;
    }
}

static void saveGshd(const std::string &path)
{
    const Error::ErrorCode errorCode = shader.SaveAsAsset(path.c_str());
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to save the shader!\n{}", errorCode));
    }
}

static void exportGlsl(const std::string &path)
{
    const Error::ErrorCode errorCode = shader.SaveAsGlsl(path.c_str());
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to export the shader!\n{}", errorCode));
    }
}

static void Render()
{
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration |
                                             ImGuiWindowFlags_NoMove |
                                             ImGuiWindowFlags_NoSavedSettings |
                                             ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("shdedit", nullptr, windowFlags);
    bool newPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N);
    bool openPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O);
    bool importPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_O);
    bool savePressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S);
    bool exportPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S);

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            newPressed |= ImGui::MenuItem("New", "Ctrl+N");
            ImGui::Separator();
            openPressed |= ImGui::MenuItem("Open", "Ctrl+O");
            importPressed |= ImGui::MenuItem("Import", "Ctrl+Shift+O");
            savePressed |= ImGui::MenuItem("Save", "Ctrl+S");
            exportPressed |= ImGui::MenuItem("Export", "Ctrl+Shift+S");
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4"))
            {
                SDKWindow::Get().PostQuit();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tools"))
        {
            if (ImGui::MenuItem("Batch Compile"))
            {
                BatchCompileWindow::Show();
            }
            if (ImGui::MenuItem("Batch Decompile"))
            {
                BatchDecompileWindow::Show();
            }
            ImGui::Separator();
            ImGui::EndMenu();
        }
        SharedMgr::Get().SharedMenuUI("shdedit");
        ImGui::EndMainMenuBar();
    }

    if (openPressed)
    {
        SDKWindow::Get().OpenFileDialog(openGshd, DialogFilters::gshdFilters);
    } else if (importPressed)
    {
        SDKWindow::Get().OpenFileDialog(importGlsl, DialogFilters::glslFilters);
    } else if (savePressed)
    {
        SDKWindow::Get().SaveFileDialog(saveGshd, DialogFilters::gshdFilters);
    } else if (exportPressed)
    {
        SDKWindow::Get().SaveFileDialog(exportGlsl, DialogFilters::glslFilters);
    } else if (newPressed)
    {
        shader = ShaderAsset();
    }

    const ImVec2 &availableSize = ImGui::GetContentRegionAvail();

    constexpr float statsWidth = 150.0f;
    const float imageWidth = availableSize.x - statsWidth - 8.0f;

    ImGui::BeginChild("ImagePane",
                      ImVec2(imageWidth, availableSize.y),
                      ImGuiChildFlags_Borders,
                      ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus);
    {
        ImGui::PushFont(SDKWindow::Get().GetMonospaceFont(), 18);
        ImGui::InputTextMultiline("##glsl", &shader.GetGLSL(), ImVec2(-1, -1), ImGuiInputTextFlags_AllowTabInput);
        ImGui::PopFont();
    }
    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChild("StatsPane", ImVec2(statsWidth, availableSize.y));
    {
        ImGui::TextUnformatted("Platform");
        if (ImGui::RadioButton("Vulkan", shader.platform == ShaderAsset::ShaderPlatform::PLATFORM_VULKAN))
        {
            shader.platform = ShaderAsset::ShaderPlatform::PLATFORM_VULKAN;
        }
        if (ImGui::RadioButton("OpenGL", shader.platform == ShaderAsset::ShaderPlatform::PLATFORM_OPENGL))
        {
            shader.platform = ShaderAsset::ShaderPlatform::PLATFORM_OPENGL;
            if (shader.type == ShaderAsset::ShaderType::SHADER_TYPE_COMPUTE)
            {
                shader.type = ShaderAsset::ShaderType::SHADER_TYPE_FRAGMENT;
            }
        }

        ImGui::TextUnformatted("Type");
        if (ImGui::RadioButton("Fragment", shader.type == ShaderAsset::ShaderType::SHADER_TYPE_FRAGMENT))
        {
            shader.type = ShaderAsset::ShaderType::SHADER_TYPE_FRAGMENT;
        }
        if (ImGui::RadioButton("Vertex", shader.type == ShaderAsset::ShaderType::SHADER_TYPE_VERTEX))
        {
            shader.type = ShaderAsset::ShaderType::SHADER_TYPE_VERTEX;
        }
        ImGui::BeginDisabled(shader.platform == ShaderAsset::ShaderPlatform::PLATFORM_OPENGL);
        if (ImGui::RadioButton("Compute", shader.type == ShaderAsset::ShaderType::SHADER_TYPE_COMPUTE))
        {
            shader.type = ShaderAsset::ShaderType::SHADER_TYPE_COMPUTE;
        }
        ImGui::EndDisabled();
    }
    ImGui::EndChild();

    ImGui::End();

    BatchCompileWindow::Render();
    BatchDecompileWindow::Render();
}

int main(int argc, char **argv)
{
    if (!SDKWindow::Get().Init("GAME SDK Shader Editor"))
    {
        return -1;
    }

    const std::string &openPath = DesktopInterface::Get().GetFileArgument(argc, argv, {".gshd"});
    if (!openPath.empty())
    {
        openGshd(openPath);
    } else
    {
        const std::string &importPath = DesktopInterface::Get().GetFileArgument(argc,
                                                                                argv,
                                                                                {
                                                                                    ".glsl",
                                                                                    ".frag",
                                                                                    ".vert",
                                                                                    ".comp",
                                                                                });
        if (!importPath.empty())
        {
            importGlsl(importPath);
        }
    }

    SDKWindow::Get().MainLoop(Render);

    SDKWindow::Get().Destroy();

    return 0;
}
