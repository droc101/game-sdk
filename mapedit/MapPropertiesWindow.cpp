//
// Created by droc101 on 11/17/25.
//

#include "MapPropertiesWindow.h"
#include <game_sdk/windows/TextureBrowserWindow.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include "MapEditor.h"

void MapPropertiesWindow::Render()
{
    if (!visible)
    {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(400, -1));
    ImGui::Begin("Map Properties",
                 &visible,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking);

    ImGui::Checkbox("Render Sky", &MapEditor::map.has_sky);
    TextureBrowserWindow::Get().InputTexture("##skyTexture", MapEditor::map.sky_texture);

    ImGui::Text("Discord RPC icon ID");
    ImGui::InputText("##rpcIcon", &MapEditor::map.discord_rpc_icon_id);

    ImGui::Text("Discord RPC display name");
    ImGui::InputText("##rpcName", &MapEditor::map.discord_rpc_map_name);

    ImGui::End();
}
