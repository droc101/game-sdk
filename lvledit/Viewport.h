//
// Created by droc101 on 9/5/25.
//

#pragma once

#include <cstdint>
#include <glm/mat4x4.hpp>
#include "imgui.h"

class Viewport
{
    public:
        enum class ViewportType : uint8_t
        {
            TOP_DOWN_XZ,
            FRONT_XY,
            SIDE_YZ
        };

        Viewport() = delete;
        Viewport(ImVec2 gridPos, ImVec2 gridSize, ViewportType type);

        ~Viewport() = default;

        void RenderImGui();

        void GetWindowRect(ImVec2 &pos, ImVec2 &size) const;
        [[nodiscard]] ViewportType GetType() const;

        [[nodiscard]] glm::mat4 GetMatrix() const;

        [[nodiscard]] glm::vec3 ScreenToWorldPos(ImVec2 localScreenPos) const;

        [[nodiscard]] glm::vec2 WorldToScreenPos(glm::vec3 worldPos) const;

        void CenterPosition(glm::vec3 pos);

        float &GetZoom();

        void ClampZoom();

        [[nodiscard]] bool IsFullscreen() const;

        void ToggleFullscreen();

        [[nodiscard]] glm::vec3 GetWorldSpaceMousePos() const;

        static ImVec2 GetLocalMousePos();

    private:
        ImVec2 gridPos;
        ImVec2 gridSize;
        bool fullscreen = false;
        ViewportType type;

        ImVec2 scrollCenterPos{};
        float zoom = 20;
};
