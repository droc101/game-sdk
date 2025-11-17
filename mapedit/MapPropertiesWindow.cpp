//
// Created by droc101 on 11/17/25.
//

#include "MapPropertiesWindow.h"

#include "imgui.h"
#include "MapEditor.h"
#include "TextureBrowserWindow.h"
#include <misc/cpp/imgui_stdlib.h>

void MapPropertiesWindow::Render()
{
    if (!visible)
    {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(400, -1));
    ImGui::Begin("Map Properties", &visible, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

    ImGui::Text("Sky Texture");
    TextureBrowserWindow::InputTexture("##skyTexture", MapEditor::level.sky_texture);

    ImGui::Text("Discord RPC icon ID");
    ImGui::InputText("##rpcIcon", &MapEditor::level.discord_rpc_icon_id);

    ImGui::Text("Discord RPC display name");
    ImGui::InputText("##rpcName", &MapEditor::level.discord_rpc_map_name);

    ImGui::End();
}

