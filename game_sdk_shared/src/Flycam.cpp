//
// Created by droc101 on 9/24/26.
//

#include <game_sdk/Flycam.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>

Flycam::Flycam()
{
    ResetView();
}

void Flycam::SetViewport(const glm::vec2 size, const float nearPlane, const float farPlane)
{
    perspectiveMatrix = glm::perspective(glm::radians(90.0f), size.x / size.y, nearPlane, farPlane);
}

glm::mat4 Flycam::GetViewMatrix()
{
    const glm::quat rotationQuat = glm::quat(cameraRotation);

    const glm::mat4 view = glm::mat4_cast(glm::conjugate(rotationQuat)) * glm::translate(glm::mat4(1.0f), -cameraPos);

    return view;
}

glm::mat4 Flycam::GetPerspectiveMatrix()
{
    return perspectiveMatrix;
}

void Flycam::ProcessInput()
{
    ImGuiIO &io = ImGui::GetIO();
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
    {
        io.WantCaptureMouse = false;
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        const ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
        cameraRotation.x += glm::radians(dragDelta.y / -5.0f);
        cameraRotation.y += glm::radians(dragDelta.x / -5.0f);
        ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
        cameraRotation.x = glm::clamp(cameraRotation.x,
                                      static_cast<float>(-(M_PI_2 - FLT_EPSILON)),
                                      static_cast<float>(M_PI_2 - FLT_EPSILON));
    }
    glm::vec3 moveDir{};
    if (ImGui::IsKeyDown(ImGuiKey_W))
    {
        moveDir.z = -1;
    } else if (ImGui::IsKeyDown(ImGuiKey_S))
    {
        moveDir.z = 1;
    }
    if (ImGui::IsKeyDown(ImGuiKey_A))
    {
        moveDir.x = -1;
    } else if (ImGui::IsKeyDown(ImGuiKey_D))
    {
        moveDir.x = 1;
    }

    cameraSpeed += ImGui::GetIO().MouseWheel;
    cameraSpeed = glm::clamp(cameraSpeed, 1.0f, 128.0f);

    moveDir *= cameraSpeed;
    cameraPos += glm::quat(cameraRotation) * moveDir;
}

void Flycam::ResetView()
{
    cameraPos = {};
    cameraRotation = {0, glm::pi<float>(), 0};
}
