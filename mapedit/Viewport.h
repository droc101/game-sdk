//
// Created by droc101 on 9/5/25.
//

#pragma once

#include <cstdint>
#include <game_sdk/gl/GLHelper.h>
#include <glm/mat4x4.hpp>
#include <imgui.h>

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
        explicit Viewport(ViewportType type);

        ~Viewport();

        void RenderImGui();

        void GetWindowRect(ImVec2 &pos, ImVec2 &size) const;

        [[nodiscard]] ViewportType GetType() const;

        [[nodiscard]] glm::mat4 GetMatrix() const;

        [[nodiscard]] glm::vec3 ScreenToWorldPos(ImVec2 localScreenPos) const;

        [[nodiscard]] glm::vec2 WorldToScreenPos(glm::vec3 worldPos) const;

        void CenterPosition(glm::vec3 pos);

        float &GetZoom();

        [[nodiscard]] glm::vec3 GetWorldSpaceMousePos() const;

        [[nodiscard]] ImVec2 GetLocalMousePos() const;

        void ChangeZoom(float by);

        glm::vec2 GetWorldSpaceSize() const;

        glm::vec3 GetCameraPos() const;

    private:
        ImVec2 windowPos;
        ImVec2 windowSize;
        ViewportType type;

        ImVec2 lastLocalMousePos;

        GLHelper::GL_Framebuffer framebuffer{};

        ImVec2 scrollCenterPos{};
        float zoom = 10.0f;
};
