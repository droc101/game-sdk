//
// Created by droc101 on 7/23/25.
//

#include <array>
#include <cstdint>
#include <cstdio>
#include <format>
#include <game_sdk/DesktopInterface.h>
#include <game_sdk/DialogFilters.h>
#include <game_sdk/Options.h>
#include <game_sdk/SDKWindow.h>
#include <game_sdk/SharedMgr.h>
#include <imgui.h>
#include <libassets/asset/ShaderAsset.h>
#include <libassets/util/Error.h>
#include <Palette.h>
#include <string>
#include <TextEditor.h>
#include <vector>
#include "BatchCompileWindow.h"
#include "BatchDecompileWindow.h"

struct EditorTab
{
        ShaderAsset shader;
        TextEditor editor;
        std::string path;
        bool modified;
};

static std::vector<EditorTab> tabs{};
static size_t selectedTab = 0;

static void ThemeChanged()
{
    bool dark = true;
    if (Options::Get().theme == Options::Theme::SYSTEM)
    {
        if (SDL_GetSystemTheme() == SDL_SYSTEM_THEME_LIGHT)
        {
            dark = false;
        }
    } else if (Options::Get().theme == Options::Theme::LIGHT)
    {
        dark = false;
    }

    for (EditorTab &tab: tabs)
    {
        if (dark)
        {
            tab.editor.SetPalette(GetDarkPalette());
        } else
        {
            tab.editor.SetPalette(GetLightPalette());
        }
    }
}

static void AddTab(const ShaderAsset &shader, const std::string &path)
{
    // TODO prevent opening the same file twice
    tabs.push_back({
        .shader = shader,
        .editor = TextEditor(),
        .path = path,
        .modified = false,
    });
    EditorTab &tab = tabs.back();
    tab.editor.SetLanguageDefinition(LanguageDefinition::GLSL());
    tab.editor.SetShowWhitespaces(false);
    tab.editor.SetTabSize(4);
    tab.editor.SetReadOnly(false);
    tab.editor.SetText(tab.shader.GetGLSL());
    ThemeChanged();
}

static void openGshds(const std::vector<std::string> &paths)
{
    for (const std::string &path: paths)
    {
        ShaderAsset shader{};
        const Error::ErrorCode errorCode = ShaderAsset::CreateFromAsset(path.c_str(), shader);
        if (errorCode != Error::ErrorCode::OK)
        {
            SDKWindow::Get().ErrorMessage(std::format("Failed to open the shader!\n{}", errorCode));
            return;
        }
        AddTab(shader, path);
    }
}

static void importGlsls(const std::vector<std::string> &paths)
{
    for (const std::string &path: paths)
    {
        ShaderAsset shader{};
        const Error::ErrorCode errorCode = ShaderAsset::CreateFromGlsl(path.c_str(), shader);
        if (errorCode != Error::ErrorCode::OK)
        {
            SDKWindow::Get().ErrorMessage(std::format("Failed to import the shader!\n{}", errorCode));
            return;
        }
        AddTab(shader, "");
    }
}

static void saveGshd(const std::string &path)
{
    EditorTab &tab = tabs.at(selectedTab);
    const Error::ErrorCode errorCode = tab.shader.SaveAsAsset(path.c_str());
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to save the shader!\n{}", errorCode));
    } else
    {
        tab.modified = false;
    }
}

static void exportGlsl(const std::string &path)
{
    const EditorTab &tab = tabs.at(selectedTab);
    const Error::ErrorCode errorCode = tab.shader.SaveAsGlsl(path.c_str());
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to export the shader!\n{}", errorCode));
    }
}

static void RenderTab(EditorTab &tab)
{
    const ImVec2 &availableSize = ImGui::GetContentRegionAvail();

    constexpr float statsWidth = 150.0f;
    const float imageWidth = availableSize.x - statsWidth - 8.0f;

    ImGui::PushFont(SDKWindow::Get().GetMonospaceFont(), 18);
    tab.editor.Render("##glsl", {imageWidth, availableSize.y});
    if (tab.editor.IsTextChanged())
    {
        tab.modified = true;
    }
    ImGui::PopFont();
    ImGui::SameLine();

    ImGui::BeginChild("StatsPane", ImVec2(statsWidth, availableSize.y), ImGuiChildFlags_Borders);
    {
        ImGui::TextUnformatted("Platform");
        if (ImGui::RadioButton("Vulkan", tab.shader.platform == ShaderAsset::ShaderPlatform::PLATFORM_VULKAN))
        {
            tab.shader.platform = ShaderAsset::ShaderPlatform::PLATFORM_VULKAN;
        }
        if (ImGui::RadioButton("OpenGL", tab.shader.platform == ShaderAsset::ShaderPlatform::PLATFORM_OPENGL))
        {
            tab.shader.platform = ShaderAsset::ShaderPlatform::PLATFORM_OPENGL;
            if (tab.shader.type == ShaderAsset::ShaderType::SHADER_TYPE_COMPUTE)
            {
                tab.shader.type = ShaderAsset::ShaderType::SHADER_TYPE_FRAGMENT;
            }
        }

        ImGui::TextUnformatted("Type");
        if (ImGui::RadioButton("Fragment", tab.shader.type == ShaderAsset::ShaderType::SHADER_TYPE_FRAGMENT))
        {
            tab.shader.type = ShaderAsset::ShaderType::SHADER_TYPE_FRAGMENT;
        }
        if (ImGui::RadioButton("Vertex", tab.shader.type == ShaderAsset::ShaderType::SHADER_TYPE_VERTEX))
        {
            tab.shader.type = ShaderAsset::ShaderType::SHADER_TYPE_VERTEX;
        }
        ImGui::BeginDisabled(tab.shader.platform == ShaderAsset::ShaderPlatform::PLATFORM_OPENGL);
        if (ImGui::RadioButton("Compute", tab.shader.type == ShaderAsset::ShaderType::SHADER_TYPE_COMPUTE))
        {
            tab.shader.type = ShaderAsset::ShaderType::SHADER_TYPE_COMPUTE;
        }
        ImGui::EndDisabled();
    }
    ImGui::EndChild();
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
    bool savePressed = !tabs.empty() && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S);
    bool saveAllPressed = !tabs.empty() && ImGui::Shortcut(ImGuiMod_Alt | ImGuiMod_Shift | ImGuiKey_S);
    bool exportPressed = !tabs.empty() && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S);

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            newPressed |= ImGui::MenuItem("New", "Ctrl+N");
            ImGui::Separator();
            openPressed |= ImGui::MenuItem("Open", "Ctrl+O");
            importPressed |= ImGui::MenuItem("Import", "Ctrl+Shift+O");
            savePressed |= ImGui::MenuItem("Save", "Ctrl+S", false, !tabs.empty());
            savePressed |= ImGui::MenuItem("Save All", "Alt+Shift+S", false, !tabs.empty());
            exportPressed |= ImGui::MenuItem("Export", "Ctrl+Shift+S", false, !tabs.empty());
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4"))
            {
                SDKWindow::Get().PostQuit();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            bool canUndo = !tabs.empty();
            bool canRedo = !tabs.empty();
            bool hasSelection = false;
            TextEditor *editor = nullptr;
            if (!tabs.empty())
            {
                editor = &tabs.at(selectedTab).editor;
                canUndo = editor->CanUndo();
                canRedo = editor->CanRedo();
                hasSelection = editor->HasSelection();
            }
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, canUndo))
            {
                editor->Undo();
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z", false, canRedo))
            {
                editor->Redo();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "Ctrl+X", false, !tabs.empty() && hasSelection))
            {
                editor->Cut();
            }
            if (ImGui::MenuItem("Copy", "Ctrl+C", false, !tabs.empty() && hasSelection))
            {
                editor->Copy();
            }
            if (ImGui::MenuItem("Paste", "Ctrl+V", false, !tabs.empty()))
            {
                editor->Paste();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Select All", "Ctrl+A", false, !tabs.empty()))
            {
                editor->SelectAll();
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
        SDKWindow::Get().OpenMultiFileDialog(openGshds, DialogFilters::gshdFilters);
    } else if (importPressed)
    {
        SDKWindow::Get().OpenMultiFileDialog(importGlsls, DialogFilters::glslFilters);
    } else if (savePressed)
    {
        EditorTab &tab = tabs.at(selectedTab);
        if (tab.path.empty())
        {
            SDKWindow::Get().SaveFileDialog(saveGshd, DialogFilters::gshdFilters);
        } else
        {
            const Error::ErrorCode errorCode = tab.shader.SaveAsAsset(tab.path.c_str());
            if (errorCode != Error::ErrorCode::OK)
            {
                SDKWindow::Get().ErrorMessage(std::format("Failed to save the shader!\n{}", errorCode));
            } else
            {
                tab.modified = false;
            }
        }
    } else if (saveAllPressed)
    {
        for (EditorTab &tab: tabs)
        {
            if (!tab.path.empty())
            {
                const Error::ErrorCode errorCode = tab.shader.SaveAsAsset(tab.path.c_str());
                if (errorCode != Error::ErrorCode::OK)
                {
                    SDKWindow::Get().ErrorMessage(std::format("Failed to save the shader \"{}\"!\n{}",
                                                              tab.path,
                                                              errorCode));
                } else
                {
                    tab.modified = false;
                }
            }
        }
    } else if (exportPressed)
    {
        SDKWindow::Get().SaveFileDialog(exportGlsl, DialogFilters::glslFilters);
    } else if (newPressed)
    {
        AddTab(ShaderAsset(), "");
    }

    if (ImGui::BeginTabBar("editorTabs",
                           ImGuiTabBarFlags_AutoSelectNewTabs |
                                   ImGuiTabBarFlags_Reorderable |
                                   ImGuiTabBarFlags_FittingPolicyMixed))
    {
        if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip))
        {
            AddTab(ShaderAsset(), "");
        }

        size_t indexToClose = SIZE_MAX;
        for (size_t i = 0; i < tabs.size(); i++)
        {
            EditorTab &tab = tabs.at(i);
            std::string title = "Untitled";

            if (!tab.path.empty())
            {
#ifdef WIN32
                constexpr char const *pathSep = "\\";
#else
                constexpr char const *pathSep = "/";
#endif
                title = tab.path.substr(tab.path.find_last_of(pathSep) + 1);
            }
            title += std::format("##{}", i);

            bool open = true;
            ImGuiTabItemFlags flags = ImGuiTabItemFlags_None;
            if (tab.modified)
            {
                flags |= ImGuiTabItemFlags_UnsavedDocument;
            }
            if (ImGui::BeginTabItem(title.c_str(), &open, flags))
            {
                selectedTab = i;
                RenderTab(tab);
                ImGui::EndTabItem();
            }
            if (!open)
            {
                indexToClose = i;
            }
        }

        if (indexToClose != SIZE_MAX)
        {
            tabs.erase(tabs.begin() + indexToClose);
        }

        ImGui::EndTabBar();
    }

    ImGui::End();

    BatchCompileWindow::Render();
    BatchDecompileWindow::Render();
}

int main(int argc, char **argv)
{
    if (!SDKWindow::Get().Init("GAME SDK Shader Editor", {1366, 768}))
    {
        return -1;
    }

    SDKWindow::Get().SetThemeChangeCallback(ThemeChanged);

    ThemeChanged();

    const std::string &openPath = DesktopInterface::Get().GetFileArgument(argc, argv, {".gshd"});
    if (!openPath.empty())
    {
        openGshds({openPath});
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
            importGlsls({importPath});
        }
    }

    SDKWindow::Get().MainLoop(Render);

    SDKWindow::Get().Destroy();

    return 0;
}
