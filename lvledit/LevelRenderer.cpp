//
// Created by droc101 on 9/5/25.
//

#include "LevelRenderer.h"
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include "GLHelper.h"
#include "imgui.h"
#include "LevelEditor.h"
#include "libassets/util/Color.h"
#include "libassets/util/Error.h"
#include "Viewport.h"

bool LevelRenderer::Init()
{
    if (!GLHelper::Init())
    {
        return false;
    }

    const Error::ErrorCode linesProgramErrorCode = GLHelper::CreateProgram("assets/basicVertexColor.frag",
                                                                           "assets/basicVertexColor.vert",
                                                                           lineProgram);

    const Error::ErrorCode gridProgramErrorCode = GLHelper::CreateProgram("assets/grid.frag",
                                                                          "assets/grid.vert",
                                                                          gridProgram);

    const Error::ErrorCode cubeProgramErrorCode = GLHelper::CreateProgram("assets/lvleditgeneric.frag",
                                                                          "assets/lvleditgeneric.vert",
                                                                          genericProgram);
    if (cubeProgramErrorCode != Error::ErrorCode::OK ||
        linesProgramErrorCode != Error::ErrorCode::OK ||
        gridProgramErrorCode != Error::ErrorCode::OK)
    {
        return false;
    }

    axisHelperBuffer = GLHelper::CreateBuffer();
    // clang-format off
    const std::vector<float> axisHelperVerts = {
        0, -LevelEditor::LEVEL_HALF_SIZE, 0, 0, 1, 0,
        0, LevelEditor::LEVEL_HALF_SIZE, 0, 0, 1, 0,
        -LevelEditor::LEVEL_HALF_SIZE, 0, 0, 1, 0, 0,
        LevelEditor::LEVEL_HALF_SIZE, 0, 0, 1, 0, 0,
        0, 0, -LevelEditor::LEVEL_HALF_SIZE, 0, 0, 1,
        0, 0, LevelEditor::LEVEL_HALF_SIZE, 0, 0, 1,
    };
    // clang-format on
    GLHelper::BindBuffer(axisHelperBuffer);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(sizeof(float) * axisHelperVerts.size()),
                 axisHelperVerts.data(),
                 GL_STATIC_DRAW);

    worldBorderBuffer = GLHelper::CreateBuffer();
    // clang-format off
    const std::vector<float> verts = {
        -LevelEditor::LEVEL_HALF_SIZE, -LevelEditor::LEVEL_HALF_SIZE, -LevelEditor::LEVEL_HALF_SIZE, 0.5, 0.5, 0.5,
        LevelEditor::LEVEL_HALF_SIZE, -LevelEditor::LEVEL_HALF_SIZE, -LevelEditor::LEVEL_HALF_SIZE, 0.5, 0.5, 0.5,
        LevelEditor::LEVEL_HALF_SIZE, -LevelEditor::LEVEL_HALF_SIZE, -LevelEditor::LEVEL_HALF_SIZE, 0.5, 0.5, 0.5,
        LevelEditor::LEVEL_HALF_SIZE, LevelEditor::LEVEL_HALF_SIZE, -LevelEditor::LEVEL_HALF_SIZE, 0.5, 0.5, 0.5,
        LevelEditor::LEVEL_HALF_SIZE, LevelEditor::LEVEL_HALF_SIZE, -LevelEditor::LEVEL_HALF_SIZE, 0.5, 0.5, 0.5,
        -LevelEditor::LEVEL_HALF_SIZE, LevelEditor::LEVEL_HALF_SIZE, -LevelEditor::LEVEL_HALF_SIZE, 0.5, 0.5, 0.5,
        -LevelEditor::LEVEL_HALF_SIZE, LevelEditor::LEVEL_HALF_SIZE, -LevelEditor::LEVEL_HALF_SIZE, 0.5, 0.5, 0.5,
        -LevelEditor::LEVEL_HALF_SIZE, -LevelEditor::LEVEL_HALF_SIZE, -LevelEditor::LEVEL_HALF_SIZE, 0.5, 0.5, 0.5,
        -LevelEditor::LEVEL_HALF_SIZE, -LevelEditor::LEVEL_HALF_SIZE, LevelEditor::LEVEL_HALF_SIZE, 0.5, 0.5, 0.5,
        LevelEditor::LEVEL_HALF_SIZE, -LevelEditor::LEVEL_HALF_SIZE, LevelEditor::LEVEL_HALF_SIZE, 0.5, 0.5, 0.5,
        LevelEditor::LEVEL_HALF_SIZE, -LevelEditor::LEVEL_HALF_SIZE, LevelEditor::LEVEL_HALF_SIZE, 0.5, 0.5, 0.5,
        LevelEditor::LEVEL_HALF_SIZE, LevelEditor::LEVEL_HALF_SIZE, LevelEditor::LEVEL_HALF_SIZE, 0.5, 0.5, 0.5,
        LevelEditor::LEVEL_HALF_SIZE, LevelEditor::LEVEL_HALF_SIZE, LevelEditor::LEVEL_HALF_SIZE, 0.5, 0.5, 0.5,
        -LevelEditor::LEVEL_HALF_SIZE, LevelEditor::LEVEL_HALF_SIZE, LevelEditor::LEVEL_HALF_SIZE, 0.5, 0.5, 0.5,
        -LevelEditor::LEVEL_HALF_SIZE, LevelEditor::LEVEL_HALF_SIZE, LevelEditor::LEVEL_HALF_SIZE, 0.5, 0.5, 0.5,
        -LevelEditor::LEVEL_HALF_SIZE, -LevelEditor::LEVEL_HALF_SIZE, LevelEditor::LEVEL_HALF_SIZE, 0.5, 0.5, 0.5,
        -LevelEditor::LEVEL_HALF_SIZE, -LevelEditor::LEVEL_HALF_SIZE, -LevelEditor::LEVEL_HALF_SIZE, 0.5, 0.5, 0.5,
        -LevelEditor::LEVEL_HALF_SIZE, -LevelEditor::LEVEL_HALF_SIZE, LevelEditor::LEVEL_HALF_SIZE, 0.5, 0.5, 0.5,
        LevelEditor::LEVEL_HALF_SIZE, -LevelEditor::LEVEL_HALF_SIZE, -LevelEditor::LEVEL_HALF_SIZE, 0.5, 0.5, 0.5,
        LevelEditor::LEVEL_HALF_SIZE, -LevelEditor::LEVEL_HALF_SIZE, LevelEditor::LEVEL_HALF_SIZE, 0.5, 0.5, 0.5,
        LevelEditor::LEVEL_HALF_SIZE, LevelEditor::LEVEL_HALF_SIZE, -LevelEditor::LEVEL_HALF_SIZE, 0.5, 0.5, 0.5,
        LevelEditor::LEVEL_HALF_SIZE, LevelEditor::LEVEL_HALF_SIZE, LevelEditor::LEVEL_HALF_SIZE, 0.5, 0.5, 0.5,
        -LevelEditor::LEVEL_HALF_SIZE, LevelEditor::LEVEL_HALF_SIZE, -LevelEditor::LEVEL_HALF_SIZE, 0.5, 0.5, 0.5,
        -LevelEditor::LEVEL_HALF_SIZE, LevelEditor::LEVEL_HALF_SIZE, LevelEditor::LEVEL_HALF_SIZE, 0.5, 0.5, 0.5,
    };
    // clang-format on
    GLHelper::BindBuffer(worldBorderBuffer);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(float) * verts.size()), verts.data(), GL_STATIC_DRAW);

    workBuffer = GLHelper::CreateIndexedBuffer();
    workBufferNonIndexed = GLHelper::CreateBuffer();

    return true;
}

void LevelRenderer::Destroy()
{
    glDeleteProgram(genericProgram);
    glDeleteProgram(lineProgram);
    GLHelper::DestroyBuffer(axisHelperBuffer);
    GLHelper::DestroyBuffer(worldBorderBuffer);
}

void LevelRenderer::RenderViewport(const Viewport &vp)
{
    ImVec2 WindowSize;
    ImVec2 WindowPos;
    vp.GetWindowRect(WindowPos, WindowSize);
    WindowPos.y = ImGui::GetMainViewport()->Size.y - 1 - (WindowPos.y + WindowSize.y);
    glClearColor(0, 0, 0, 1);
    glViewport(static_cast<GLint>(WindowPos.x),
               static_cast<GLint>(WindowPos.y),
               static_cast<GLint>(WindowSize.x),
               static_cast<GLint>(WindowSize.y));
    glEnable(GL_SCISSOR_TEST);
    glScissor(static_cast<GLint>(WindowPos.x),
              static_cast<GLint>(WindowPos.y),
              static_cast<GLint>(WindowSize.x),
              static_cast<GLint>(WindowSize.y));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLineWidth(1);

    const glm::mat4 view = vp.GetMatrix();

    glDisable(GL_DEPTH_TEST);

    if (LevelEditor::drawGrid)
    {
        const float gridSpacing = LevelEditor::GRID_SPACING_VALUES.at(LevelEditor::gridSpacingIndex);
        const int numInstances = static_cast<int>(LevelEditor::LEVEL_SIZE * 2 / gridSpacing);

        glUseProgram(gridProgram);
        glUniformMatrix4fv(glGetUniformLocation(gridProgram, "matrix"), 1, GL_FALSE, glm::value_ptr(view));
        glUniform1f(glGetUniformLocation(gridProgram, "spacing"), gridSpacing);
        glUniform1i(glGetUniformLocation(gridProgram, "plane"), static_cast<int>(vp.GetType()));
        glDrawArraysInstanced(GL_LINES, 0, 2, numInstances);
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glUseProgram(lineProgram);
    glUniformMatrix4fv(glGetUniformLocation(lineProgram, "VIEW"), 1, GL_FALSE, glm::value_ptr(view));
    const GLint posAttrib = glGetAttribLocation(lineProgram, "VERTEX");
    const GLint colorAttrib = glGetAttribLocation(lineProgram, "VERTEX_COLOR");
    if (LevelEditor::drawAxisHelper)
    {
        GLHelper::BindBuffer(axisHelperBuffer);
        glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
        glVertexAttribPointer(colorAttrib,
                              3,
                              GL_FLOAT,
                              GL_FALSE,
                              6 * sizeof(float),
                              reinterpret_cast<void *>(3 * sizeof(float)));
        glEnableVertexAttribArray(posAttrib);
        glEnableVertexAttribArray(colorAttrib);
        glDrawArrays(GL_LINES, 0, 6);
    }
    if (LevelEditor::drawWorldBorder)
    {
        GLHelper::BindBuffer(worldBorderBuffer);
        glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
        glVertexAttribPointer(colorAttrib,
                              3,
                              GL_FLOAT,
                              GL_FALSE,
                              6 * sizeof(float),
                              reinterpret_cast<void *>(3 * sizeof(float)));
        glEnableVertexAttribArray(posAttrib);
        glEnableVertexAttribArray(colorAttrib);
        glDrawArrays(GL_LINES, 0, 24);
    }

    glClear(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_CULL_FACE);
}

void LevelRenderer::RenderLine(const glm::vec3 start,
                               const glm::vec3 end,
                               Color color,
                               glm::mat4 &matrix,
                               const float thickness)
{
    glUseProgram(genericProgram);

    glLineWidth(thickness);

    glUniform4fv(glGetUniformLocation(genericProgram, "alb"), 1, color.GetDataPointer());
    glUniformMatrix4fv(glGetUniformLocation(genericProgram, "MATRIX"), 1, GL_FALSE, glm::value_ptr(matrix));

    const std::vector<float> vertices = {
        start.x,
        start.y,
        start.z,
        end.x,
        end.y,
        end.z,
    };

    GLHelper::BindBuffer(workBufferNonIndexed);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(GLfloat)),
                 vertices.data(),
                 GL_STREAM_DRAW);
    const GLint posAttrLoc = glGetAttribLocation(genericProgram, "VERTEX");
    glVertexAttribPointer(posAttrLoc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(posAttrLoc);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices.size()));
}

void LevelRenderer::RenderBillboardPoint(glm::vec3 position, float pointSize, Color color, glm::mat4 &matrix)
{
    glUseProgram(genericProgram);

    glPointSize(pointSize);

    glUniform4fv(glGetUniformLocation(genericProgram, "alb"), 1, color.GetDataPointer());
    glUniformMatrix4fv(glGetUniformLocation(genericProgram, "MATRIX"), 1, GL_FALSE, glm::value_ptr(matrix));

    const std::vector<float> vertices = {position.x, position.y, position.z};

    GLHelper::BindBuffer(workBufferNonIndexed);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(GLfloat)),
                 vertices.data(),
                 GL_STREAM_DRAW);
    const GLint posAttrLoc = glGetAttribLocation(genericProgram, "VERTEX");
    glVertexAttribPointer(posAttrLoc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(posAttrLoc);
    glDrawArrays(GL_POINTS, 0, 1);
}
