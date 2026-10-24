//
// Created by droc101 on 9/5/25.
//

#include "Viewport.h"
#include <game_sdk/gl/GLHelper.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <libassets/asset/MapAsset.h>
#include <string>
#include "MapEditor.h"

Viewport::Viewport(const ViewportType type)
{
    this->type = type;
}

Viewport::~Viewport()
{
    GLHelper::DestroyFramebuffer(framebuffer);
    if (type == ViewportType::PERSPECTIVE)
    {
        DestroyWorldBuffers();
    }
}

void Viewport::GetWindowRect(ImVec2 &pos, ImVec2 &size) const
{
    pos = windowPos;
    size = windowSize;
}

void Viewport::Render()
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
    } else if (type == ViewportType::PERSPECTIVE)
    {
        title = "3D";
    }
    ImGui::PushStyleColor(ImGuiCol_Border, ImGui::GetColorU32(ImGuiCol_WindowBg));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 4);
    constexpr ImGuiWindowFlags WINDOW_FLAGS = ImGuiWindowFlags_NoCollapse |
                                              ImGuiWindowFlags_NoMove |
                                              ImGuiWindowFlags_NoBringToFrontOnFocus |
                                              ImGuiWindowFlags_NoDecoration |
                                              ImGuiWindowFlags_NoScrollbar |
                                              ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::Begin(("" + title).c_str(), nullptr, WINDOW_FLAGS);
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();

    if (ImGui::IsWindowDocked())
    {
        const ImGuiWindow *w = ImGui::FindWindowByName(title.c_str());
        if (w->DockNode->IsHiddenTabBar())
        {
            w->DockNode->WantHiddenTabBarToggle = true;
        }
    }

    const ImVec2 wPos = ImGui::GetWindowPos();
    const ImVec2 mPos = ImGui::GetMousePos();
    lastLocalMousePos = {mPos.x - wPos.x, mPos.y - wPos.y};

    windowPos = ImGui::GetWindowPos();
    const float windowSizeX = ImGui::GetContentRegionAvail().x +
                              ImGui::GetCursorScreenPos().x -
                              ImGui::GetWindowPos().x +
                              8;
    const float windowSizeY = ImGui::GetContentRegionAvail().y +
                              ImGui::GetCursorScreenPos().y -
                              ImGui::GetWindowPos().y +
                              8;
    windowSize = {windowSizeX, windowSizeY};

    if (!framebuffer.created)
    {
        this->framebuffer = GLHelper::CreateFramebuffer({windowSize.x, windowSize.y});
    } else if (glm::vec2(windowSize.x, windowSize.y) != framebuffer.size)
    {
        GLHelper::ResizeFramebuffer(framebuffer, {windowSize.x, windowSize.y});
    }

    if (GetType() == ViewportType::PERSPECTIVE)
    {
        if (!worldBuffers.created)
        {
            CreateWorldBuffers(glm::vec2(windowSize.x, windowSize.y));
        } else if (glm::vec2(windowSize.x, windowSize.y) != worldBuffers.size)
        {
            ResizeWorldBuffers(glm::vec2(windowSize.x, windowSize.y));
        }
    }

    GLHelper::BindFramebuffer(framebuffer);

    RecalculateMatrices();

    MapEditor::tool->RenderViewport(*this);

    if (type == ViewportType::PERSPECTIVE)
    {
        UpdatePerspectiveHover();
    }
    GLHelper::UnbindFramebuffer();

    const ImVec2 origCursor = ImGui::GetCursorPos();
    ImGui::SetCursorPos({0, 0});
    if (type == ViewportType::PERSPECTIVE && MapEditor::showInternalBuffers)
    {
        const ImVec2 imgSize = {windowSize.x / 2, windowSize.y / 2};
        ImGui::Image(framebuffer.colorTexture, imgSize, {0, 1}, {1, 0});
        ImGui::SetCursorPos({imgSize.x, 0});
        ImGui::Image(worldBuffers.objectInfoTexture, imgSize, {0, 1}, {1, 0});
        ImGui::SetCursorPos({0, imgSize.y});
        ImGui::Image(worldBuffers.positionTexture, imgSize, {0, 1}, {1, 0});
        ImGui::SetCursorPos({imgSize.x, imgSize.y});
        ImGui::Image(worldBuffers.normalTexture, imgSize, {0, 1}, {1, 0});
    } else
    {
        ImGui::Image(framebuffer.colorTexture, {framebuffer.size.x, framebuffer.size.y}, {0, 1}, {1, 0});
    }
    ImGui::SetCursorPos(origCursor);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetColorU32(ImGuiCol_WindowBg));

    constexpr ImGuiChildFlags CHILD_FLAGS = ImGuiChildFlags_AlwaysAutoResize |
                                            ImGuiChildFlags_AutoResizeX |
                                            ImGuiChildFlags_AutoResizeY |
                                            ImGuiChildFlags_Borders;
    if (MapEditor::drawViewportInfo && ImGui::BeginChild("_vp_stats", ImVec2(0, 0), CHILD_FLAGS))
    {
        if (type != ViewportType::PERSPECTIVE)
        {
            ImGui::Text("Pos: %.2f, %.2f\nZoom: %.2f units/screen\nGrid: %.3f units",
                        scrollCenterPos.x,
                        scrollCenterPos.y,
                        zoom,
                        MapEditor::GRID_SPACING_VALUES.at(MapEditor::gridSpacingIndex));
        } else
        {
            ImGui::Text("Pos: %.2f, %.2f, %.2f\nRotation: %.2f, %.2f\nSpeed: %.2f",
                        perspectiveCameraPos.x,
                        perspectiveCameraPos.y,
                        perspectiveCameraPos.z,
                        glm::degrees(perspectiveCameraRotation.x),
                        glm::degrees(perspectiveCameraRotation.y),
                        perspectiveCameraSpeed);
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

    if (ImGui::IsWindowFocused() && ImGui::IsWindowHovered())
    {
        if (type != ViewportType::PERSPECTIVE)
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
            ChangeZoom(ImGui::GetIO().MouseWheel * -ZOOM_STEP);
        } else
        {
            ImGuiIO &io = ImGui::GetIO();
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
            {
                io.WantCaptureMouse = false;
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                const ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
                perspectiveCameraRotation.x += glm::radians(dragDelta.y / -5.0f);
                perspectiveCameraRotation.y += glm::radians(dragDelta.x / 5.0f);
                perspectiveCameraRotation.x = glm::clamp(perspectiveCameraRotation.x,
                                                         glm::radians(-89.0f),
                                                         glm::radians(89.0f));
                ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
            }
            glm::vec3 moveDir{};
            if (ImGui::IsKeyDown(ImGuiKey_W))
            {
                moveDir.z = 1;
            } else if (ImGui::IsKeyDown(ImGuiKey_S))
            {
                moveDir.z = -1;
            }
            if (ImGui::IsKeyDown(ImGuiKey_A))
            {
                moveDir.x = -1;
            } else if (ImGui::IsKeyDown(ImGuiKey_D))
            {
                moveDir.x = 1;
            }

            perspectiveCameraSpeed += ImGui::GetIO().MouseWheel;
            perspectiveCameraSpeed = glm::clamp(perspectiveCameraSpeed, 1.0f, 128.0f);

            const glm::vec3 forward = GetPerspectiveForward();
            const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
            perspectiveCameraPos += (forward * moveDir.z + right * moveDir.x) * perspectiveCameraSpeed;
        }
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
    const glm::vec4 clipPos = glm::vec4(ndcPos2d, 0.0f, 1.0f);
    const glm::vec4 worldPos = inverseWorldScreenMatrix * clipPos;
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
    ImVec2 igWindowSize;
    ImVec2 igWindowPos;
    GetWindowRect(igWindowPos, igWindowSize);
    const glm::vec4 fullNdc = worldScreenMatrix * glm::vec4(worldPos, 1.0f);
    const glm::vec2 ndc = glm::vec2(fullNdc.x, fullNdc.y);
    const float screenX = (ndc.x * 0.5f + 0.5f) * igWindowSize.x;
    const float screenY = (-ndc.y * 0.5f + 0.5f) * igWindowSize.y;
    return {screenX, screenY};
}

float Viewport::PixelsToWorldDistance(const float pixels) const
{
    return (1.0f / (windowSize.y / zoom)) * pixels;
}

glm::mat4 Viewport::GetMatrix() const
{
    return worldScreenMatrix;
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

float Viewport::GetZoom() const
{
    return zoom;
}

ImVec2 Viewport::GetLocalMousePos() const
{
    return lastLocalMousePos;
}

glm::vec3 Viewport::GetWorldSpaceMousePos() const
{
    return ScreenToWorldPos(lastLocalMousePos);
}

void Viewport::ChangeZoom(const float by)
{
    zoom = glm::clamp(zoom + by, 20.0f, MapEditor::MAP_SIZE + 1000.0f);
    // TODO: zoom around mouse cursor instead of origin
}

glm::vec2 Viewport::GetWorldSpaceSize() const
{
    const float width = (windowSize.x / windowSize.y) * zoom;
    return {width, zoom};
}

glm::vec3 Viewport::GetCameraPos() const
{
    return ScreenToWorldPos(ImVec2(windowSize.x / 2, windowSize.y / 2));
}

void Viewport::RecalculateMatrices()
{
    const float aspect = (framebuffer.size.y != 0.0f) ? (framebuffer.size.x / framebuffer.size.y) : 1.0f;
    const float halfHeight = zoom / 2.0f;
    const float halfWidth = aspect * halfHeight;

    if (type != ViewportType::PERSPECTIVE)
    {
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
            up = glm::vec3(0, 0, -1);
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
        worldScreenMatrix = ortho * view;
    } else
    {
        const glm::mat4 perspectiveMatrix = glm::perspective(glm::radians(90.0f),
                                                             aspect,
                                                             MapAsset::MAP_MAX_HALF_EXTENTS * 3.0f,
                                                             0.01f);

        const glm::mat4 view = glm::lookAt(perspectiveCameraPos,
                                           perspectiveCameraPos + GetPerspectiveForward(),
                                           glm::vec3(0, 1, 0));

        worldScreenMatrix = perspectiveMatrix * view;
    }

    inverseWorldScreenMatrix = glm::inverse(worldScreenMatrix);
}

glm::vec3 Viewport::Make3D(const glm::vec2 twoDimensionalComponent, const float otherAxis) const
{
    return AxisHelper::Make3D(GetAxis(), twoDimensionalComponent, otherAxis);
}

glm::vec2 Viewport::Make2D(const glm::vec3 point) const
{
    return AxisHelper::Make2D(GetAxis(), point);
}

Axis Viewport::GetAxis() const
{
    assert(type != ViewportType::PERSPECTIVE);
    switch (type)
    {
        default:
        case ViewportType::TOP_DOWN_XZ:
            return Axis::Y;
        case ViewportType::FRONT_XY:
            return Axis::Z;
        case ViewportType::SIDE_YZ:
            return Axis::X;
    }
}

bool Viewport::Is3D() const
{
    return type == ViewportType::PERSPECTIVE;
}

glm::vec3 Viewport::GetPerspectiveForward() const
{
    const float pitch = perspectiveCameraRotation.x;
    const float yaw = perspectiveCameraRotation.y;
    return {glm::sin(yaw) * glm::cos(pitch), glm::sin(pitch), -glm::cos(yaw) * glm::cos(pitch)};
}

glm::vec3 Viewport::GetPerspectiveCameraPosition() const
{
    return perspectiveCameraPos;
}

void Viewport::CreateWorldBuffers(const glm::vec2 size)
{
    glGenFramebuffers(1, &worldBuffers.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, worldBuffers.fbo);
    glGenRenderbuffers(1, &worldBuffers.rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, worldBuffers.rbo);

    glGenTextures(1, &worldBuffers.objectInfoTexture);
    glBindTexture(GL_TEXTURE_2D, worldBuffers.objectInfoTexture);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGB32F,
                 static_cast<GLsizei>(size.x),
                 static_cast<GLsizei>(size.y),
                 0,
                 GL_RGB,
                 GL_FLOAT,
                 nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, worldBuffers.objectInfoTexture, 0);

    glGenTextures(1, &worldBuffers.positionTexture);
    glBindTexture(GL_TEXTURE_2D, worldBuffers.positionTexture);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGB32F,
                 static_cast<GLsizei>(size.x),
                 static_cast<GLsizei>(size.y),
                 0,
                 GL_RGB,
                 GL_FLOAT,
                 nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, worldBuffers.positionTexture, 0);

    glGenTextures(1, &worldBuffers.normalTexture);
    glBindTexture(GL_TEXTURE_2D, worldBuffers.normalTexture);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGB32F,
                 static_cast<GLsizei>(size.x),
                 static_cast<GLsizei>(size.y),
                 0,
                 GL_RGB,
                 GL_FLOAT,
                 nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, worldBuffers.normalTexture, 0);

    glRenderbufferStorage(GL_RENDERBUFFER,
                          GL_DEPTH24_STENCIL8,
                          static_cast<GLsizei>(size.x),
                          static_cast<GLsizei>(size.y));
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, worldBuffers.rbo);

    constexpr std::array<GLenum, 3> COLOR_ATTACHMENTS = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2,
    };
    glDrawBuffers(COLOR_ATTACHMENTS.size(), COLOR_ATTACHMENTS.data());

    const GLenum frameBufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    assert(frameBufferStatus == GL_FRAMEBUFFER_COMPLETE);

    GLHelper::UnbindFramebuffer();

    worldBuffers.size = size;
    worldBuffers.created = true;
}

void Viewport::ResizeWorldBuffers(const glm::vec2 size)
{
    if (worldBuffers.size == size)
    {
        return;
    }

    GLint boundRbo = GL_NONE;
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &boundRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, worldBuffers.rbo);

    glBindTexture(GL_TEXTURE_2D, worldBuffers.objectInfoTexture);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGB32F,
                 static_cast<GLsizei>(size.x),
                 static_cast<GLsizei>(size.y),
                 0,
                 GL_RGB,
                 GL_FLOAT,
                 nullptr);

    glBindTexture(GL_TEXTURE_2D, worldBuffers.positionTexture);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGB32F,
                 static_cast<GLsizei>(size.x),
                 static_cast<GLsizei>(size.y),
                 0,
                 GL_RGB,
                 GL_FLOAT,
                 nullptr);

    glBindTexture(GL_TEXTURE_2D, worldBuffers.normalTexture);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGB32F,
                 static_cast<GLsizei>(size.x),
                 static_cast<GLsizei>(size.y),
                 0,
                 GL_RGB,
                 GL_FLOAT,
                 nullptr);

    glRenderbufferStorage(GL_RENDERBUFFER,
                          GL_DEPTH24_STENCIL8,
                          static_cast<GLsizei>(size.x),
                          static_cast<GLsizei>(size.y));

    glBindRenderbuffer(GL_RENDERBUFFER, boundRbo);

    worldBuffers.size = size;
}

void Viewport::DestroyWorldBuffers()
{
    worldBuffers.created = false;
    glDeleteTextures(1, &worldBuffers.objectInfoTexture);
    glDeleteTextures(1, &worldBuffers.positionTexture);
    glDeleteTextures(1, &worldBuffers.normalTexture);
    glDeleteFramebuffers(1, &worldBuffers.fbo);
    glDeleteRenderbuffers(1, &worldBuffers.rbo);
}

void Viewport::BindWorldBuffers() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, worldBuffers.fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, worldBuffers.rbo);
    glViewport(0, 0, static_cast<GLsizei>(worldBuffers.size.x), static_cast<GLsizei>(worldBuffers.size.y));
}

Viewport::PerspectiveHoverInfo Viewport::GetPerspectiveHover() const
{
    assert(type == ViewportType::PERSPECTIVE);
    return lastHoverInfo;
}

void Viewport::UpdatePerspectiveHover()
{
    assert(type == ViewportType::PERSPECTIVE);
    BindWorldBuffers();
    std::array<float, 3> rawObjectInfo{};
    lastHoverInfo = {
        .objectType = 0,
        .objectIndex = 0,
        .objectSubIndex = 0,
        .position = {},
        .normal = {},
    };

    const GLint mouseX = static_cast<GLint>(lastLocalMousePos.x);
    const GLint mouseY = static_cast<GLint>(windowSize.y - lastLocalMousePos.y);

    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(mouseX, mouseY, 1, 1, GL_RGB, GL_FLOAT, rawObjectInfo.data());

    lastHoverInfo.objectType = *reinterpret_cast<uint8_t *>(&rawObjectInfo.at(0));

    if (lastHoverInfo.objectType == static_cast<uint8_t>(EditorTool::ItemType::NONE))
    {
        GLHelper::UnbindFramebuffer();
        return; // no need to bother reading the other values if nothing is hovered
    }

    lastHoverInfo.objectIndex = *reinterpret_cast<uint32_t *>(&rawObjectInfo.at(1));
    lastHoverInfo.objectSubIndex = *reinterpret_cast<uint32_t *>(&rawObjectInfo.at(2));

    glReadBuffer(GL_COLOR_ATTACHMENT1);
    glReadPixels(mouseX, mouseY, 1, 1, GL_RGB, GL_FLOAT, glm::value_ptr(lastHoverInfo.position));

    glReadBuffer(GL_COLOR_ATTACHMENT2);
    glReadPixels(mouseX, mouseY, 1, 1, GL_RGB, GL_FLOAT, glm::value_ptr(lastHoverInfo.normal));
}
