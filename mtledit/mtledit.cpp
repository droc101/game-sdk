//
// Created by droc101 on 11/16/25.
//
#include <array>
#include <cstdint>
#include <cstdio>
#include <format>
#include <game_sdk/DesktopInterface.h>
#include <game_sdk/DialogFilters.h>
#include <game_sdk/SDKWindow.h>
#include <game_sdk/SharedMgr.h>
#include <game_sdk/windows/TextureBrowserWindow.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <libassets/asset/LevelMaterialAsset.h>
#include <libassets/type/Material.h>
#include <libassets/util/Error.h>
#include <string>

static LevelMaterialAsset material{};

static void openGmtl(const std::string &path)
{
    const Error::ErrorCode errorCode = LevelMaterialAsset::CreateFromAsset(path.c_str(), material);
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::ErrorMessage(std::format("Failed to open the material!\n{}", errorCode));
    }
}

static void saveGmtl(const std::string &path)
{
    const Error::ErrorCode errorCode = material.SaveAsAsset(path.c_str());
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::ErrorMessage(std::format("Failed to save the material!\n{}", errorCode));
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
    ImGui::Begin("mtledit", nullptr, windowFlags);
    bool newPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N);
    bool openPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O);
    bool savePressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S);

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            newPressed |= ImGui::MenuItem("New", "Ctrl+N");
            openPressed |= ImGui::MenuItem("Open", "Ctrl+O");
            savePressed |= ImGui::MenuItem("Save", "Ctrl+S");
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4"))
            {
                SDKWindow::PostQuit();
            }
            ImGui::EndMenu();
        }
        SharedMgr::SharedMenuUI("mtledit");
        ImGui::EndMainMenuBar();
    }

    if (openPressed)
    {
        SDKWindow::OpenFileDialog(openGmtl, DialogFilters::gmtlFilters);
    } else if (savePressed)
    {
        SDKWindow::SaveFileDialog(saveGmtl, DialogFilters::gmtlFilters);
    } else if (newPressed)
    {
        material = LevelMaterialAsset();
    }

    ImGui::Text("Texture");
    TextureBrowserWindow::InputTexture("##texture", material.texture);
    ImGui::Text("Base Scale");
    ImGui::PushItemWidth(-1);
    ImGui::InputFloat2("##baseScale", glm::value_ptr(material.baseScale));
    // TODO soundClass (when more exist)
    bool unshaded = material.shader == Material::MaterialShader::SHADER_UNSHADED;
    if (ImGui::Checkbox("Unshaded", &unshaded))
    {
        material.shader = unshaded ? Material::MaterialShader::SHADER_UNSHADED
                                   : Material::MaterialShader::SHADER_SHADED;
    }
    ImGui::SameLine();
    ImGui::Checkbox("Invisible", &material.compileInvisible);
    ImGui::SameLine();
    ImGui::Checkbox("No Collision", &material.compileNoClip);


    ImGui::End();
}

int main(int argc, char **argv)
{
    if (!SDKWindow::Init("GAME SDK Material Editor"))
    {
        return -1;
    }

    const std::string &openPath = DesktopInterface::GetFileArgument(argc, argv, {".gmtl"});
    if (!openPath.empty())
    {
        openGmtl(openPath);
    }

    SDKWindow::MainLoop(Render);

    SDKWindow::Destroy();

    return 0;
}
