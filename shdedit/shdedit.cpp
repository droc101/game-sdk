//
// Created by droc101 on 7/23/25.
//

#include <array>
#include <cstdio>
#include <format>
#include <imgui.h>
#include <libassets/asset/ShaderAsset.h>
#include <libassets/util/Error.h>
#include <misc/cpp/imgui_stdlib.h>
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_video.h>
#include <string>
#include "BatchCompileWindow.h"
#include "BatchDecompileWindow.h"
#include "DesktopInterface.h"
#include "DialogFilters.h"
#include "SDKWindow.h"
#include "SharedMgr.h"

static SDKWindow sdkWindow{};

static ShaderAsset shader{};
static bool shaderLoaded = false;

static void openGshd(const std::string &path)
{
    const Error::ErrorCode errorCode = ShaderAsset::CreateFromAsset(path.c_str(), shader);
    if (errorCode != Error::ErrorCode::OK)
    {
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                      "Error",
                                      std::format("Failed to open the shader!\n{}", errorCode).c_str(),
                                      sdkWindow.GetWindow()))
        {
            printf("Error: SDL_ShowSimpleMessageBox(): %s\n", SDL_GetError());
        }
        return;
    }
    shaderLoaded = true;
}

static void openGfonCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    openGshd(fileList[0]);
}

static void importGlsl(const std::string &path)
{
    const Error::ErrorCode errorCode = ShaderAsset::CreateFromGlsl(path.c_str(), shader);
    if (errorCode != Error::ErrorCode::OK)
    {
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                      "Error",
                                      std::format("Failed to import the shader!\n{}", errorCode).c_str(),
                                      sdkWindow.GetWindow()))
        {
            printf("Error: SDL_ShowSimpleMessageBox(): %s\n", SDL_GetError());
        }
        return;
    }
    shaderLoaded = true;
}

static void importCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    importGlsl(fileList[0]);
}

static void saveGfonCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode errorCode = shader.SaveAsAsset(fileList[0]);
    if (errorCode != Error::ErrorCode::OK)
    {
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                      "Error",
                                      std::format("Failed to save the shader!\n{}", errorCode).c_str(),
                                      sdkWindow.GetWindow()))
        {
            printf("Error: SDL_ShowSimpleMessageBox(): %s\n", SDL_GetError());
        }
    }
}

static void exportCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode errorCode = shader.SaveAsGlsl(fileList[0]);
    if (errorCode != Error::ErrorCode::OK)
    {
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                      "Error",
                                      std::format("Failed to export the shader!\n{}", errorCode).c_str(),
                                      sdkWindow.GetWindow()))
        {
            printf("Error: SDL_ShowSimpleMessageBox(): %s\n", SDL_GetError());
        }
    }
}

static void Render(SDL_Window *sdlWindow)
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
    bool savePressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S) && shaderLoaded;
    bool exportPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S) && shaderLoaded;

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            newPressed |= ImGui::MenuItem("New", "Ctrl+N");
            ImGui::Separator();
            openPressed |= ImGui::MenuItem("Open", "Ctrl+O");
            importPressed |= ImGui::MenuItem("Import", "Ctrl+Shift+O");
            savePressed |= ImGui::MenuItem("Save", "Ctrl+S", false, shaderLoaded);
            exportPressed |= ImGui::MenuItem("Export", "Ctrl+Shift+S", false, shaderLoaded);
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4"))
            {
                sdkWindow.PostQuit();
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
        SharedMgr::SharedMenuUI("shdedit");
        ImGui::EndMainMenuBar();
    }

    if (openPressed)
    {
        SDL_ShowOpenFileDialog(openGfonCallback,
                               nullptr,
                               sdlWindow,
                               DialogFilters::gshdFilters.data(),
                               1,
                               nullptr,
                               false);
    } else if (importPressed)
    {
        SDL_ShowOpenFileDialog(importCallback,
                               nullptr,
                               sdlWindow,
                               DialogFilters::glslFilters.data(),
                               4,
                               nullptr,
                               false);
    } else if (savePressed)
    {
        SDL_ShowSaveFileDialog(saveGfonCallback, nullptr, sdlWindow, DialogFilters::gshdFilters.data(), 1, nullptr);
    } else if (exportPressed)
    {
        SDL_ShowSaveFileDialog(exportCallback, nullptr, sdlWindow, DialogFilters::glslFilters.data(), 4, nullptr);
    } else if (newPressed)
    {
        shader = ShaderAsset();
        shaderLoaded = true;
    }

    if (shaderLoaded)
    {
        const ImVec2 &availableSize = ImGui::GetContentRegionAvail();

        constexpr float statsWidth = 150.0f;
        const float imageWidth = availableSize.x - statsWidth - 8.0f;

        ImGui::BeginChild("ImagePane",
                          ImVec2(imageWidth, availableSize.y),
                          ImGuiChildFlags_Borders,
                          ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus);
        {
            ImGui::InputTextMultiline("##glsl", &shader.GetGLSL(), ImVec2(-1, -1), ImGuiInputTextFlags_AllowTabInput);
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
    } else
    {
        ImGui::TextDisabled("No shader is open. Open, create, or import one from the File menu.");
    }

    ImGui::End();

    BatchCompileWindow::Render(sdlWindow);
    BatchDecompileWindow::Render(sdlWindow);
}

int main(int argc, char **argv)
{
    if (!sdkWindow.Init("shdedit"))
    {
        return -1;
    }

    const std::string &openPath = DesktopInterface::GetFileArgument(argc, argv, {".gshd"});
    if (!openPath.empty())
    {
        openGshd(openPath);
    } else
    {
        const std::string &importPath = DesktopInterface::GetFileArgument(argc,
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

    sdkWindow.MainLoop(Render);

    sdkWindow.Destroy();

    return 0;
}
