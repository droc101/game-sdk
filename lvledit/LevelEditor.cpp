//
// Created by droc101 on 9/6/25.
//

#include "LevelEditor.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <imgui.h>
#include <libassets/type/WallMaterial.h>
#include <libassets/util/Error.h>
#include <limits>
#include <vector>
#include "SharedMgr.h"
#include "TextureBrowserWindow.h"

float LevelEditor::SnapToGrid(const float f)
{
    if (!snapToGrid)
    {
        return f;
    }
    const float gridSize = GRID_SPACING_VALUES.at(gridSpacingIndex);
    const float nf = f / gridSize;
    const float fs = std::round(nf);
    return fs * gridSize;
}

glm::vec3 LevelEditor::SnapToGrid(const glm::vec3 v)
{
    return {SnapToGrid(v.x), SnapToGrid(v.y), SnapToGrid(v.z)};
}

glm::vec2 LevelEditor::SnapToGrid(const glm::vec2 v)
{
    return {SnapToGrid(v.x), SnapToGrid(v.y)};
}


bool LevelEditor::IsPointInBounds(const glm::vec3 p)
{
    const bool x = p.x >= -512 && p.x <= 512;
    const bool y = p.y >= -512 && p.y <= 512;
    const bool z = p.z >= -512 && p.z <= 512;
    return x && y && z;
}

float LevelEditor::VecDistanceToLine2D(const glm::vec2 lineStart, const glm::vec2 lineEnd, const glm::vec2 testPoint)
{
    const float lineMag = distance(lineStart, lineEnd);

    if (lineMag == 0.0)
    {
        return distance(lineStart, testPoint);
    }

    const float u = (((testPoint.x - lineStart.x) * (lineEnd.x - lineStart.x)) +
                     ((testPoint.y - lineStart.y) * (lineEnd.y - lineStart.y))) /
                    (lineMag * lineMag);

    if (u < 0.0)
    {
        return distance(testPoint, lineStart);
    }
    if (u > 1.0)
    {
        return distance(testPoint, lineEnd);
    }

    glm::vec2 intersection;
    intersection.x = lineStart.x + u * (lineEnd.x - lineStart.x);
    intersection.y = lineStart.y + u * (lineEnd.y - lineStart.y);

    return distance(testPoint, intersection);
}

std::array<float, 4> LevelEditor::CalculateBBox(const std::vector<std::array<float, 2>> &points)
{
    std::vector<glm::vec2> glmPoints{};
    for (const std::array<float, 2> &point: points)
    {
        glmPoints.emplace_back(point.at(0), point.at(1));
    }
    return CalculateBBox(glmPoints);
}


std::array<float, 4> LevelEditor::CalculateBBox(const std::vector<glm::vec2> &points)
{
    glm::vec2 minPoint = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    glm::vec2 maxPoint = {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};

    for (const glm::vec2 &vert: points)
    {
        if (vert.x < minPoint.x)
        {
            minPoint.x = vert.x;
        }
        if (vert.x > maxPoint.x)
        {
            maxPoint.x = vert.x;
        }

        if (vert.y < minPoint.y)
        {
            minPoint.y = vert.y;
        }
        if (vert.y > maxPoint.y)
        {
            maxPoint.y = vert.y;
        }
    }
    return {minPoint.x, minPoint.y, maxPoint.x, maxPoint.y};
}

void LevelEditor::MaterialToolWindow(WallMaterial &wallMat)
{
    ImGui::PushItemWidth(-1);
    ImTextureID tid{};
    const Error::ErrorCode e = SharedMgr::textureCache->GetTextureID(wallMat.texture, tid);
    ImVec2 sz = ImGui::GetContentRegionAvail();
    if (e == Error::ErrorCode::OK)
    {
        constexpr int imagePanelHeight = 128;
        ImVec2 imageSize{};
        SharedMgr::textureCache->GetTextureSize(wallMat.texture, imageSize);
        const glm::vec2 scales = {(sz.x - 16) / imageSize.x, imagePanelHeight / imageSize.y};
        const float scale = std::ranges::min(scales.x, scales.y);

        imageSize = {imageSize.x * scale, imageSize.y * scale};
        if (ImGui::BeginChild("##imageBox",
                              {sz.x, imagePanelHeight + 16},
                              ImGuiChildFlags_Border,
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
    TextureBrowserWindow::InputTexture("##Texture", wallMat.texture);
    ImGui::Separator();
    ImGui::Text("UV Offset");
    ImGui::InputFloat2("##uvOffset", wallMat.uvOffset.data());
    ImGui::Text("UV Scale");
    ImGui::InputFloat2("##uvScale", wallMat.uvScale.data());
}
