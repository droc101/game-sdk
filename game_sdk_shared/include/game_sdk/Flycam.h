//
// Created by droc101 on 9/24/26.
//

#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

class Flycam
{
    public:
        Flycam();

        glm::mat4 GetViewMatrix();
        glm::mat4 GetPerspectiveMatrix();

        void SetViewport(glm::vec2 size, float nearPlane, float farPlane);

        void ProcessInput();

        void ResetView();

        glm::vec3 cameraPos{};
        glm::vec3 cameraRotation{};
        float cameraSpeed = 4.0f;

    private:
        glm::mat4 perspectiveMatrix{};
};
