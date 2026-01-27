//
// Created by droc101 on 9/5/25.
//

#pragma once

#include <cstdint>
#include <glm/mat4x4.hpp>
#include "GLHelper.h"
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
        Viewport(ViewportType type);

        ~Viewport();

        void RenderImGui();

        void GetWindowRect(ImVec2 &pos, ImVec2 &size) const;

        [[nodiscard]] ViewportType GetType() const;

        [[nodiscard]] glm::mat4 GetMatrix() const;

        [[nodiscard]] glm::vec3 ScreenToWorldPos(ImVec2 localScreenPos) const;

        [[nodiscard]] glm::vec2 WorldToScreenPos(glm::vec3 worldPos) const;

        void CenterPosition(glm::vec3 pos);

        float &GetZoom();

        void ClampZoom();

        [[nodiscard]] glm::vec3 GetWorldSpaceMousePos() const;

        static ImVec2 GetLocalMousePos();

    private:
        ImVec2 windowPos;
        ImVec2 windowSize;
        ViewportType type;

        GLHelper::GL_Framebuffer framebuffer;

        ImVec2 scrollCenterPos{};
        float zoom = 10.0f;
};
