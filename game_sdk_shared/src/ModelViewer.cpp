//
// Created by droc101 on 2/17/26.
//

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <game_sdk/gl/GLHelper.h>
#include <game_sdk/ModelViewer.h>
#include <game_sdk/SharedMgr.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <libassets/asset/ModelAsset.h>
#include <libassets/type/Material.h>
#include <libassets/type/ModelLod.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <utility>
#include <vector>

bool ModelViewer::Init()
{
    framebuffer = GLHelper::CreateFramebuffer({800, 600});

    const Error::ErrorCode modelProgramErrorCode = GLHelper::CreateProgram("assets/shaders/model.frag",
                                                                           "assets/shaders/model.vert",
                                                                           program);
    const Error::ErrorCode cubeProgramErrorCode = GLHelper::CreateProgram("assets/shaders/uniform_color.frag",
                                                                          "assets/shaders/uniform_color.vert",
                                                                          linesProgram);
    if (modelProgramErrorCode != Error::ErrorCode::OK || cubeProgramErrorCode != Error::ErrorCode::OK)
    {
        return false;
    }

    LoadCube();

    LoadBBox();

    glGenVertexArrays(1, &staticCollisionVao);
    glBindVertexArray(staticCollisionVao);

    glGenBuffers(1, &staticCollisionVbo);

    UpdateView(0, 0, 1);

    return true;
}

void ModelViewer::Destroy()
{
    DestroyModel();
    GLHelper::DestroyFramebuffer(framebuffer);
    GLHelper::DestroyBuffer(cubeBuffer);
    GLHelper::DestroyIndexedBuffer(bboxBuffer);
    glDeleteProgram(program);
    glDeleteProgram(linesProgram);
}

void ModelViewer::SetModel(ModelAsset &&newModel)
{
    model = std::move(newModel);
    lodIndex = 0;
    skinIndex = 0;

    ReloadModel();
}

void ModelViewer::ReloadModel()
{
    DestroyModel();
    for (size_t i = 0; i < model.GetLodCount(); i++)
    {
        const ModelLod &lod = model.GetLod(i);
        GLModelLod glod;
        glGenVertexArrays(1, &glod.vao);
        glBindVertexArray(glod.vao);
        glGenBuffers(1, &glod.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, glod.vbo);
        DataWriter writer;
        model.GetVertexBuffer(i, writer);
        std::vector<uint8_t> buffer;
        writer.CopyToVector(buffer);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(writer.GetBufferSize()), buffer.data(), GL_STATIC_DRAW);

        for (size_t j = 0; j < model.GetSkinCount(); j++)
        {
            for (size_t k = 0; k < model.GetMaterialsPerSkin(); k++)
            {
                const size_t matIndex = model.GetSkin(j).at(k);
                const Material &mat = model.GetMaterial(matIndex);
                (void)SharedMgr::Get().textureCache.LoadTexture(mat.texture);
            }
        }

        for (size_t j = 0; j < model.GetMaterialsPerSkin(); j++)
        {
            GLuint ebo = 0;
            glGenBuffers(1, &ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         static_cast<GLsizeiptr>(lod.indexCounts.at(j) * sizeof(uint32_t)),
                         lod.materialIndices.at(j).data(),
                         GL_STATIC_DRAW);
            glod.ebos.push_back(ebo);
        }

        lods.push_back(glod);
    }

    LoadHulls();
    LoadStaticCollision();

    UpdateView(0, 0, 1);
}

ModelAsset &ModelViewer::GetModel()
{
    return model;
}

void ModelViewer::UpdateView(const float pitchDegrees, const float yawDegrees, const float cameraDistance)
{
    pitch = glm::radians(pitchDegrees);
    yaw = glm::radians(yawDegrees);
    distance = cameraDistance;
    if (distance < 0.1)
    {
        distance = 0.1;
    }
    pitch = glm::clamp<float>(pitch, -90, 90);
    UpdateMatrix();
}

void ModelViewer::UpdateViewRel(const float pitchDegrees, const float yawDegrees, const float cameraDistance)
{
    pitch += glm::radians(pitchDegrees);
    yaw += glm::radians(yawDegrees);
    distance += cameraDistance;
    if (distance < 0.1)
    {
        distance = 0.1;
    }
    pitch = glm::clamp<float>(pitch, -90, 90);
    UpdateMatrix();
}

void ModelViewer::RenderWindow(const char *title, const ImGuiWindowFlags additionalFlags)
{
    RenderFramebuffer();
    constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    if (ImGui::Begin(title, nullptr, windowFlags | additionalFlags))
    {
        RenderImGui();
    }
    ImGui::End();
}

void ModelViewer::RenderChildWindow(const char *title,
                                    ImGuiChildFlags additionalChildFlags,
                                    ImGuiWindowFlags additionalWindowFlags)
{
    RenderFramebuffer();
    constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    if (ImGui::BeginChild(title, {0, 0}, additionalChildFlags, windowFlags | additionalWindowFlags))
    {
        RenderImGui();
    }
    ImGui::EndChild();
}

void ModelViewer::DestroyModel()
{
    for (const GLModelLod &lod: lods)
    {
        glDeleteVertexArrays(1, &lod.vao);
        glDeleteBuffers(1, &lod.vbo);
        for (const GLuint &buffer: lod.ebos)
        {
            glDeleteBuffers(1, &buffer);
        }
    }
    lods.clear();
}

void ModelViewer::RenderImGui()
{
    ImVec2 windowSize = ImGui::GetContentRegionMax();
    windowSize.x += 8;
    windowSize.y += 8;
    ResizeWindow(static_cast<GLsizei>(windowSize.x), static_cast<GLsizei>(windowSize.y));

    const bool previewFocused = ImGui::IsWindowHovered();
    ImGui::Image(GetFramebufferTexture(), GetFramebufferSize(), {0, 1}, {1, 0});

    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && previewFocused)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        const ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
        UpdateViewRel(dragDelta.y / 5.0f, dragDelta.x / -5.0f, 0);
        ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
    }

    const float mouseWheel = ImGui::GetIO().MouseWheel;
    if (mouseWheel != 0 && previewFocused)
    {
        UpdateViewRel(0, 0, mouseWheel / -10.0f);
    }
}

void ModelViewer::RenderFramebuffer()
{
    GLHelper::BindFramebuffer(framebuffer);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    if (cullBackfaces)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    } else
    {
        glDisable(GL_CULL_FACE);
    }

    if (wireframe)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glUseProgram(program);
    const float *color = backgroundColor.GetDataPointer();
    glClearColor(color[0], color[1], color[2], color[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const GLModelLod &glod = lods.at(lodIndex);
    glBindVertexArray(glod.vao);

    glBindBuffer(GL_ARRAY_BUFFER, glod.vbo);
    GLint posAttrib = glGetAttribLocation(program, "VERTEX");
    const GLint uvAttrib = glGetAttribLocation(program, "VERTEX_UV");
    const GLint colorAttrib = glGetAttribLocation(program, "VERTEX_COLOR");
    const GLint normAttrib = glGetAttribLocation(program, "VERTEX_NORMAL");
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), nullptr);
    glVertexAttribPointer(uvAttrib,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          12 * sizeof(float),
                          reinterpret_cast<void *>(3 * sizeof(float)));
    glVertexAttribPointer(colorAttrib,
                          4,
                          GL_FLOAT,
                          GL_FALSE,
                          12 * sizeof(float),
                          reinterpret_cast<void *>(5 * sizeof(float)));
    glVertexAttribPointer(normAttrib,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          12 * sizeof(float),
                          reinterpret_cast<void *>(9 * sizeof(float)));
    glEnableVertexAttribArray(posAttrib);
    glEnableVertexAttribArray(uvAttrib);
    glEnableVertexAttribArray(colorAttrib);
    glEnableVertexAttribArray(normAttrib);

    glUniformMatrix4fv(glGetUniformLocation(program, "PROJECTION"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(program, "VIEW"), 1, GL_FALSE, glm::value_ptr(view));
    glUniform1i(glGetUniformLocation(program, "displayMode"), static_cast<GLint>(displayMode));

    for (size_t i = 0; i < model.GetMaterialsPerSkin(); i++)
    {
        const size_t matIndex = model.GetSkin(skinIndex).at(i);
        Material &mat = model.GetMaterial(matIndex);
        glUniform4fv(glGetUniformLocation(program, "ALBEDO"), 1, mat.color.GetDataPointer());

        GLuint texture = 0;
        const Error::ErrorCode code = SharedMgr::Get().textureCache.GetTextureGLuint(mat.texture, texture);
        if (code != Error::ErrorCode::OK)
        {
            continue;
        }
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(glGetUniformLocation(program, "ALBEDO_TEXTURE"), 0);

        const GLuint ebo = glod.ebos.at(i);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glDrawElements(GL_TRIANGLES,
                       static_cast<GLsizei>(model.GetLod(lodIndex).indexCounts.at(i)),
                       GL_UNSIGNED_INT,
                       nullptr);
    }

    if (showUnitCube)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glUseProgram(linesProgram);
        GLHelper::BindBuffer(cubeBuffer);
        posAttrib = glGetAttribLocation(linesProgram, "VERTEX");
        glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        glEnableVertexAttribArray(posAttrib);
        glUniformMatrix4fv(glGetUniformLocation(linesProgram, "PROJECTION"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(linesProgram, "VIEW"), 1, GL_FALSE, glm::value_ptr(view));
        glUniform4f(glGetUniformLocation(linesProgram, "lineColor"), 0.1f, 0.1f, 0.1f, 1.0f);
        glDrawArrays(GL_LINES, 0, 24);
    }

    if (showCollisionModel)
    {
        if (model.GetCollisionModelType() == ModelAsset::CollisionModelType::DYNAMIC_MULTIPLE_CONVEX)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glUseProgram(linesProgram);
            glUniformMatrix4fv(glGetUniformLocation(linesProgram, "PROJECTION"),
                               1,
                               GL_FALSE,
                               glm::value_ptr(projection));
            glUniformMatrix4fv(glGetUniformLocation(linesProgram, "VIEW"), 1, GL_FALSE, glm::value_ptr(view));
            glUniform4f(glGetUniformLocation(linesProgram, "lineColor"), 1.0f, 0.5f, 0.5f, 1.0f);
            for (const GLHull &hull: hulls)
            {
                glBindVertexArray(hull.vao);
                glBindBuffer(GL_ARRAY_BUFFER, hull.vbo);
                posAttrib = glGetAttribLocation(linesProgram, "VERTEX");
                glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
                glEnableVertexAttribArray(posAttrib);
                glPointSize(3.0);
                glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(hull.elements));
            }
        } else
        {
            glDisable(GL_CULL_FACE);
            glBindBuffer(GL_ARRAY_BUFFER, staticCollisionVbo);

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glUseProgram(linesProgram);
            glBindVertexArray(staticCollisionVao);
            posAttrib = glGetAttribLocation(linesProgram, "VERTEX");
            glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
            glEnableVertexAttribArray(posAttrib);
            glUniformMatrix4fv(glGetUniformLocation(linesProgram, "PROJECTION"),
                               1,
                               GL_FALSE,
                               glm::value_ptr(projection));
            glUniformMatrix4fv(glGetUniformLocation(linesProgram, "VIEW"), 1, GL_FALSE, glm::value_ptr(view));
            glUniform4f(glGetUniformLocation(linesProgram, "lineColor"), 1.0f, 0.5f, 0.5f, 0.25f);
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(model.GetStaticCollisionMesh().GetNumTriangles()) * 3);

            glUniform4f(glGetUniformLocation(linesProgram, "lineColor"), 1.0f, 0.5f, 0.5f, 1.0f);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(model.GetStaticCollisionMesh().GetNumTriangles()) * 3);
        }
    }

    if (showBoundingBox)
    {
        glDisable(GL_CULL_FACE);
        GLHelper::BindIndexedBuffer(bboxBuffer);
        const std::array<float, 24> points = model.GetBoundingBox().GetPointsFlat();
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * points.size(), points.data(), GL_STATIC_DRAW);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glUseProgram(linesProgram);
        posAttrib = glGetAttribLocation(linesProgram, "VERTEX");
        glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        glEnableVertexAttribArray(posAttrib);
        glUniformMatrix4fv(glGetUniformLocation(linesProgram, "PROJECTION"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(linesProgram, "VIEW"), 1, GL_FALSE, glm::value_ptr(view));
        glUniform4f(glGetUniformLocation(linesProgram, "lineColor"), 0.1f, 0.5f, 0.8f, 0.25f);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);

        glUniform4f(glGetUniformLocation(linesProgram, "lineColor"), 0.1f, 0.5f, 0.8f, 1.0f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
    }

    GLHelper::UnbindFramebuffer();
}

void ModelViewer::ResizeWindow(GLsizei width, GLsizei height)
{
    windowAspect = static_cast<float>(width) / static_cast<float>(height);
    GLHelper::ResizeFramebuffer(framebuffer, {width, height});
    UpdateMatrix();
}

ImTextureID ModelViewer::GetFramebufferTexture() const
{
    return framebuffer.colorTexture;
}

ImVec2 ModelViewer::GetFramebufferSize() const
{
    return {framebuffer.size.x, framebuffer.size.y};
}

void ModelViewer::UpdateMatrix()
{
    const glm::mat4 &persp = glm::perspective<float>(90.0, windowAspect, 0.01f, 1000.0f);

    const float x = distance * cosf(pitch) * -sinf(yaw);
    const float y = distance * sinf(pitch);
    const float z = distance * cosf(pitch) * -cosf(yaw);
    const glm::vec3 cameraPos{x, y, z};
    const glm::mat4 &look = glm::lookAt(cameraPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    projection = persp;
    view = look;
}

void ModelViewer::LoadCube()
{
    // clang-format off
    constexpr std::array cubeVerts = {
        // Bottom face
        -0.5f, -0.5f, -0.5f,   0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,   0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,  -0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,  -0.5f, -0.5f, -0.5f,

        // Top face
        -0.5f, -0.5f,  0.5f,   0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,   0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,  -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,  -0.5f, -0.5f,  0.5f,

        // Vertical edges
        -0.5f, -0.5f, -0.5f,  -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f, -0.5f,   0.5f, -0.5f,  0.5f,
         0.5f,  0.5f, -0.5f,   0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,  -0.5f,  0.5f,  0.5f,
    };
    // clang-format on

    cubeBuffer = GLHelper::CreateBuffer();
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * cubeVerts.size(), cubeVerts.data(), GL_STATIC_DRAW);
}

void ModelViewer::LoadBBox()
{
    bboxBuffer = GLHelper::CreateIndexedBuffer();
    const std::array<GLuint, 36> indices = {
        0, 1, 3, 0, 3, 2, 4, 7, 5, 4, 6, 7, 0, 5, 1, 0, 4, 5, 2, 3, 7, 2, 7, 6, 0, 2, 6, 0, 6, 4, 1, 7, 3, 1, 5, 7,
    };
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), indices.data(), GL_STATIC_DRAW);
}

void ModelViewer::LoadHulls()
{
    for (const GLHull &hull: hulls)
    {
        glDeleteBuffers(1, &hull.vbo);
        glDeleteVertexArrays(1, &hull.vao);
    }
    hulls.clear();

    for (size_t i = 0; i < model.GetNumHulls(); i++)
    {
        GLHull glHull;
        glGenVertexArrays(1, &glHull.vao);
        glGenBuffers(1, &glHull.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, glHull.vbo);
        std::vector<float> points = model.GetHull(i).GetPointsForRender();
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(sizeof(GLfloat) * points.size()),
                     points.data(),
                     GL_STATIC_DRAW);
        glHull.elements = points.size() / 3;
        hulls.push_back(glHull);
    }
}

void ModelViewer::LoadStaticCollision()
{
    glBindVertexArray(staticCollisionVao);
    glBindBuffer(GL_ARRAY_BUFFER, staticCollisionVbo);
    const std::vector<float> verts = model.GetStaticCollisionMesh().GetVerticesForRender();
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(sizeof(GLfloat) * verts.size()),
                 verts.data(),
                 GL_STATIC_DRAW);
}
