//
// Created by droc101 on 9/5/25.
//

#include "MapRenderer.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <game_sdk/gl/GLHelper.h>
#include <game_sdk/SharedMgr.h>
#include <glm/ext.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <libassets/asset/ModelAsset.h>
#include <libassets/type/Color.h>
#include <libassets/type/MapVertex.h>
#include <libassets/type/Material.h>
#include <libassets/type/ModelLod.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <libassets/util/Logger.h>
#include <libassets/util/SearchPathManager.h>
#include <ranges>
#include <string>
#include <vector>
#include "libassets/asset/LevelMaterialAsset.h"
#include "libassets/type/Brush.h"
#include "MapEditor.h"
#include "Viewport.h"

bool MapRenderer::Init()
{
    const Error::ErrorCode vertexColorProgramErrorCode = GLHelper::CreateProgram("assets/shaders/mapedit/"
                                                                                 "vertex_color.frag",
                                                                                 "assets/shaders/mapedit/"
                                                                                 "vertex_color.vert",
                                                                                 vertexColorProgram);

    const Error::ErrorCode gridProgramErrorCode = GLHelper::CreateProgram("assets/shaders/mapedit/grid.frag",
                                                                          "assets/shaders/mapedit/grid.vert",
                                                                          gridProgram);

    const Error::ErrorCode uniformColorProgramErrorCode = GLHelper::CreateProgram("assets/shaders/mapedit/"
                                                                                  "uniform_color.frag",
                                                                                  "assets/shaders/mapedit/"
                                                                                  "uniform_color.vert",
                                                                                  uniformColorProgram);

    const Error::ErrorCode spriteProgramErrorCode = GLHelper::CreateProgram("assets/shaders/mapedit/sprite.frag",
                                                                            "assets/shaders/mapedit/sprite.vert",
                                                                            spriteProgram);

    const Error::ErrorCode shadedModelProgramErrorCode = GLHelper::CreateProgram("assets/shaders/mapedit/"
                                                                                 "model_shaded.frag",
                                                                                 "assets/shaders/mapedit/"
                                                                                 "model_shaded.vert",
                                                                                 shadedModelProgram);

    const Error::ErrorCode brushProgramErrorCode = GLHelper::CreateProgram("assets/shaders/mapedit/brush.frag",
                                                                           "assets/shaders/mapedit/brush.vert",
                                                                           brushProgram);

    const Error::ErrorCode worldBuffersProgramErrorCode = GLHelper::CreateProgram("assets/shaders/mapedit/"
                                                                                  "world_buffers.frag",
                                                                                  "assets/shaders/mapedit/"
                                                                                  "world_buffers.vert",
                                                                                  worldBuffersProgram);

    if (uniformColorProgramErrorCode != Error::ErrorCode::OK ||
        vertexColorProgramErrorCode != Error::ErrorCode::OK ||
        gridProgramErrorCode != Error::ErrorCode::OK ||
        spriteProgramErrorCode != Error::ErrorCode::OK ||
        shadedModelProgramErrorCode != Error::ErrorCode::OK ||
        brushProgramErrorCode != Error::ErrorCode::OK ||
        worldBuffersProgramErrorCode != Error::ErrorCode::OK)
    {
        return false;
    }

    axisHelperBuffer = GLHelper::CreateBuffer();
    // clang-format off
    const std::vector<float> axisHelperVerts = {
        0, -MapEditor::MAP_HALF_SIZE, 0, 0, 1, 0,
        0, MapEditor::MAP_HALF_SIZE, 0, 0, 1, 0,
        -MapEditor::MAP_HALF_SIZE, 0, 0, 1, 0, 0,
        MapEditor::MAP_HALF_SIZE, 0, 0, 1, 0, 0,
        0, 0, -MapEditor::MAP_HALF_SIZE, 0, 0, 1,
        0, 0, MapEditor::MAP_HALF_SIZE, 0, 0, 1,
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
        -MapEditor::MAP_HALF_SIZE, -MapEditor::MAP_HALF_SIZE, -MapEditor::MAP_HALF_SIZE, 0.5, 0.5, 0.5,
        MapEditor::MAP_HALF_SIZE, -MapEditor::MAP_HALF_SIZE, -MapEditor::MAP_HALF_SIZE, 0.5, 0.5, 0.5,
        MapEditor::MAP_HALF_SIZE, -MapEditor::MAP_HALF_SIZE, -MapEditor::MAP_HALF_SIZE, 0.5, 0.5, 0.5,
        MapEditor::MAP_HALF_SIZE, MapEditor::MAP_HALF_SIZE, -MapEditor::MAP_HALF_SIZE, 0.5, 0.5, 0.5,
        MapEditor::MAP_HALF_SIZE, MapEditor::MAP_HALF_SIZE, -MapEditor::MAP_HALF_SIZE, 0.5, 0.5, 0.5,
        -MapEditor::MAP_HALF_SIZE, MapEditor::MAP_HALF_SIZE, -MapEditor::MAP_HALF_SIZE, 0.5, 0.5, 0.5,
        -MapEditor::MAP_HALF_SIZE, MapEditor::MAP_HALF_SIZE, -MapEditor::MAP_HALF_SIZE, 0.5, 0.5, 0.5,
        -MapEditor::MAP_HALF_SIZE, -MapEditor::MAP_HALF_SIZE, -MapEditor::MAP_HALF_SIZE, 0.5, 0.5, 0.5,
        -MapEditor::MAP_HALF_SIZE, -MapEditor::MAP_HALF_SIZE, MapEditor::MAP_HALF_SIZE, 0.5, 0.5, 0.5,
        MapEditor::MAP_HALF_SIZE, -MapEditor::MAP_HALF_SIZE, MapEditor::MAP_HALF_SIZE, 0.5, 0.5, 0.5,
        MapEditor::MAP_HALF_SIZE, -MapEditor::MAP_HALF_SIZE, MapEditor::MAP_HALF_SIZE, 0.5, 0.5, 0.5,
        MapEditor::MAP_HALF_SIZE, MapEditor::MAP_HALF_SIZE, MapEditor::MAP_HALF_SIZE, 0.5, 0.5, 0.5,
        MapEditor::MAP_HALF_SIZE, MapEditor::MAP_HALF_SIZE, MapEditor::MAP_HALF_SIZE, 0.5, 0.5, 0.5,
        -MapEditor::MAP_HALF_SIZE, MapEditor::MAP_HALF_SIZE, MapEditor::MAP_HALF_SIZE, 0.5, 0.5, 0.5,
        -MapEditor::MAP_HALF_SIZE, MapEditor::MAP_HALF_SIZE, MapEditor::MAP_HALF_SIZE, 0.5, 0.5, 0.5,
        -MapEditor::MAP_HALF_SIZE, -MapEditor::MAP_HALF_SIZE, MapEditor::MAP_HALF_SIZE, 0.5, 0.5, 0.5,
        -MapEditor::MAP_HALF_SIZE, -MapEditor::MAP_HALF_SIZE, -MapEditor::MAP_HALF_SIZE, 0.5, 0.5, 0.5,
        -MapEditor::MAP_HALF_SIZE, -MapEditor::MAP_HALF_SIZE, MapEditor::MAP_HALF_SIZE, 0.5, 0.5, 0.5,
        MapEditor::MAP_HALF_SIZE, -MapEditor::MAP_HALF_SIZE, -MapEditor::MAP_HALF_SIZE, 0.5, 0.5, 0.5,
        MapEditor::MAP_HALF_SIZE, -MapEditor::MAP_HALF_SIZE, MapEditor::MAP_HALF_SIZE, 0.5, 0.5, 0.5,
        MapEditor::MAP_HALF_SIZE, MapEditor::MAP_HALF_SIZE, -MapEditor::MAP_HALF_SIZE, 0.5, 0.5, 0.5,
        MapEditor::MAP_HALF_SIZE, MapEditor::MAP_HALF_SIZE, MapEditor::MAP_HALF_SIZE, 0.5, 0.5, 0.5,
        -MapEditor::MAP_HALF_SIZE, MapEditor::MAP_HALF_SIZE, -MapEditor::MAP_HALF_SIZE, 0.5, 0.5, 0.5,
        -MapEditor::MAP_HALF_SIZE, MapEditor::MAP_HALF_SIZE, MapEditor::MAP_HALF_SIZE, 0.5, 0.5, 0.5,
    };
    // clang-format on
    GLHelper::BindBuffer(worldBorderBuffer);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(float) * verts.size()), verts.data(), GL_STATIC_DRAW);

    workBuffer = GLHelper::CreateIndexedBuffer();
    workBufferNonIndexed = GLHelper::CreateBuffer();

    const std::string errorModelPath = SharedMgr::Get().pathManager.GetAssetPath("model/error.gmdl");
    if (errorModelPath.empty())
    {
        Logger::Error("Failed to load models/error.gmdl");
        return false;
    }

    const ModelBuffer buf = LoadModel(errorModelPath);
    modelBuffers["model/error.gmdl"] = buf;

    return true;
}

void MapRenderer::Destroy()
{
    glDeleteProgram(uniformColorProgram);
    glDeleteProgram(vertexColorProgram);
    glDeleteProgram(gridProgram);
    glDeleteProgram(spriteProgram);
    GLHelper::DestroyBuffer(axisHelperBuffer);
    GLHelper::DestroyBuffer(worldBorderBuffer);
    for (const ModelBuffer &buffer: modelBuffers | std::views::values)
    {
        glDeleteVertexArrays(1, &buffer.vao);
        glDeleteBuffers(1, &buffer.vbo);
        for (const GLuint &ebo: buffer.ebos)
        {
            glDeleteBuffers(1, &ebo);
        }
    }
}

void MapRenderer::RenderViewportGrid(const Viewport &vp)
{
    glLineWidth(1);

    glm::mat4 view = vp.GetMatrix();

    glDisable(GL_DEPTH_TEST);

    if (MapEditor::drawGrid && vp.GetType() != Viewport::ViewportType::PERSPECTIVE)
    {
        const float gridSpacing = MapEditor::GRID_SPACING_VALUES.at(MapEditor::gridSpacingIndex);
        const int numInstances = static_cast<int>(MapEditor::MAP_SIZE * 2 / gridSpacing);

        glUseProgram(gridProgram);
        glUniformMatrix4fv(glGetUniformLocation(gridProgram, "VIEW_MATRIX"), 1, GL_FALSE, glm::value_ptr(view));
        glUniform1f(glGetUniformLocation(gridProgram, "SPACING"), gridSpacing);
        glUniform1i(glGetUniformLocation(gridProgram, "PLANE"), static_cast<int>(vp.GetType()));
        glDrawArraysInstanced(GL_LINES, 0, 2, numInstances);
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glUseProgram(vertexColorProgram);
    glUniformMatrix4fv(glGetUniformLocation(vertexColorProgram, "VIEW_MATRIX"), 1, GL_FALSE, glm::value_ptr(view));
    const GLint posAttrib = glGetAttribLocation(vertexColorProgram, "VERTEX");
    const GLint colorAttrib = glGetAttribLocation(vertexColorProgram, "VERTEX_COLOR");
    if (MapEditor::drawAxisHelper)
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
    if (MapEditor::drawWorldBorder)
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

    // RenderModel(testModel, view, identity);
}

void MapRenderer::RenderLine(const glm::vec3 start,
                             const glm::vec3 end,
                             Color color,
                             const glm::mat4 &matrix,
                             const float thickness)
{
    glUseProgram(uniformColorProgram);

    glLineWidth(thickness);

    glUniform4fv(glGetUniformLocation(uniformColorProgram, "COLOR"), 1, color.GetDataPointer());
    glUniformMatrix4fv(glGetUniformLocation(uniformColorProgram, "VIEW_MATRIX"), 1, GL_FALSE, glm::value_ptr(matrix));
    glUniformMatrix4fv(glGetUniformLocation(uniformColorProgram, "WORLD_MATRIX"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(identity));

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
    const GLint posAttrLoc = glGetAttribLocation(uniformColorProgram, "VERTEX");
    glVertexAttribPointer(posAttrLoc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(posAttrLoc);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices.size()));
}

void MapRenderer::RenderBillboardPoint(const glm::vec3 position,
                                       const float pointSize,
                                       Color color,
                                       const glm::mat4 &matrix)
{
    glUseProgram(uniformColorProgram);

    glPointSize(pointSize);

    glUniform4fv(glGetUniformLocation(uniformColorProgram, "COLOR"), 1, color.GetDataPointer());
    glUniformMatrix4fv(glGetUniformLocation(uniformColorProgram, "VIEW_MATRIX"), 1, GL_FALSE, glm::value_ptr(matrix));
    glUniformMatrix4fv(glGetUniformLocation(uniformColorProgram, "WORLD_MATRIX"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(identity));

    const std::vector<float> vertices = {position.x, position.y, position.z};

    GLHelper::BindBuffer(workBufferNonIndexed);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(GLfloat)),
                 vertices.data(),
                 GL_STREAM_DRAW);
    const GLint posAttrLoc = glGetAttribLocation(uniformColorProgram, "VERTEX");
    glVertexAttribPointer(posAttrLoc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(posAttrLoc);
    glDrawArrays(GL_POINTS, 0, 1);
}

void MapRenderer::RenderBillboardPointObjectBuffer(glm::vec3 position,
                                                   float pointSize,
                                                   ObjectBufferInfo info,
                                                   const glm::mat4 &matrix)
{
    glUseProgram(worldBuffersProgram);

    glPointSize(pointSize);

    glUniformMatrix4fv(glGetUniformLocation(worldBuffersProgram, "VIEW_MATRIX"), 1, GL_FALSE, glm::value_ptr(matrix));
    glUniformMatrix4fv(glGetUniformLocation(worldBuffersProgram, "WORLD_MATRIX"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(identity));

    glUniform1ui(glGetUniformLocation(worldBuffersProgram, "OBJECT_TYPE"), info.objectType);
    glUniform1ui(glGetUniformLocation(worldBuffersProgram, "OBJECT_INDEX"), info.objectIndex);
    glUniform1ui(glGetUniformLocation(worldBuffersProgram, "OBJECT_SUB_INDEX"), info.objectSubIndex);

    const std::vector<float> vertices = {position.x, position.y, position.z};

    GLHelper::BindBuffer(workBufferNonIndexed);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(GLfloat)),
                 vertices.data(),
                 GL_STREAM_DRAW);
    const GLint posAttrLoc = glGetAttribLocation(worldBuffersProgram, "VERTEX");
    glVertexAttribPointer(posAttrLoc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(posAttrLoc);
    glDrawArrays(GL_POINTS, 0, 1);
}

void MapRenderer::RenderBillboardSprite(const glm::vec3 position,
                                        const float pointSize,
                                        const std::string &texture,
                                        Color color,
                                        const glm::mat4 &matrix)
{
    glUseProgram(spriteProgram);

    glPointSize(pointSize);

    GLuint textureId = -1;
    const Error::ErrorCode e = SharedMgr::Get().textureCache.GetTextureGLuint(texture, textureId);
    if (e != Error::ErrorCode::OK)
    {
        textureId = SharedMgr::Get().textureCache.GetMissingTextureGLuint();
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glUniform1i(glGetUniformLocation(spriteProgram, "SPRITE"), 0);
    glUniform4fv(glGetUniformLocation(spriteProgram, "COLOR"), 1, color.GetDataPointer());
    glUniformMatrix4fv(glGetUniformLocation(spriteProgram, "VIEW_MATRIX"), 1, GL_FALSE, glm::value_ptr(matrix));
    glUniformMatrix4fv(glGetUniformLocation(spriteProgram, "WORLD_MATRIX"), 1, GL_FALSE, glm::value_ptr(identity));

    const std::vector<float> vertices = {position.x, position.y, position.z};

    GLHelper::BindBuffer(workBufferNonIndexed);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(GLfloat)),
                 vertices.data(),
                 GL_STREAM_DRAW);
    const GLint posAttrLoc = glGetAttribLocation(spriteProgram, "VERTEX");
    glVertexAttribPointer(posAttrLoc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(posAttrLoc);
    glDrawArrays(GL_POINTS, 0, 1);
}

void MapRenderer::RenderUnitVector(const glm::vec3 origin,
                                   const glm::vec3 eulerAngles,
                                   const Color color,
                                   const glm::mat4 &matrix,
                                   const float thickness,
                                   const float length)
{
    const glm::vec3 anglesRad = glm::radians(eulerAngles);
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), anglesRad.y, glm::vec3(0, 1, 0));
    rotationMatrix = glm::rotate(rotationMatrix, anglesRad.x, glm::vec3(1, 0, 0));
    rotationMatrix = glm::rotate(rotationMatrix, anglesRad.z, glm::vec3(0, 0, 1));

    const glm::vec4 unitVector(0.0f, 0.0f, -length, 0.0f);
    const glm::vec4 rotatedVector = rotationMatrix * unitVector;
    const glm::vec3 lenVector = glm::vec3(rotatedVector);

    RenderLine(origin, origin + lenVector, color, matrix, thickness);
}

void MapRenderer::RenderModel(std::string model,
                              const glm::mat4 &viewMatrix,
                              const glm::mat4 &worldMatrix,
                              const Color &modColor)
{
    if (!model.empty())
    {
        if (!modelBuffers.contains(model))
        {
            const std::string absolutePath = SharedMgr::Get().pathManager.GetAssetPath(model);
            if (absolutePath.empty())
            {
                model = "model/error.gmdl";
            } else
            {
                const ModelBuffer buf = LoadModel(absolutePath);
                modelBuffers[model] = buf;
            }
        }
        RenderModel(modelBuffers.at(model), viewMatrix, worldMatrix, modColor);
    }
}

void MapRenderer::RenderModelTextured(std::string model,
                                      const glm::mat4 &viewMatrix,
                                      const glm::mat4 &worldMatrix,
                                      const uint32_t skin,
                                      const Color &modColor,
                                      const Color &overlayColor)
{
    if (!model.empty())
    {
        if (!modelBuffers.contains(model))
        {
            const std::string absolutePath = SharedMgr::Get().pathManager.GetAssetPath(model);
            if (absolutePath.empty())
            {
                model = "model/error.gmdl";
            } else
            {
                const ModelBuffer buf = LoadModel(absolutePath);
                modelBuffers[model] = buf;
            }
        }
        RenderModelTextured(modelBuffers.at(model), viewMatrix, worldMatrix, skin, modColor, overlayColor);
    }
}

void MapRenderer::RenderModelObjectBuffer(std::string model,
                                          const glm::mat4 &viewMatrix,
                                          const glm::mat4 &worldMatrix,
                                          const ObjectBufferInfo info)
{
    if (!model.empty())
    {
        if (!modelBuffers.contains(model))
        {
            const std::string absolutePath = SharedMgr::Get().pathManager.GetAssetPath(model);
            if (absolutePath.empty())
            {
                model = "model/error.gmdl";
            } else
            {
                const ModelBuffer buf = LoadModel(absolutePath);
                modelBuffers[model] = buf;
            }
        }
        RenderModelObjectBufferInternal(modelBuffers.at(model), viewMatrix, worldMatrix, info);
    }
}

void MapRenderer::RenderBrushFace(const Brush::TriangulatedFace &face,
                                  const glm::mat4 &viewMatrix,
                                  const glm::mat4 &worldMatrix,
                                  const Color &overlayColor)
{
    glEnable(GL_CULL_FACE);

    glUseProgram(brushProgram);
    glBindVertexArray(workBuffer.vao);

    glBindBuffer(GL_ARRAY_BUFFER, workBuffer.vbo);
    glBufferData(GL_ARRAY_BUFFER, face.Vertices().size() * sizeof(MapVertex), face.Vertices().data(), GL_STREAM_DRAW);

    const GLint posAttrib = glGetAttribLocation(brushProgram, "VERTEX");
    const GLint uvAttrib = glGetAttribLocation(brushProgram, "VERTEX_UV");
    const GLint normalAttrib = glGetAttribLocation(brushProgram, "VERTEX_NORMAL");
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(MapVertex), nullptr);
    glVertexAttribPointer(uvAttrib,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(MapVertex),
                          reinterpret_cast<const void *>(offsetof(MapVertex, uv)));
    glVertexAttribPointer(normalAttrib,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(MapVertex),
                          reinterpret_cast<const void *>(offsetof(MapVertex, normal)));
    glEnableVertexAttribArray(posAttrib);
    glEnableVertexAttribArray(uvAttrib);
    glEnableVertexAttribArray(normalAttrib);

    glUniformMatrix4fv(glGetUniformLocation(brushProgram, "VIEW_MATRIX"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(glGetUniformLocation(brushProgram, "WORLD_MATRIX"), 1, GL_FALSE, glm::value_ptr(worldMatrix));
    glUniform4fv(glGetUniformLocation(brushProgram, "OVERLAY_COLOR"), 1, overlayColor.GetDataPointer());

    const std::string materialPath = SharedMgr::Get().pathManager.GetAssetPath(face.Material());
    LevelMaterialAsset material{};
    Error::ErrorCode error = material.LoadFromAsset(materialPath);
    if (error != Error::ErrorCode::OK)
    {
        Logger::Error("Creating material asset \"{}\" failed with error: {}", materialPath, error);
        return; // TODO: Use missing texture
    }

    GLuint texture = 0;
    const Error::ErrorCode code = SharedMgr::Get().textureCache.GetTextureGLuint(material.texture, texture);
    if (code != Error::ErrorCode::OK)
    {
        texture = SharedMgr::Get().textureCache.GetMissingTextureGLuint();
    }
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(brushProgram, "TEXTURE"), 0);
    glUniform1i(glGetUniformLocation(brushProgram, "SHADED"),
                material.shader == Material::MaterialShader::SHADER_SHADED);

    const GLuint ebo = workBuffer.ebo;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 face.Indices().size() * sizeof(uint32_t),
                 face.Indices().data(),
                 GL_STREAM_DRAW);
    glDrawElements(GL_TRIANGLES, face.Indices().size(), GL_UNSIGNED_INT, nullptr);

    glDisable(GL_CULL_FACE);
}

void MapRenderer::RenderBrushFaceObjectBuffer(const Brush::TriangulatedFace &face,
                                              const glm::mat4 &viewMatrix,
                                              const glm::mat4 &worldMatrix,
                                              const ObjectBufferInfo info)
{
    glEnable(GL_CULL_FACE);

    glUseProgram(worldBuffersProgram);
    glBindVertexArray(workBuffer.vao);

    glBindBuffer(GL_ARRAY_BUFFER, workBuffer.vbo);
    glBufferData(GL_ARRAY_BUFFER, face.Vertices().size() * sizeof(MapVertex), face.Vertices().data(), GL_STREAM_DRAW);

    const GLint posAttrib = glGetAttribLocation(worldBuffersProgram, "VERTEX");
    const GLint normalAttrib = glGetAttribLocation(worldBuffersProgram, "VERTEX_NORMAL");
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(MapVertex), nullptr);
    glVertexAttribPointer(normalAttrib,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(MapVertex),
                          reinterpret_cast<const void *>(offsetof(MapVertex, normal)));
    glEnableVertexAttribArray(posAttrib);
    glEnableVertexAttribArray(normalAttrib);

    glUniformMatrix4fv(glGetUniformLocation(worldBuffersProgram, "VIEW_MATRIX"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(glGetUniformLocation(worldBuffersProgram, "WORLD_MATRIX"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(worldMatrix));

    glUniform1ui(glGetUniformLocation(worldBuffersProgram, "OBJECT_TYPE"), info.objectType);
    glUniform1ui(glGetUniformLocation(worldBuffersProgram, "OBJECT_INDEX"), info.objectIndex);
    glUniform1ui(glGetUniformLocation(worldBuffersProgram, "OBJECT_SUB_INDEX"), info.objectSubIndex);

    const GLuint ebo = workBuffer.ebo;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 face.Indices().size() * sizeof(uint32_t),
                 face.Indices().data(),
                 GL_STREAM_DRAW);
    glDrawElements(GL_TRIANGLES, face.Indices().size(), GL_UNSIGNED_INT, nullptr);

    glDisable(GL_CULL_FACE);
}

const ModelAsset &MapRenderer::GetModel(std::string model)
{
    if (!model.empty())
    {
        if (!modelBuffers.contains(model))
        {
            const std::string absolutePath = SharedMgr::Get().pathManager.GetAssetPath(model);
            if (absolutePath.empty())
            {
                model = "model/error.gmdl";
            } else
            {
                const ModelBuffer buf = LoadModel(absolutePath);
                modelBuffers[model] = buf;
            }
        }
        return modelBuffers[model].model;
    }
    return modelBuffers["model/error.gmdl"].model;
}

MapRenderer::ModelBuffer MapRenderer::LoadModel(const std::string &path)
{
    Logger::Info("Loading model \"{}\"", path);
    ModelBuffer buf{};
    const Error::ErrorCode e = buf.model.LoadFromAsset(path);
    assert(e == Error::ErrorCode::OK); // TODO proper handling
    glGenVertexArrays(1, &buf.vao);
    glBindVertexArray(buf.vao);

    glGenBuffers(1, &buf.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, buf.vbo);

    DataWriter writer;
    buf.model.GetVertexBuffer(0, writer);
    std::vector<uint8_t> buffer;
    writer.CopyToVector(buffer);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(writer.GetBufferSize()), buffer.data(), GL_STATIC_DRAW);

    for (const ModelLod::ModelComponent &component: buf.model.GetLod(0).components)
    {
        GLuint ebo = 0;
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(component.indices.size() * sizeof(uint32_t)),
                     component.indices.data(),
                     GL_STATIC_DRAW);
        buf.ebos.push_back(ebo);
    }

    return buf;
}

void MapRenderer::RenderModel(ModelBuffer &buffer,
                              const glm::mat4 &viewMatrix,
                              const glm::mat4 &worldMatrix,
                              const Color &c)
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glUseProgram(uniformColorProgram);

    glLineWidth(1);

    glUniform4fv(glGetUniformLocation(uniformColorProgram, "COLOR"), 1, c.GetDataPointer());
    glUniformMatrix4fv(glGetUniformLocation(uniformColorProgram, "VIEW_MATRIX"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(glGetUniformLocation(uniformColorProgram, "WORLD_MATRIX"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(worldMatrix));

    glBindVertexArray(buffer.vao);
    glBindBuffer(GL_ARRAY_BUFFER, buffer.vbo);

    const GLint posAttrib = glGetAttribLocation(uniformColorProgram, "VERTEX");
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), nullptr);
    glEnableVertexAttribArray(posAttrib);

    for (size_t i = 0; i < buffer.ebos.size(); i++)
    {
        const GLuint ebo = buffer.ebos.at(i);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glDrawElements(GL_TRIANGLES,
                       static_cast<GLsizei>(buffer.model.GetLod(0).components.at(i).indices.size()),
                       GL_UNSIGNED_INT,
                       nullptr);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void MapRenderer::RenderModelTextured(ModelBuffer &buffer,
                                      const glm::mat4 &viewMatrix,
                                      const glm::mat4 &worldMatrix,
                                      const uint32_t skinIndex,
                                      const Color &modColor,
                                      const Color &overlayColor)
{
    const uint32_t lodIndex = 0;

    glEnable(GL_CULL_FACE);

    glUseProgram(shadedModelProgram);
    glBindVertexArray(buffer.vao);

    glBindBuffer(GL_ARRAY_BUFFER, buffer.vbo);
    const GLint posAttrib = glGetAttribLocation(shadedModelProgram, "VERTEX");
    const GLint uvAttrib = glGetAttribLocation(shadedModelProgram, "VERTEX_UV");
    const GLint colorAttrib = glGetAttribLocation(shadedModelProgram, "VERTEX_COLOR");
    const GLint normAttrib = glGetAttribLocation(shadedModelProgram, "VERTEX_NORMAL");
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

    glUniformMatrix4fv(glGetUniformLocation(shadedModelProgram, "VIEW_MATRIX"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(glGetUniformLocation(shadedModelProgram, "WORLD_MATRIX"), 1, GL_FALSE, glm::value_ptr(worldMatrix));
    glUniform4fv(glGetUniformLocation(shadedModelProgram, "MOD_COLOR"), 1, modColor.GetDataPointer());
    glUniform4fv(glGetUniformLocation(shadedModelProgram, "OVERLAY_COLOR"), 1, overlayColor.GetDataPointer());

    for (size_t i = 0; i < buffer.model.GetMaterialsPerSkin(); i++)
    {
        const size_t matIndex = buffer.model.GetSkin(skinIndex).at(i);
        Material &mat = buffer.model.GetMaterial(matIndex);
        glUniform4fv(glGetUniformLocation(shadedModelProgram, "MATERIAL_COLOR"), 1, mat.color.GetDataPointer());
        glUniform1i(glGetUniformLocation(shadedModelProgram, "SHADED"),
                    mat.shader == Material::MaterialShader::SHADER_SHADED);

        std::string textureName;
        if (mat.shader == Material::MaterialShader::SHADER_SKY)
        {
            textureName = MapEditor::map.skyTexture;
        } else
        {
            textureName = mat.texture;
        }

        GLuint texture = 0;
        const Error::ErrorCode code = SharedMgr::Get().textureCache.GetTextureGLuint(textureName, texture);
        if (code != Error::ErrorCode::OK)
        {
            texture = SharedMgr::Get().textureCache.GetMissingTextureGLuint();
        }
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(glGetUniformLocation(shadedModelProgram, "TEXTURE"), 0);

        const GLuint ebo = buffer.ebos.at(i);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glDrawElements(GL_TRIANGLES,
                       static_cast<GLsizei>(buffer.model.GetLod(lodIndex).components.at(i).indices.size()),
                       GL_UNSIGNED_INT,
                       nullptr);
    }

    glDisable(GL_CULL_FACE);
}

void MapRenderer::RenderModelObjectBufferInternal(ModelBuffer &buffer,
                                                  const glm::mat4 &viewMatrix,
                                                  const glm::mat4 &worldMatrix,
                                                  ObjectBufferInfo info)
{
    const uint32_t lodIndex = 0;

    glEnable(GL_CULL_FACE);

    glUseProgram(worldBuffersProgram);
    glBindVertexArray(buffer.vao);

    glBindBuffer(GL_ARRAY_BUFFER, buffer.vbo);
    const GLint posAttrib = glGetAttribLocation(worldBuffersProgram, "VERTEX");
    const GLint normAttrib = glGetAttribLocation(worldBuffersProgram, "VERTEX_NORMAL");
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), nullptr);
    glVertexAttribPointer(normAttrib,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          12 * sizeof(float),
                          reinterpret_cast<void *>(9 * sizeof(float)));
    glEnableVertexAttribArray(posAttrib);
    glEnableVertexAttribArray(normAttrib);

    glUniformMatrix4fv(glGetUniformLocation(worldBuffersProgram, "VIEW_MATRIX"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(glGetUniformLocation(worldBuffersProgram, "WORLD_MATRIX"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(worldMatrix));

    glUniform1ui(glGetUniformLocation(worldBuffersProgram, "OBJECT_TYPE"), info.objectType);
    glUniform1ui(glGetUniformLocation(worldBuffersProgram, "OBJECT_INDEX"), info.objectIndex);
    glUniform1ui(glGetUniformLocation(worldBuffersProgram, "OBJECT_SUB_INDEX"), info.objectSubIndex);

    for (size_t i = 0; i < buffer.model.GetMaterialsPerSkin(); i++)
    {
        const GLuint ebo = buffer.ebos.at(i);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glDrawElements(GL_TRIANGLES,
                       static_cast<GLsizei>(buffer.model.GetLod(lodIndex).components.at(i).indices.size()),
                       GL_UNSIGNED_INT,
                       nullptr);
    }

    glDisable(GL_CULL_FACE);
}

void MapRenderer::RenderWall(const glm::mat4 &viewMatrix,
                             const glm::mat4 &worldMatrix,
                             const Wall &wall,
                             const Color &overlayColor)
{
    const float bottom = wall.localCenter.y - (wall.size.y / 2.0f);
    const float top = wall.localCenter.y + (wall.size.y / 2.0f);
    const glm::vec2 startPoint = wall.zAxisOrientation ? glm::vec2(0, wall.localCenter.x - wall.size.x / 2.0f)
                                                       : glm::vec2(wall.localCenter.x - wall.size.x / 2.0f, 0);
    const glm::vec2 endPoint = wall.zAxisOrientation ? glm::vec2(0, wall.localCenter.x + wall.size.x / 2.0f)
                                                     : glm::vec2(wall.localCenter.x + wall.size.x / 2.0f, 0);

    glm::vec3 normal{};
    glm::vec3 backfaceNormal{};
    if (!wall.zAxisOrientation)
    {
        normal.z = 1;
        backfaceNormal.z = -1;
    } else
    {
        normal.x = -1;
        backfaceNormal.x = 1;
    }
    const glm::vec2 startUV = glm::vec2(wall.uvOffset.x, wall.uvOffset.y);
    const glm::vec2 endUV = glm::vec2((wall.uvScale.x / 16.0f) * wall.size.x + wall.uvOffset.x,
                                      (wall.uvScale.y / 16.0f) * wall.size.y + wall.uvOffset.y);

    const float vertices[8][12] = {
        // X Y Z U V A
        {
            startPoint.x,
            top,
            startPoint.y,
            startUV.x,
            startUV.y,
            1,
            1,
            1,
            1,
            normal.x,
            normal.y,
            normal.z,
        },
        {
            endPoint.x,
            top,
            endPoint.y,
            endUV.x,
            startUV.y,
            1,
            1,
            1,
            1,
            normal.x,
            normal.y,
            normal.z,
        },
        {
            endPoint.x,
            bottom,
            endPoint.y,
            endUV.x,
            endUV.y,
            1,
            1,
            1,
            1,
            normal.x,
            normal.y,
            normal.z,
        },
        {
            startPoint.x,
            bottom,
            startPoint.y,
            startUV.x,
            endUV.y,
            1,
            1,
            1,
            1,
            normal.x,
            normal.y,
            normal.z,
        },

        // backface
        {
            startPoint.x,
            top,
            startPoint.y,
            endUV.x,
            startUV.y,
            1,
            1,
            1,
            1,
            backfaceNormal.x,
            backfaceNormal.y,
            backfaceNormal.z,
        },
        {
            endPoint.x,
            top,
            endPoint.y,
            startUV.x,
            startUV.y,
            1,
            1,
            1,
            1,
            backfaceNormal.x,
            backfaceNormal.y,
            backfaceNormal.z,
        },
        {
            endPoint.x,
            bottom,
            endPoint.y,
            startUV.x,
            endUV.y,
            1,
            1,
            1,
            1,
            backfaceNormal.x,
            backfaceNormal.y,
            backfaceNormal.z,
        },
        {
            startPoint.x,
            bottom,
            startPoint.y,
            endUV.x,
            endUV.y,
            1,
            1,
            1,
            1,
            backfaceNormal.x,
            backfaceNormal.y,
            backfaceNormal.z,
        },
    };

    const uint32_t indices[] = {2, 1, 0, 3, 2, 0, 4, 5, 6, 4, 6, 7};

    glEnable(GL_CULL_FACE);

    glUseProgram(shadedModelProgram);
    glBindVertexArray(workBuffer.vao);

    glBindBuffer(GL_ARRAY_BUFFER, workBuffer.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);

    const GLint posAttrib = glGetAttribLocation(shadedModelProgram, "VERTEX");
    const GLint uvAttrib = glGetAttribLocation(shadedModelProgram, "VERTEX_UV");
    const GLint colorAttrib = glGetAttribLocation(shadedModelProgram, "VERTEX_COLOR");
    const GLint normAttrib = glGetAttribLocation(shadedModelProgram, "VERTEX_NORMAL");
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

    glUniformMatrix4fv(glGetUniformLocation(shadedModelProgram, "VIEW_MATRIX"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(glGetUniformLocation(shadedModelProgram, "WORLD_MATRIX"), 1, GL_FALSE, glm::value_ptr(worldMatrix));
    glUniform4fv(glGetUniformLocation(shadedModelProgram, "MOD_COLOR"), 1, wall.color.GetDataPointer());
    glUniform4fv(glGetUniformLocation(shadedModelProgram, "OVERLAY_COLOR"), 1, overlayColor.GetDataPointer());

    glUniform4fv(glGetUniformLocation(shadedModelProgram, "MATERIAL_COLOR"), 1, Color(-1).GetDataPointer());
    glUniform1i(glGetUniformLocation(shadedModelProgram, "SHADED"), !wall.unshaded);

    GLuint texture = 0;
    const Error::ErrorCode code = SharedMgr::Get().textureCache.GetTextureGLuint(wall.texture, texture);
    if (code != Error::ErrorCode::OK)
    {
        texture = SharedMgr::Get().textureCache.GetMissingTextureGLuint();
    }
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(shadedModelProgram, "TEXTURE"), 0);

    const GLuint ebo = workBuffer.ebo;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), &indices, GL_STREAM_DRAW);
    glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(uint32_t), GL_UNSIGNED_INT, nullptr);

    glDisable(GL_CULL_FACE);
}

void MapRenderer::RenderWallObjectBuffer(const glm::mat4 &viewMatrix,
                                         const glm::mat4 &worldMatrix,
                                         const Wall &wall,
                                         ObjectBufferInfo info)
{
    const float bottom = wall.localCenter.y - (wall.size.y / 2.0f);
    const float top = wall.localCenter.y + (wall.size.y / 2.0f);
    const glm::vec2 startPoint = wall.zAxisOrientation ? glm::vec2(0, wall.localCenter.x - wall.size.x / 2.0f)
                                                       : glm::vec2(wall.localCenter.x - wall.size.x / 2.0f, 0);
    const glm::vec2 endPoint = wall.zAxisOrientation ? glm::vec2(0, wall.localCenter.x + wall.size.x / 2.0f)
                                                     : glm::vec2(wall.localCenter.x + wall.size.x / 2.0f, 0);

    glm::vec3 normal{};
    glm::vec3 backfaceNormal{};
    if (!wall.zAxisOrientation)
    {
        normal.z = 1;
        backfaceNormal.z = -1;
    } else
    {
        normal.x = -1;
        backfaceNormal.x = 1;
    }

    const float vertices[8][6] = {
        // X Y Z U V A
        {
            startPoint.x,
            top,
            startPoint.y,
            normal.x,
            normal.y,
            normal.z,
        },
        {
            endPoint.x,
            top,
            endPoint.y,
            normal.x,
            normal.y,
            normal.z,
        },
        {
            endPoint.x,
            bottom,
            endPoint.y,
            normal.x,
            normal.y,
            normal.z,
        },
        {
            startPoint.x,
            bottom,
            startPoint.y,
            normal.x,
            normal.y,
            normal.z,
        },

        // backface
        {
            startPoint.x,
            top,
            startPoint.y,
            backfaceNormal.x,
            backfaceNormal.y,
            backfaceNormal.z,
        },
        {
            endPoint.x,
            top,
            endPoint.y,
            backfaceNormal.x,
            backfaceNormal.y,
            backfaceNormal.z,
        },
        {
            endPoint.x,
            bottom,
            endPoint.y,
            backfaceNormal.x,
            backfaceNormal.y,
            backfaceNormal.z,
        },
        {
            startPoint.x,
            bottom,
            startPoint.y,
            backfaceNormal.x,
            backfaceNormal.y,
            backfaceNormal.z,
        },
    };

    const uint32_t indices[] = {2, 1, 0, 3, 2, 0, 4, 5, 6, 4, 6, 7};

    glEnable(GL_CULL_FACE);

    glUseProgram(worldBuffersProgram);
    glBindVertexArray(workBuffer.vao);

    glBindBuffer(GL_ARRAY_BUFFER, workBuffer.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);

    const GLint posAttrib = glGetAttribLocation(worldBuffersProgram, "VERTEX");
    const GLint normAttrib = glGetAttribLocation(worldBuffersProgram, "VERTEX_NORMAL");
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glVertexAttribPointer(normAttrib,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          6 * sizeof(float),
                          reinterpret_cast<void *>(3 * sizeof(float)));
    glEnableVertexAttribArray(posAttrib);
    glEnableVertexAttribArray(normAttrib);

    glUniformMatrix4fv(glGetUniformLocation(worldBuffersProgram, "VIEW_MATRIX"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(glGetUniformLocation(worldBuffersProgram, "WORLD_MATRIX"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(worldMatrix));

    glUniform1ui(glGetUniformLocation(worldBuffersProgram, "OBJECT_TYPE"), info.objectType);
    glUniform1ui(glGetUniformLocation(worldBuffersProgram, "OBJECT_INDEX"), info.objectIndex);
    glUniform1ui(glGetUniformLocation(worldBuffersProgram, "OBJECT_SUB_INDEX"), info.objectSubIndex);

    const GLuint ebo = workBuffer.ebo;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), &indices, GL_STREAM_DRAW);
    glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(uint32_t), GL_UNSIGNED_INT, nullptr);

    glDisable(GL_CULL_FACE);
}
