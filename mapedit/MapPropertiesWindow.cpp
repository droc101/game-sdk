//
// Created by droc101 on 11/17/25.
//

#include "MapPropertiesWindow.h"
#include <cmath>
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

    ImGui::Checkbox("Render Sky", &MapEditor::map.hasSky);
    TextureBrowserWindow::Get().InputTexture("##skyTexture", MapEditor::map.skyTexture);

    ImGui::Text("Discord RPC icon ID");
    ImGui::InputText("##rpcIcon", &MapEditor::map.discordRpcIconId);

    ImGui::Text("Discord RPC display name");
    ImGui::InputText("##rpcName", &MapEditor::map.discordRpcMapName);

    ImGui::Text("Lightcube luxels per unit");
    ImGui::InputScalar("##lightcubeLuxels", ImGuiDataType_U8, &MapEditor::map.lightCubeLuxelsPerUnit);
    const float lightcubeSizeBytes = powf(static_cast<float>(MapEditor::map.lightCubeLuxelsPerUnit) * 16, 3) * 2 * 4;
    ImGui::Text("16 unit lightcube storage size: %.2f MB", lightcubeSizeBytes / (1024.0f * 1024.0f));

    ImGui::End();
}
