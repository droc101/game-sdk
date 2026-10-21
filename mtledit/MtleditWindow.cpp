//
// Created by droc101 on 8/21/26.
//

#include "MtleditWindow.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <game_sdk/DialogFilters.h>
#include <game_sdk/SharedMgr.h>
#include <game_sdk/Window.h>
#include <game_sdk/WindowManager.h>
#include <game_sdk/windows/TextureBrowserWindow.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <libassets/type/Material.h>
#include <libassets/util/Error.h>
#include <string>

const Window::WindowProperties &MtleditWindow::GetProperties() const
{
    return properties;
}

bool MtleditWindow::Init()
{
    const std::string &openPath = WindowManager::Get().GetArgumentParser().GetFileArgument({".gmtl"});
    if (!openPath.empty())
    {
        OpenGmtl(openPath);
    }
    return true;
}

void MtleditWindow::FileDropped(const std::string &filePath)
{
    if (filePath.ends_with(".gmtl"))
    {
        OpenGmtl(filePath);
    }
}

void MtleditWindow::OpenGmtl(const std::string &path)
{
    const Error::ErrorCode errorCode = material.LoadFromAsset(path);
    if (errorCode != Error::ErrorCode::OK)
    {
        ErrorMessage(std::format("Failed to open the material!\n{}", errorCode));
    }
}

void MtleditWindow::SaveGmtl(const std::string &path) const
{
    const Error::ErrorCode errorCode = material.SaveToAsset(path);
    if (errorCode != Error::ErrorCode::OK)
    {
        ErrorMessage(std::format("Failed to save the material!\n{}", errorCode));
    }
}

void MtleditWindow::Render()
{
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
                RequestClose();
            }
            ImGui::EndMenu();
        }
        SharedMgr::Get().SharedMenuUI("mtledit");
        ImGui::EndMainMenuBar();
    }

    if (openPressed)
    {
        OpenFileDialog(FILE_DIALOG_CALLBACK(OpenGmtl), DialogFilters::GMTL_FILTERS);
    } else if (savePressed)
    {
        SaveFileDialog(FILE_DIALOG_CALLBACK(SaveGmtl), DialogFilters::GMTL_FILTERS);
    } else if (newPressed)
    {
        material = LevelMaterialAsset();
    }

    ImGui::PushItemWidth(-1);
    ImTextureID tid{};
    const Error::ErrorCode e = SharedMgr::Get().textureCache.GetTextureID(material.texture, tid);
    ImVec2 sz = ImGui::GetContentRegionAvail();
    if (e == Error::ErrorCode::OK)
    {
        constexpr int IMAGE_PANEL_HEIGHT = 128;
        ImVec2 imageSize{};
        SharedMgr::Get().textureCache.GetTextureSize(material.texture, imageSize);
        const glm::vec2 scales = {(sz.x - 16) / imageSize.x, IMAGE_PANEL_HEIGHT / imageSize.y};
        const float scale = std::ranges::min(scales.x, scales.y);

        imageSize = {imageSize.x * scale, imageSize.y * scale};
        if (ImGui::BeginChild("##imageBox",
                              {sz.x, IMAGE_PANEL_HEIGHT + 16},
                              ImGuiChildFlags_Borders,
                              ImGuiWindowFlags_NoResize))
        {
            sz = ImGui::GetContentRegionAvail();
            ImVec2 pos = ImGui::GetCursorPos();
            pos.x += (sz.x - imageSize.x) * 0.5f;
            pos.y += (sz.y - imageSize.y) * 0.5f;

            ImGui::SetCursorPos(pos);
            ImGui::Image(tid, imageSize);
        }
        ImGui::EndChild();
    }
    ImGui::Separator();

    ImGui::Text("Texture");
    TextureBrowserWindow::InputTexture("##texture", material.texture);
    ImGui::Text("Base Scale");
    ImGui::PushItemWidth(-1);
    ImGui::InputFloat2("##baseScale", glm::value_ptr(material.baseScale));
    // TODO soundClass (when more exist)
    ImGui::TextUnformatted("Shader");
    constexpr std::array<const char *, 3> SHADERS = {"Sky", "Unshaded", "Shaded"};
    size_t sel = static_cast<uint32_t>(material.shader);
    if (ImGui::BeginCombo("##shader", SHADERS.at(sel)))
    {
        for (size_t i = 0; i < SHADERS.size(); i++)
        {
            if (i == static_cast<size_t>(Material::MaterialShader::SHADER_SKY))
            {
                continue;
            }
            const bool isSelected = sel == i;
            if (ImGui::Selectable(SHADERS.at(i), isSelected))
            {
                sel = i;
            }
            material.shader = static_cast<Material::MaterialShader>(sel);
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Separator();
    ImGui::CheckboxFlags("Invisible", &material.flags, LevelMaterialAsset::FLAG_INVISIBLE);
    ImGui::CheckboxFlags("Casts Shadows", &material.flags, LevelMaterialAsset::FLAG_CASTS_SHADOWS);
    ImGui::Separator();
    ImGui::SetNextItemWidth(-1);
    ImGui::Text("Emissive Strength");

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 0});

    ImGui::InputFloat("##emissive", &material.emissive);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0, 0});
    ImGui::PushFont(GetNormalFont(), 8);
    ImGui::SliderFloat("##emissiveSlider", &material.emissive, 0.0, 1.0, "", ImGuiSliderFlags_NoRoundToFormat);
    ImGui::PopFont();
    ImGui::PopStyleVar(2);
}
