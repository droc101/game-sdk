//
// Created by droc101 on 9/5/25.
//

#include "Viewport.h"
#include <game_sdk/gl/GLHelper.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <imgui.h>
#include <string>
#include "MapEditor.h"

Viewport::Viewport(const ViewportType type)
{
    this->type = type;
}

Viewport::~Viewport()
{
    GLHelper::DestroyFramebuffer(framebuffer);
}

void Viewport::GetWindowRect(ImVec2 &pos, ImVec2 &size) const
{
    pos = windowPos;
    size = windowSize;
}

void Viewport::RenderImGui()
{
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
                                             ImGuiWindowFlags_NoMove |
                                             ImGuiWindowFlags_NoBringToFrontOnFocus |
                                             ImGuiWindowFlags_NoDecoration |
                                             ImGuiWindowFlags_NoScrollbar |
                                             ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::Begin(("" + title).c_str(), nullptr, windowFlags);
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();

    windowPos = ImGui::GetWindowPos();
    windowSize = ImGui::GetContentRegionMax();
    windowSize.x += 8;
    windowSize.y += 8;

    if (!framebuffer.created)
    {
        this->framebuffer = GLHelper::CreateFramebuffer({windowSize.x, windowSize.y});
    } else if (glm::vec2(windowSize.x, windowSize.y) != framebuffer.size)
    {
        GLHelper::ResizeFramebuffer(framebuffer, {windowSize.x, windowSize.y});
    }

    GLHelper::BindFramebuffer(framebuffer);

    MapEditor::tool->RenderViewport(*this);

    GLHelper::UnbindFramebuffer();

    const ImVec2 origCursor = ImGui::GetCursorPos();
    ImGui::SetCursorPos({0, 0});
    ImGui::Image(framebuffer.colorTexture, {framebuffer.size.x, framebuffer.size.y}, {0, 1}, {1, 0});
    ImGui::SetCursorPos(origCursor);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetColorU32(ImGuiCol_WindowBg));

    constexpr ImGuiChildFlags childFlags = ImGuiChildFlags_AlwaysAutoResize |
                                           ImGuiChildFlags_AutoResizeX |
                                           ImGuiChildFlags_AutoResizeY |
                                           ImGuiChildFlags_Borders;
    if (MapEditor::drawViewportInfo && ImGui::BeginChild("_vp_stats", ImVec2(0, 0), childFlags))
    {
        ImGui::Text("Pos: %.2f, %.2f\nZoom: %.2f units/screen\nGrid: %.2f units",
                    scrollCenterPos.x,
                    scrollCenterPos.y,
                    zoom,
                    MapEditor::GRID_SPACING_VALUES.at(MapEditor::gridSpacingIndex));
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

            const float unitsPerPixel = zoom / windowSize.y;
            scrollCenterPos = ImVec2(scrollCenterPos.x + (mouseDelta.x * -unitsPerPixel),
                                     scrollCenterPos.y + (mouseDelta.y * unitsPerPixel));
            if (scrollCenterPos.x > MapEditor::MAP_HALF_SIZE + 50)
            {
                scrollCenterPos.x = MapEditor::MAP_HALF_SIZE + 50;
            }
            if (scrollCenterPos.y > MapEditor::MAP_HALF_SIZE + 50)
            {
                scrollCenterPos.y = MapEditor::MAP_HALF_SIZE + 50;
            }
            if (scrollCenterPos.x < -(MapEditor::MAP_HALF_SIZE + 50))
            {
                scrollCenterPos.x = -(MapEditor::MAP_HALF_SIZE + 50);
            }
            if (scrollCenterPos.y < -(MapEditor::MAP_HALF_SIZE + 50))
            {
                scrollCenterPos.y = -(MapEditor::MAP_HALF_SIZE + 50);
            }
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        }
        zoom += ImGui::GetIO().MouseWheel * -5.0f; // TODO zoom around cursor
        ClampZoom();
    }

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
    const float aspect = (framebuffer.size.y != 0.0f) ? (framebuffer.size.x / framebuffer.size.y) : 1.0f;
    const float halfHeight = zoom / 2.0f;
    const float halfWidth = aspect * halfHeight;

    const float left = scrollCenterPos.x - halfWidth;
    const float right = scrollCenterPos.x + halfWidth;
    const float top = scrollCenterPos.y + halfHeight;
    const float bottom = scrollCenterPos.y - halfHeight;
    const glm::mat4 ortho = glm::ortho(left, right, bottom, top, 0.01f, MapEditor::MAP_SIZE + 100);

    glm::vec3 up;
    glm::vec3 eye;
    glm::vec3 target;
    if (type == ViewportType::TOP_DOWN_XZ)
    {
        target = glm::vec3(0, MapEditor::MAP_HALF_SIZE + 8, 0);
        eye = glm::vec3(0, MapEditor::MAP_HALF_SIZE + 18, 0);
        up = glm::vec3(0, 0, 1);
    } else if (type == ViewportType::SIDE_YZ)
    {
        target = glm::vec3(MapEditor::MAP_HALF_SIZE + 8, 0, 0);
        eye = glm::vec3(MapEditor::MAP_HALF_SIZE + 18, 0, 0);
        up = glm::vec3(0, 1, 0);
    } else
    {
        target = glm::vec3(0, 0, MapEditor::MAP_HALF_SIZE + 8);
        eye = glm::vec3(0, 0, MapEditor::MAP_HALF_SIZE + 18);
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
            scrollCenterPos = {-pos.x, pos.z};
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
    if (zoom > MapEditor::MAP_SIZE + 500)
    {
        zoom = MapEditor::MAP_SIZE + 500;
    }
}

ImVec2 Viewport::GetLocalMousePos()
{
    const ImVec2 wPos = ImGui::GetWindowPos();
    const ImVec2 mPos = ImGui::GetMousePos();
    return {mPos.x - wPos.x, mPos.y - wPos.y};
}

glm::vec3 Viewport::GetWorldSpaceMousePos() const
{
    return ScreenToWorldPos(GetLocalMousePos());
}
