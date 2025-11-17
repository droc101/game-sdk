//
// Created by droc101 on 9/5/25.
//

#include "Viewport.h"
#include <cassert>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <string>
#include "GLHelper.h"
#include "imgui.h"
#include "MapEditor.h"

Viewport::Viewport(const ImVec2 gridPos, const ImVec2 gridSize, const ViewportType type)
{
    assert(gridPos.x >= 0 && gridPos.x < 2);
    assert(gridPos.y >= 0 && gridPos.y < 2);
    assert(gridSize.x > 0 && gridSize.x <= 2);
    assert(gridSize.y > 0 && gridSize.y <= 2);
    this->gridPos = gridPos;
    this->gridSize = gridSize;
    this->type = type;
}

void Viewport::GetWindowRect(ImVec2 &pos, ImVec2 &size) const
{
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    const float sidebarSize = MapEditor::showSidebar ? MapEditor::SIDEBAR_WIDTH : 0;
    const ImVec2 GridTopLeft = ImVec2(viewport->WorkPos.x + sidebarSize,
                                      viewport->WorkPos.y + MapEditor::TOOLBAR_HEIGHT);
    const ImVec2 GridCellSize = ImVec2((viewport->WorkSize.x - sidebarSize) / 2,
                                       (viewport->WorkSize.y - MapEditor::TOOLBAR_HEIGHT) / 2);
    if (fullscreen)
    {
        pos = ImVec2(GridTopLeft.x, GridTopLeft.y);
        size = ImVec2(GridCellSize.x * 2, GridCellSize.y * 2);
    } else
    {
        pos = ImVec2(GridTopLeft.x + (GridCellSize.x * gridPos.x), GridTopLeft.y + (GridCellSize.y * gridPos.y));
        size = ImVec2(GridCellSize.x * gridSize.x, GridCellSize.y * gridSize.y);
    }
}


void Viewport::RenderImGui()
{
    ImVec2 WindowSize;
    ImVec2 WindowPos;
    GetWindowRect(WindowPos, WindowSize);
    ImGui::SetNextWindowSize(WindowSize);
    ImGui::SetNextWindowPos(WindowPos);
    std::string title;
    if (type == ViewportType::TOP_DOWN_XZ)
    {
        title = "Top down (XZ)";
    } else if (type == ViewportType::FRONT_XY)
    {
        title = "Front (XY)";
    } else if (type == ViewportType::SIDE_YZ)
    {
        title = "Side (YZ)";
    }
    ImGui::PushStyleColor(ImGuiCol_Border, ImGui::GetColorU32(ImGuiCol_WindowBg));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 4);
    constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse |
                                             ImGuiWindowFlags_NoResize |
                                             ImGuiWindowFlags_NoMove |
                                             ImGuiWindowFlags_NoSavedSettings |
                                             ImGuiWindowFlags_NoBringToFrontOnFocus |
                                             ImGuiWindowFlags_NoDocking |
                                             ImGuiWindowFlags_NoDecoration;
    ImGui::Begin(("##_" + title).c_str(), nullptr, windowFlags);
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetColorU32(ImGuiCol_WindowBg));

    constexpr ImGuiChildFlags childFlags = ImGuiChildFlags_AlwaysAutoResize |
                                           ImGuiChildFlags_AutoResizeX |
                                           ImGuiChildFlags_AutoResizeY |
                                           ImGuiChildFlags_Border;
    if (ImGui::BeginChild("_vp_stats", ImVec2(0, 0), childFlags))
    {
        ImGui::TextUnformatted(title.c_str());
        if (MapEditor::drawViewportInfo)
        {
            ImGui::Text("Pos: %.2f, %.2f\nZoom: %.2f units/screen\nGrid: %.2f units",
                        scrollCenterPos.x,
                        scrollCenterPos.y,
                        zoom,
                        MapEditor::GRID_SPACING_VALUES.at(MapEditor::gridSpacingIndex));
        }
        ImGui::EndChild();
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    if (ImGui::IsWindowHovered() && !(ImGui::IsMouseDown(ImGuiMouseButton_Left) ||
                                      ImGui::IsMouseDown(ImGuiMouseButton_Right) ||
                                      ImGui::IsMouseDown(ImGuiMouseButton_Middle)))
    {
        ImGui::SetWindowFocus();
    }

    if (ImGui::IsWindowFocused())
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Right) || ImGui::IsMouseDown(ImGuiMouseButton_Middle))
        {
            const ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;

            const float unitsPerPixel = zoom / WindowSize.y;
            scrollCenterPos = ImVec2(scrollCenterPos.x + (mouseDelta.x * -unitsPerPixel),
                                     scrollCenterPos.y + (mouseDelta.y * unitsPerPixel));
            if (scrollCenterPos.x > 550)
            {
                scrollCenterPos.x = 550;
            }
            if (scrollCenterPos.y > 550)
            {
                scrollCenterPos.y = 550;
            }
            if (scrollCenterPos.x < -550)
            {
                scrollCenterPos.x = -550;
            }
            if (scrollCenterPos.y < -550)
            {
                scrollCenterPos.y = -550;
            }
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        }
        zoom += ImGui::GetIO().MouseWheel * -5.0f; // TODO zoom around cursor
        ClampZoom();
    }

    MapEditor::tool->RenderViewport(*this);

    ImGui::End();
}

Viewport::ViewportType Viewport::GetType() const
{
    return type;
}

glm::vec3 Viewport::ScreenToWorldPos(ImVec2 localScreenPos) const
{
    ImVec2 WindowSize;
    ImVec2 WindowPos;
    GetWindowRect(WindowPos, WindowSize);
    const glm::vec2 ndcPos2d = GLHelper::ScreenToNDC({localScreenPos.x, localScreenPos.y},
                                                     {WindowSize.x, WindowSize.y});
    const glm::mat4 matrix = GetMatrix();
    const glm::mat4 invMatrix = glm::inverse(matrix);
    const glm::vec4 clipPos = glm::vec4(ndcPos2d, 0.0f, 1.0f);
    const glm::vec4 worldPos = invMatrix * clipPos;
    glm::vec3 worldPos3 = glm::vec3(worldPos) / worldPos.w;
    switch (type)
    {
        case ViewportType::TOP_DOWN_XZ:
            worldPos3.y = 0;
            break;
        case ViewportType::FRONT_XY:
            worldPos3.z = 0;
            break;
        case ViewportType::SIDE_YZ:
        default:
            worldPos3.x = 0;
            break;
    }
    return worldPos3;
}

glm::vec2 Viewport::WorldToScreenPos(const glm::vec3 worldPos) const
{
    ImVec2 WindowSize;
    ImVec2 WindowPos;
    GetWindowRect(WindowPos, WindowSize);
    const glm::mat4 matrix = GetMatrix();
    const glm::vec4 fullNdc = matrix * glm::vec4(worldPos, 1.0f);
    const glm::vec2 ndc = glm::vec2(fullNdc.x, fullNdc.y);
    const float screenX = (ndc.x * 0.5f + 0.5f) * WindowSize.x;
    const float screenY = (-ndc.y * 0.5f + 0.5f) * WindowSize.y;
    return {screenX, screenY};
}


glm::mat4 Viewport::GetMatrix() const
{
    ImVec2 WindowSize;
    ImVec2 WindowPos;
    GetWindowRect(WindowPos, WindowSize);
    const float aspect = (WindowSize.y != 0.0f) ? (WindowSize.x / WindowSize.y) : 1.0f;
    const float halfHeight = zoom / 2.0f;
    const float halfWidth = aspect * halfHeight;

    const float left = scrollCenterPos.x - halfWidth;
    const float right = scrollCenterPos.x + halfWidth;
    const float top = scrollCenterPos.y + halfHeight;
    const float bottom = scrollCenterPos.y - halfHeight;
    const glm::mat4 ortho = glm::ortho(left, right, bottom, top, 0.01f, MapEditor::LEVEL_SIZE + 100);

    glm::vec3 up;
    glm::vec3 eye;
    glm::vec3 target;
    if (type == ViewportType::TOP_DOWN_XZ)
    {
        target = glm::vec3(0, MapEditor::LEVEL_HALF_SIZE + 8, 0);
        eye = glm::vec3(0, MapEditor::LEVEL_HALF_SIZE + 18, 0);
        up = glm::vec3(0, 0, 1);
    } else if (type == ViewportType::SIDE_YZ)
    {
        target = glm::vec3(MapEditor::LEVEL_HALF_SIZE + 8, 0, 0);
        eye = glm::vec3(MapEditor::LEVEL_HALF_SIZE + 18, 0, 0);
        up = glm::vec3(0, 1, 0);
    } else
    {
        target = glm::vec3(0, 0, MapEditor::LEVEL_HALF_SIZE + 8);
        eye = glm::vec3(0, 0, MapEditor::LEVEL_HALF_SIZE + 18);
        up = glm::vec3(0, 1, 0);
    }

    const glm::mat4 view = glm::lookAt(eye, target, up);

    return ortho * view;
}

void Viewport::CenterPosition(glm::vec3 pos)
{
    switch (type)
    {
        case ViewportType::TOP_DOWN_XZ:
            scrollCenterPos = {pos.x, pos.z};
            break;
        case ViewportType::FRONT_XY:
            scrollCenterPos = {pos.x, pos.y};
            break;
        case ViewportType::SIDE_YZ:
        default:
            scrollCenterPos = {pos.z, pos.y};
            break;
    }
}

float &Viewport::GetZoom()
{
    return zoom;
}

void Viewport::ClampZoom()
{
    if (zoom < 5)
    {
        zoom = 5;
    }
    if (zoom > MapEditor::LEVEL_SIZE + 500)
    {
        zoom = MapEditor::LEVEL_SIZE + 500;
    }
}

ImVec2 Viewport::GetLocalMousePos()
{
    const ImVec2 wPos = ImGui::GetWindowPos();
    const ImVec2 mPos = ImGui::GetMousePos();
    return {mPos.x - wPos.x, mPos.y - wPos.y};
}

bool Viewport::IsFullscreen() const
{
    return fullscreen;
}

void Viewport::ToggleFullscreen()
{
    fullscreen = !fullscreen;
}

glm::vec3 Viewport::GetWorldSpaceMousePos() const
{
    return ScreenToWorldPos(GetLocalMousePos());
}
