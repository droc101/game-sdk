//
// Created by droc101 on 9/5/25.
//

#include "MapRenderer.h"
#include <array>
#include <cassert>
#include <cmath>
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
    // TODO this should use one big VAO and VBO for all models
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

void MapRenderer::RenderViewportGrid(const Viewport &vp)
{
    glLineWidth(1);

    glm::mat4 view = vp.GetMatrix();

    glDisable(GL_DEPTH_TEST);

    if (MapEditor::drawGrid && !vp.Is3D())
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
    glDepthFunc(GL_GEQUAL);

    glUseProgram(vertexColorProgram);
    glUniformMatrix4fv(glGetUniformLocation(vertexColorProgram, "VIEW_MATRIX"), 1, GL_FALSE, glm::value_ptr(view));
    const GLint posAttrib = glGetAttribLocation(vertexColorProgram, "VERTEX");
    const GLint colorAttrib = glGetAttribLocation(vertexColorProgram, "VERTEX_COLOR");
    if (MapEditor::drawAxisHelper && !vp.Is3D())
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
    if (MapEditor::drawWorldBorder && !vp.Is3D())
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
    glEnable(GL_CULL_FACE);
}

void MapRenderer::CmdPoint(const glm::vec3 position, const float pointSize, Color color)
{
    pointCommands.emplace_back(position, pointSize, color);
}

void MapRenderer::CmdLine(const glm::vec3 start, const glm::vec3 end, Color color, const float thickness)
{
    lineCommands.emplace_back(start, end, thickness, color);
}

void MapRenderer::CmdSprite(const glm::vec3 position, const float pointSize, const std::string &texture, Color color)
{
    spriteCommands.emplace_back(position, pointSize, texture, color);
}

void MapRenderer::CmdUnitVector(const glm::vec3 origin,
                                const glm::vec3 eulerAngles,
                                const Color color,
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

    CmdLine(origin, origin + lenVector, color, thickness);
}

void MapRenderer::CmdModel(std::string model, const glm::mat4 &worldMatrix, const Color &modColor)
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
        modelCommands.emplace_back(modelBuffers.at(model), worldMatrix, modColor);
    }
}

void MapRenderer::CmdTexturedModel(std::string model,
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
        texturedModelCommands.emplace_back(modelBuffers.at(model), worldMatrix, skin, 0, modColor, overlayColor);
    }
}

void MapRenderer::CmdBrushFace(const Brush::TriangulatedFace &face,
                               const glm::mat4 &worldMatrix,
                               const Color &overlayColor)
{
    brushFaceCommands.emplace_back(face, worldMatrix, overlayColor);
}

void MapRenderer::CmdWall(const glm::mat4 &worldMatrix, const Wall &wall, const Color &overlayColor)
{
    const WallCommand bufferEntry = {
        .wall = wall,
        .overlayColor = overlayColor,
        .worldMatrix = worldMatrix,
    };
    wallCommands.push_back(bufferEntry);
}

void MapRenderer::ExecuteCommands(const glm::mat4 &viewMatrix)
{
    // -------------------------------- Lines/Points/Models --------------------------------
    if (!lineCommands.empty() || !pointCommands.empty() || !modelCommands.empty())
    {
        glUseProgram(uniformColorProgram);
        glUniformMatrix4fv(glGetUniformLocation(uniformColorProgram, "VIEW_MATRIX"),
                           1,
                           GL_FALSE,
                           glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(glGetUniformLocation(uniformColorProgram, "WORLD_MATRIX"),
                           1,
                           GL_FALSE,
                           glm::value_ptr(identity));
        GLHelper::BindBuffer(workBufferNonIndexed);
        const GLint uniformColorProgramPosAttrLoc = glGetAttribLocation(uniformColorProgram, "VERTEX");
        glVertexAttribPointer(uniformColorProgramPosAttrLoc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);
        glEnableVertexAttribArray(uniformColorProgramPosAttrLoc);

        // -------------------------------- Lines --------------------------------
        if (!lineCommands.empty())
        {
            std::vector<glm::vec3> lineVertices;
            lineVertices.reserve(lineCommands.size() * 2);
            for (const LineCommand &line: lineCommands)
            {
                lineVertices.push_back(line.start);
                lineVertices.push_back(line.end);
            }
            float lineWidth = NAN;
            glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(glm::vec3), lineVertices.data(), GL_STREAM_DRAW);
            for (uint32_t i = 0; i < lineCommands.size(); i++)
            {
                const LineCommand &line = lineCommands.at(i);
                if (lineWidth != line.width)
                {
                    glLineWidth(line.width);
                    lineWidth = line.width;
                }
                glUniform4fv(glGetUniformLocation(uniformColorProgram, "COLOR"), 1, line.color.GetDataPointer());
                glDrawArrays(GL_LINES, static_cast<GLint>(i * 2), 2);
            }
            lineCommands.clear();
        }

        // -------------------------------- Points --------------------------------
        if (!pointCommands.empty())
        {
            glVertexAttribPointer(uniformColorProgramPosAttrLoc, 3, GL_FLOAT, GL_FALSE, sizeof(PointCommand), nullptr);
            glEnableVertexAttribArray(uniformColorProgramPosAttrLoc);
            glBufferData(GL_ARRAY_BUFFER,
                         pointCommands.size() * sizeof(PointCommand),
                         pointCommands.data(),
                         GL_STREAM_DRAW);
            for (uint32_t i = 0; i < pointCommands.size(); i++)
            {
                const PointCommand &point = pointCommands.at(i);
                glPointSize(point.size);
                glUniform4fv(glGetUniformLocation(uniformColorProgram, "COLOR"), 1, point.color.GetDataPointer());
                glDrawArrays(GL_POINTS, static_cast<GLint>(i), 1);
            }
            pointCommands.clear();
        }

        // -------------------------------- Models --------------------------------
        if (!modelCommands.empty())
        {
            glDisable(GL_CULL_FACE);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(1);

            for (const ModelCommand &command: modelCommands)
            {
                glUniform4fv(glGetUniformLocation(uniformColorProgram, "COLOR"), 1, command.color.GetDataPointer());

                glUniformMatrix4fv(glGetUniformLocation(uniformColorProgram, "WORLD_MATRIX"),
                                   1,
                                   GL_FALSE,
                                   glm::value_ptr(command.worldMatrix));

                glBindVertexArray(command.buffer.vao);
                glBindBuffer(GL_ARRAY_BUFFER, command.buffer.vbo);

                const GLint posAttrib = glGetAttribLocation(uniformColorProgram, "VERTEX");
                glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), nullptr);
                glEnableVertexAttribArray(posAttrib);

                for (size_t i = 0; i < command.buffer.ebos.size(); i++)
                {
                    const GLuint ebo = command.buffer.ebos.at(i);
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
                    glDrawElements(GL_TRIANGLES,
                                   static_cast<GLsizei>(command.buffer.model.GetLod(0).components.at(i).indices.size()),
                                   GL_UNSIGNED_INT,
                                   nullptr);
                }
            }
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_CULL_FACE);
            modelCommands.clear();
        }
    }

    // -------------------------------- Walls/Textured Models Setup --------------------------------
    if (!wallCommands.empty() || !texturedModelCommands.empty())
    {
        glUseProgram(shadedModelProgram);
        GLHelper::BindIndexedBuffer(workBuffer);
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

        glUniformMatrix4fv(glGetUniformLocation(shadedModelProgram, "VIEW_MATRIX"),
                           1,
                           GL_FALSE,
                           glm::value_ptr(viewMatrix));

        // -------------------------------- Walls --------------------------------
        if (!wallCommands.empty())
        {
            std::vector<float> wallVerts{};
            wallVerts.reserve(wallCommands.size() * 8 * 12);
            for (const WallCommand &bufferEntry: wallCommands)
            {
                const float bottom = bufferEntry.wall.localCenter.y - (bufferEntry.wall.size.y / 2.0f);
                const float top = bufferEntry.wall.localCenter.y + (bufferEntry.wall.size.y / 2.0f);
                glm::vec2 startPoint{};
                glm::vec2 endPoint{};
                glm::vec3 normal{};
                glm::vec3 backfaceNormal{};

                if (bufferEntry.wall.zAxisOrientation)
                {
                    startPoint = glm::vec2(0, bufferEntry.wall.localCenter.x - bufferEntry.wall.size.x / 2.0f);
                    endPoint = glm::vec2(0, bufferEntry.wall.localCenter.x + bufferEntry.wall.size.x / 2.0f);
                    normal.x = -1;
                    backfaceNormal.x = 1;
                } else
                {
                    startPoint = glm::vec2(bufferEntry.wall.localCenter.x - bufferEntry.wall.size.x / 2.0f, 0);
                    endPoint = glm::vec2(bufferEntry.wall.localCenter.x + bufferEntry.wall.size.x / 2.0f, 0);
                    normal.z = 1;
                    backfaceNormal.z = -1;
                }

                const glm::vec2 startUV = glm::vec2(bufferEntry.wall.uvOffset.x, bufferEntry.wall.uvOffset.y);
                const glm::vec2 endUV = glm::vec2((bufferEntry.wall.uvScale.x / 16.0f) * bufferEntry.wall.size.x +
                                                          bufferEntry.wall.uvOffset.x,
                                                  (bufferEntry.wall.uvScale.y / 16.0f) * bufferEntry.wall.size.y +
                                                          bufferEntry.wall.uvOffset.y);

                // clang-format off
                const std::array<float, 8 * 12> verts = {
                    // X Y Z U V A
                    startPoint.x, top, startPoint.y, startUV.x, startUV.y, 1, 1, 1, 1, normal.x, normal.y, normal.z,
                    // ----
                    endPoint.x, top, endPoint.y, endUV.x, startUV.y, 1, 1, 1, 1, normal.x, normal.y, normal.z,
                    // ----
                    endPoint.x, bottom, endPoint.y, endUV.x, endUV.y, 1, 1, 1, 1, normal.x, normal.y, normal.z,
                    // ----
                    startPoint.x, bottom, startPoint.y, startUV.x, endUV.y, 1, 1, 1, 1, normal.x, normal.y, normal.z,
                    // ---- backface
                    startPoint.x, top, startPoint.y, endUV.x, startUV.y, 1, 1, 1, 1, backfaceNormal.x, backfaceNormal.y, backfaceNormal.z,
                    // ----
                    endPoint.x, top, endPoint.y, startUV.x, startUV.y, 1, 1, 1, 1, backfaceNormal.x, backfaceNormal.y, backfaceNormal.z,
                    // ----
                    endPoint.x, bottom, endPoint.y, startUV.x, endUV.y, 1, 1, 1, 1, backfaceNormal.x, backfaceNormal.y, backfaceNormal.z,
                    // ----
                    startPoint.x, bottom, startPoint.y, endUV.x, endUV.y, 1, 1, 1, 1, backfaceNormal.x, backfaceNormal.y, backfaceNormal.z,
                };
                // clang-format on

                wallVerts.insert(wallVerts.end(), verts.begin(), verts.end());
            }

            glBufferData(GL_ARRAY_BUFFER, wallVerts.size() * sizeof(float), wallVerts.data(), GL_STREAM_DRAW);
            glUniform4fv(glGetUniformLocation(shadedModelProgram, "MATERIAL_COLOR"), 1, Color(-1).GetDataPointer());

            for (size_t i = 0; i < wallCommands.size(); i++)
            {
                const WallCommand &bufferEntry = wallCommands.at(i);
                std::array<uint32_t, 12> indices = {2, 1, 0, 3, 2, 0, 4, 5, 6, 4, 6, 7};
                for (unsigned int &index: indices)
                {
                    index += i * 8;
                }
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), &indices, GL_STREAM_DRAW);


                glUniformMatrix4fv(glGetUniformLocation(shadedModelProgram, "WORLD_MATRIX"),
                                   1,
                                   GL_FALSE,
                                   glm::value_ptr(bufferEntry.worldMatrix));
                glUniform4fv(glGetUniformLocation(shadedModelProgram, "MOD_COLOR"),
                             1,
                             bufferEntry.wall.color.GetDataPointer());
                glUniform4fv(glGetUniformLocation(shadedModelProgram, "OVERLAY_COLOR"),
                             1,
                             bufferEntry.overlayColor.GetDataPointer());
                glUniform1i(glGetUniformLocation(shadedModelProgram, "SHADED"), !bufferEntry.wall.unshaded);

                GLuint texture = 0;
                const Error::ErrorCode code = SharedMgr::Get().textureCache.GetTextureGLuint(bufferEntry.wall.texture,
                                                                                             texture);
                if (code != Error::ErrorCode::OK)
                {
                    texture = SharedMgr::Get().textureCache.GetMissingTextureGLuint();
                }
                glBindTexture(GL_TEXTURE_2D, texture);
                glUniform1i(glGetUniformLocation(shadedModelProgram, "TEXTURE"), 0);

                glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);
            }
            wallCommands.clear();
        }

        // -------------------------------- Textured Models --------------------------------
        if (!texturedModelCommands.empty())
        {
            for (const TexturedModelCommand &command: texturedModelCommands)
            {
                glBindVertexArray(command.buffer.vao);
                glBindBuffer(GL_ARRAY_BUFFER, command.buffer.vbo);
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

                glUniformMatrix4fv(glGetUniformLocation(shadedModelProgram, "WORLD_MATRIX"),
                                   1,
                                   GL_FALSE,
                                   glm::value_ptr(command.worldMatrix));
                glUniform4fv(glGetUniformLocation(shadedModelProgram, "MOD_COLOR"),
                             1,
                             command.modColor.GetDataPointer());
                glUniform4fv(glGetUniformLocation(shadedModelProgram, "OVERLAY_COLOR"),
                             1,
                             command.overlayColor.GetDataPointer());

                for (size_t i = 0; i < command.buffer.model.GetMaterialsPerSkin(); i++)
                {
                    const size_t matIndex = command.buffer.model.GetSkin(command.skinIndex).at(i);
                    Material &mat = command.buffer.model.GetMaterial(matIndex);
                    glUniform4fv(glGetUniformLocation(shadedModelProgram, "MATERIAL_COLOR"),
                                 1,
                                 mat.color.GetDataPointer());
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

                    const GLuint ebo = command.buffer.ebos.at(i);
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
                    glDrawElements(GL_TRIANGLES,
                                   static_cast<GLsizei>(command.buffer.model.GetLod(command.lodIndex)
                                                                .components.at(i)
                                                                .indices.size()),
                                   GL_UNSIGNED_INT,
                                   nullptr);
                }
            }
            texturedModelCommands.clear();
        }
    }

    // -------------------------------- Sprites --------------------------------
    if (!spriteCommands.empty())
    {
        glUseProgram(spriteProgram);
        GLHelper::BindBuffer(workBufferNonIndexed);
        const GLint posAttrLoc = glGetAttribLocation(spriteProgram, "VERTEX");
        glVertexAttribPointer(posAttrLoc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);
        glEnableVertexAttribArray(posAttrLoc);

        for (const SpriteCommand &command: spriteCommands)
        {
            glPointSize(command.pointSize);

            GLuint textureId = -1;
            const Error::ErrorCode e = SharedMgr::Get().textureCache.GetTextureGLuint(command.texture, textureId);
            if (e != Error::ErrorCode::OK)
            {
                textureId = SharedMgr::Get().textureCache.GetMissingTextureGLuint();
            }

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureId);
            glUniform1i(glGetUniformLocation(spriteProgram, "SPRITE"), 0);
            glUniform4fv(glGetUniformLocation(spriteProgram, "COLOR"), 1, command.color.GetDataPointer());
            glUniformMatrix4fv(glGetUniformLocation(spriteProgram, "VIEW_MATRIX"),
                               1,
                               GL_FALSE,
                               glm::value_ptr(viewMatrix));
            glUniformMatrix4fv(glGetUniformLocation(spriteProgram, "WORLD_MATRIX"),
                               1,
                               GL_FALSE,
                               glm::value_ptr(identity));

            const std::array vertices = {command.position.x, command.position.y, command.position.z};
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STREAM_DRAW);

            glDrawArrays(GL_POINTS, 0, 1);
        }
        spriteCommands.clear();
    }

    // -------------------------------- Brushes --------------------------------
    if (!brushFaceCommands.empty())
    {
        glUseProgram(brushProgram);
        GLHelper::BindIndexedBuffer(workBuffer);
        const GLint brushPosAttrib = glGetAttribLocation(brushProgram, "VERTEX");
        const GLint brushUvAttrib = glGetAttribLocation(brushProgram, "VERTEX_UV");
        const GLint brushNormalAttrib = glGetAttribLocation(brushProgram, "VERTEX_NORMAL");
        glVertexAttribPointer(brushPosAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(MapVertex), nullptr);
        glVertexAttribPointer(brushUvAttrib,
                              2,
                              GL_FLOAT,
                              GL_FALSE,
                              sizeof(MapVertex),
                              reinterpret_cast<const void *>(offsetof(MapVertex, uv)));
        glVertexAttribPointer(brushNormalAttrib,
                              3,
                              GL_FLOAT,
                              GL_FALSE,
                              sizeof(MapVertex),
                              reinterpret_cast<const void *>(offsetof(MapVertex, normal)));
        glEnableVertexAttribArray(brushPosAttrib);
        glEnableVertexAttribArray(brushUvAttrib);
        glEnableVertexAttribArray(brushNormalAttrib);

        glUniformMatrix4fv(glGetUniformLocation(brushProgram, "VIEW_MATRIX"), 1, GL_FALSE, glm::value_ptr(viewMatrix));

        for (const BrushFaceCommand &command: brushFaceCommands)
        {
            GLuint texture = 0;
            const std::string materialPath = SharedMgr::Get().pathManager.GetAssetPath(command.face.material);
            LevelMaterialAsset material{};
            Error::ErrorCode error = material.LoadFromAsset(materialPath);
            if (error != Error::ErrorCode::OK)
            {
                texture = SharedMgr::Get().textureCache.GetMissingTextureGLuint();
            } else
            {
                if (!MapEditor::showInvisibleFaces && material.HasFlags(LevelMaterialAsset::FLAG_INVISIBLE))
                {
                    continue;
                }
                const Error::ErrorCode code = SharedMgr::Get().textureCache.GetTextureGLuint(material.texture, texture);
                if (code != Error::ErrorCode::OK)
                {
                    texture = SharedMgr::Get().textureCache.GetMissingTextureGLuint();
                }
            }

            glBindTexture(GL_TEXTURE_2D, texture);

            glUniformMatrix4fv(glGetUniformLocation(brushProgram, "WORLD_MATRIX"),
                               1,
                               GL_FALSE,
                               glm::value_ptr(command.worldMatrix));
            glUniform4fv(glGetUniformLocation(brushProgram, "OVERLAY_COLOR"), 1, command.overlayColor.GetDataPointer());
            glUniform1i(glGetUniformLocation(brushProgram, "TEXTURE"), 0);
            glUniform1i(glGetUniformLocation(brushProgram, "SHADED"),
                        material.shader == Material::MaterialShader::SHADER_SHADED);

            glBufferData(GL_ARRAY_BUFFER,
                         command.face.vertices.size() * sizeof(MapVertex),
                         command.face.vertices.data(),
                         GL_STREAM_DRAW);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         command.face.indices.size() * sizeof(uint32_t),
                         command.face.indices.data(),
                         GL_STREAM_DRAW);
            glDrawElements(GL_TRIANGLES, command.face.indices.size(), GL_UNSIGNED_INT, nullptr);
        }
        brushFaceCommands.clear();
    }
}

void MapRenderer::ExecuteObjectBufferCommands(const glm::mat4 &viewMatrix)
{
    if (objectBufferPointCommands.empty() &&
        objectBufferWallCommands.empty() &&
        objectBufferBrushFaceCommands.empty() &&
        objectBufferModelCommands.empty())
    {
        return;
    }

    glUseProgram(worldBuffersProgram);
    glUniformMatrix4fv(glGetUniformLocation(worldBuffersProgram, "VIEW_MATRIX"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(viewMatrix));

    const GLint posAttrib = glGetAttribLocation(worldBuffersProgram, "VERTEX");
    const GLint normAttrib = glGetAttribLocation(worldBuffersProgram, "VERTEX_NORMAL");
    GLHelper::BindIndexedBuffer(workBuffer);

    if (!objectBufferPointCommands.empty())
    {
        glUniformMatrix4fv(glGetUniformLocation(worldBuffersProgram, "WORLD_MATRIX"),
                           1,
                           GL_FALSE,
                           glm::value_ptr(identity));
        glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(ObjectBufferPointCommand), nullptr);
        glEnableVertexAttribArray(posAttrib);
        glBufferData(GL_ARRAY_BUFFER,
                     objectBufferPointCommands.size() * sizeof(PointCommand),
                     objectBufferPointCommands.data(),
                     GL_STREAM_DRAW);
        for (uint32_t i = 0; i < objectBufferPointCommands.size(); i++)
        {
            const ObjectBufferPointCommand &point = objectBufferPointCommands.at(i);
            glPointSize(point.pointSize);
            glUniform1ui(glGetUniformLocation(worldBuffersProgram, "OBJECT_TYPE"), point.info.objectType);
            glUniform1ui(glGetUniformLocation(worldBuffersProgram, "OBJECT_INDEX"), point.info.objectIndex);
            glUniform1ui(glGetUniformLocation(worldBuffersProgram, "OBJECT_SUB_INDEX"), point.info.objectSubIndex);
            glDrawArrays(GL_POINTS, static_cast<GLint>(i), 1);
        }
        objectBufferPointCommands.clear();
    }

    if (!objectBufferWallCommands.empty())
    {
        glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
        glVertexAttribPointer(normAttrib,
                              3,
                              GL_FLOAT,
                              GL_FALSE,
                              6 * sizeof(float),
                              reinterpret_cast<void *>(3 * sizeof(float)));
        glEnableVertexAttribArray(posAttrib);
        glEnableVertexAttribArray(normAttrib);

        std::vector<float> wallVerts{};
        wallVerts.reserve(objectBufferWallCommands.size() * 8 * 6);
        for (const ObjectBufferWallCommand &bufferEntry: objectBufferWallCommands)
        {
            const float bottom = bufferEntry.wall.localCenter.y - (bufferEntry.wall.size.y / 2.0f);
            const float top = bufferEntry.wall.localCenter.y + (bufferEntry.wall.size.y / 2.0f);
            glm::vec2 startPoint{};
            glm::vec2 endPoint{};
            glm::vec3 normal{};
            glm::vec3 backfaceNormal{};

            if (bufferEntry.wall.zAxisOrientation)
            {
                startPoint = glm::vec2(0, bufferEntry.wall.localCenter.x - bufferEntry.wall.size.x / 2.0f);
                endPoint = glm::vec2(0, bufferEntry.wall.localCenter.x + bufferEntry.wall.size.x / 2.0f);
                normal.x = -1;
                backfaceNormal.x = 1;
            } else
            {
                startPoint = glm::vec2(bufferEntry.wall.localCenter.x - bufferEntry.wall.size.x / 2.0f, 0);
                endPoint = glm::vec2(bufferEntry.wall.localCenter.x + bufferEntry.wall.size.x / 2.0f, 0);
                normal.z = 1;
                backfaceNormal.z = -1;
            }

            // clang-format off
            const std::array<float, 8 * 6> verts = {
                // X Y Z U V A
                startPoint.x, top, startPoint.y, normal.x, normal.y, normal.z,
                // ----
                endPoint.x, top, endPoint.y, normal.x, normal.y, normal.z,
                // ----
                endPoint.x, bottom, endPoint.y, normal.x, normal.y, normal.z,
                // ----
                startPoint.x, bottom, startPoint.y, normal.x, normal.y, normal.z,
                // ---- backface
                startPoint.x, top, startPoint.y, backfaceNormal.x, backfaceNormal.y, backfaceNormal.z,
                // ----
                endPoint.x, top, endPoint.y, backfaceNormal.x, backfaceNormal.y, backfaceNormal.z,
                // ----
                endPoint.x, bottom, endPoint.y, backfaceNormal.x, backfaceNormal.y, backfaceNormal.z,
                // ----
                startPoint.x, bottom, startPoint.y, backfaceNormal.x, backfaceNormal.y, backfaceNormal.z,
            };
            // clang-format on

            wallVerts.insert(wallVerts.end(), verts.begin(), verts.end());
        }
        glBufferData(GL_ARRAY_BUFFER, wallVerts.size() * sizeof(float), wallVerts.data(), GL_STREAM_DRAW);

        for (size_t i = 0; i < objectBufferWallCommands.size(); i++)
        {
            const ObjectBufferWallCommand &bufferEntry = objectBufferWallCommands.at(i);
            std::array<uint32_t, 12> indices = {2, 1, 0, 3, 2, 0, 4, 5, 6, 4, 6, 7};
            for (unsigned int &index: indices)
            {
                index += i * 8;
            }
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), &indices, GL_STREAM_DRAW);

            glUniformMatrix4fv(glGetUniformLocation(worldBuffersProgram, "WORLD_MATRIX"),
                               1,
                               GL_FALSE,
                               glm::value_ptr(bufferEntry.worldMatrix));

            glUniform1ui(glGetUniformLocation(worldBuffersProgram, "OBJECT_TYPE"), bufferEntry.info.objectType);
            glUniform1ui(glGetUniformLocation(worldBuffersProgram, "OBJECT_INDEX"), bufferEntry.info.objectIndex);
            glUniform1ui(glGetUniformLocation(worldBuffersProgram, "OBJECT_SUB_INDEX"),
                         bufferEntry.info.objectSubIndex);

            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);
        }
        objectBufferWallCommands.clear();
    }

    if (!objectBufferBrushFaceCommands.empty())
    {
        glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(MapVertex), nullptr);
        glVertexAttribPointer(normAttrib,
                              3,
                              GL_FLOAT,
                              GL_FALSE,
                              sizeof(MapVertex),
                              reinterpret_cast<const void *>(offsetof(MapVertex, normal)));
        glEnableVertexAttribArray(posAttrib);
        glEnableVertexAttribArray(normAttrib);
        for (const ObjectBufferBrushFaceCommand &command: objectBufferBrushFaceCommands)
        {
            glBufferData(GL_ARRAY_BUFFER,
                         command.face.vertices.size() * sizeof(MapVertex),
                         command.face.vertices.data(),
                         GL_STREAM_DRAW);

            glUniformMatrix4fv(glGetUniformLocation(worldBuffersProgram, "VIEW_MATRIX"),
                               1,
                               GL_FALSE,
                               glm::value_ptr(viewMatrix));
            glUniformMatrix4fv(glGetUniformLocation(worldBuffersProgram, "WORLD_MATRIX"),
                               1,
                               GL_FALSE,
                               glm::value_ptr(command.worldMatrix));

            glUniform1ui(glGetUniformLocation(worldBuffersProgram, "OBJECT_TYPE"), command.info.objectType);
            glUniform1ui(glGetUniformLocation(worldBuffersProgram, "OBJECT_INDEX"), command.info.objectIndex);
            glUniform1ui(glGetUniformLocation(worldBuffersProgram, "OBJECT_SUB_INDEX"), command.info.objectSubIndex);

            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         command.face.indices.size() * sizeof(uint32_t),
                         command.face.indices.data(),
                         GL_STREAM_DRAW);
            glDrawElements(GL_TRIANGLES, command.face.indices.size(), GL_UNSIGNED_INT, nullptr);
        }
        objectBufferBrushFaceCommands.clear();
    }

    if (!objectBufferModelCommands.empty())
    {
        for (const ObjectBufferModelCommand &command: objectBufferModelCommands)
        {
            glBindVertexArray(command.buffer.vao);

            glBindBuffer(GL_ARRAY_BUFFER, command.buffer.vbo);

            glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), nullptr);
            glVertexAttribPointer(normAttrib,
                                  3,
                                  GL_FLOAT,
                                  GL_FALSE,
                                  12 * sizeof(float),
                                  reinterpret_cast<void *>(9 * sizeof(float)));
            glEnableVertexAttribArray(posAttrib);
            glEnableVertexAttribArray(normAttrib);

            glUniformMatrix4fv(glGetUniformLocation(worldBuffersProgram, "WORLD_MATRIX"),
                               1,
                               GL_FALSE,
                               glm::value_ptr(command.worldMatrix));

            glUniform1ui(glGetUniformLocation(worldBuffersProgram, "OBJECT_TYPE"), command.info.objectType);
            glUniform1ui(glGetUniformLocation(worldBuffersProgram, "OBJECT_INDEX"), command.info.objectIndex);
            glUniform1ui(glGetUniformLocation(worldBuffersProgram, "OBJECT_SUB_INDEX"), command.info.objectSubIndex);

            for (size_t i = 0; i < command.buffer.model.GetMaterialsPerSkin(); i++)
            {
                const GLuint ebo = command.buffer.ebos.at(i);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
                glDrawElements(GL_TRIANGLES,
                               static_cast<GLsizei>(command.buffer.model.GetLod(command.lodIndex)
                                                            .components.at(i)
                                                            .indices.size()),
                               GL_UNSIGNED_INT,
                               nullptr);
            }
        }
        objectBufferModelCommands.clear();
    }
}

void MapRenderer::CmdObjectBufferPoint(const glm::vec3 position,
                                                   const float pointSize,
                                                   const ObjectBufferInfo info)
{
    objectBufferPointCommands.emplace_back(position, pointSize, info);
}


void MapRenderer::CmdObjectBufferModel(std::string model, const glm::mat4 &worldMatrix, const ObjectBufferInfo info)
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
        objectBufferModelCommands.emplace_back(modelBuffers.at(model), worldMatrix, info, 0);
    }
}


void MapRenderer::CmdObjectBufferBrushFace(const Brush::TriangulatedFace &face,
                                              const glm::mat4 &worldMatrix,
                                              const ObjectBufferInfo info)
{
    objectBufferBrushFaceCommands.emplace_back(face, worldMatrix, info);
}

void MapRenderer::CmdObjectBufferWall(const glm::mat4 &worldMatrix, const Wall &wall, const ObjectBufferInfo info)
{
    objectBufferWallCommands.emplace_back(worldMatrix, wall, info);
}
