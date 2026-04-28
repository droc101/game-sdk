//
// Created by droc101 on 9/5/25.
//

#pragma once

#include <cstdint>
#include <game_sdk/gl/GLHelper.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
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

        /**
         * Render this viewport, including both the ImGUI and OpenGL portions
         */
        void Render();

        /**
         * Get the ImGui window position and size
         */
        void GetWindowRect(ImVec2 &pos, ImVec2 &size) const;

        /**
         * Get the type of this viewport
         */
        [[nodiscard]] ViewportType GetType() const;

        /**
         * Get the world -> screen matrix for this viewport
         */
        [[nodiscard]] glm::mat4 GetMatrix() const;

        /**
         * Convert a screen position to the corresponding world position
         * @param localScreenPos screen position, where {0,0} is the top-left of the viewport window
         */
        [[nodiscard]] glm::vec3 ScreenToWorldPos(ImVec2 localScreenPos) const;

        /**
         * Convert a world position to the corresponding screen position
         * @param worldPos World position to convert
         * @return screen position, where {0,0} is the top-left of the viewport window
         */
        [[nodiscard]] glm::vec2 WorldToScreenPos(glm::vec3 worldPos) const;

        /**
         * Center the camera on a 3D position
         */
        void CenterPosition(glm::vec3 pos);

        /**
         * Get a reference to the zoom level (units visible along the vertical axis of the viewport)
         * @warning Only write to this to directly set zoom to an exact value, use @c ChangeZoom for adjusting zoom
         */
        float &GetZoom();

        /**
         * Get the mouse position in 3D space
         * @note the local depth axis will be 0
         */
        [[nodiscard]] glm::vec3 GetWorldSpaceMousePos() const;

        /**
         * Get the local mouse position in the viewport
         */
        [[nodiscard]] ImVec2 GetLocalMousePos() const;

        /**
         * Adjust zoom
         */
        void ChangeZoom(float by);

        /**
         * Get the screen size in world space
         */
        [[nodiscard]] glm::vec2 GetWorldSpaceSize() const;

        /**
         * Get the camera's position in world space
         * @note the local depth axis will be 0
         */
        [[nodiscard]] glm::vec3 GetCameraPos() const;

    private:
        /// Screen space window position
        ImVec2 windowPos;
        /// Screen space window size
        ImVec2 windowSize;
        /// This viewport's type
        ViewportType type;

        /// The last local mouse position
        ImVec2 lastLocalMousePos;
        /// World to screen matrix
        glm::mat4 worldScreenMatrix = glm::identity<glm::mat4>();
        /// Inverse of world to screen matrix
        glm::mat4 inverseWorldScreenMatrix = glm::identity<glm::mat4>();

        /// The OpenGL framebuffer
        GLHelper::GL_Framebuffer framebuffer{};

        /// The camera's center position
        ImVec2 scrollCenterPos{};
        /// The number of units visible along the vertical axis of the viewport
        float zoom = 10.0f;

        void RecalculateMatrices();
};
