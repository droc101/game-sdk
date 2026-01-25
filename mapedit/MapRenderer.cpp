//
// Created by droc101 on 9/5/25.
//

#include "MapRenderer.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <glm/ext.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <libassets/asset/ModelAsset.h>
#include <libassets/type/Actor.h>
#include <libassets/type/ActorDefinition.h>
#include <libassets/type/Color.h>
#include <libassets/type/ModelLod.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <ranges>
#include <string>
#include <vector>
#include "GLHelper.h"
#include "imgui.h"
#include "MapEditor.h"
#include "OpenGLImGuiTextureAssetCache.h"
#include "Options.h"
#include "SharedMgr.h"
#include "Viewport.h"

bool MapRenderer::Init()
{
    if (!GLHelper::Init())
    {
        return false;
    }

    const Error::ErrorCode linesProgramErrorCode = GLHelper::CreateProgram("assets/mapedit/basicVertexColor.frag",
                                                                           "assets/mapedit/basicVertexColor.vert",
                                                                           lineProgram);

    const Error::ErrorCode gridProgramErrorCode = GLHelper::CreateProgram("assets/mapedit/grid.frag",
                                                                          "assets/mapedit/grid.vert",
                                                                          gridProgram);

    const Error::ErrorCode cubeProgramErrorCode = GLHelper::CreateProgram("assets/mapedit/generic.frag",
                                                                          "assets/mapedit/generic.vert",
                                                                          genericProgram);

    const Error::ErrorCode spriteProgramErrorCode = GLHelper::CreateProgram("assets/mapedit/sprite.frag",
                                                                            "assets/mapedit/sprite.vert",
                                                                            spriteProgram);

    if (cubeProgramErrorCode != Error::ErrorCode::OK ||
        linesProgramErrorCode != Error::ErrorCode::OK ||
        gridProgramErrorCode != Error::ErrorCode::OK ||
        spriteProgramErrorCode != Error::ErrorCode::OK)
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

    const std::vector<std::string> modelsPaths = SharedMgr::ScanFolder(Options::GetAssetsPath() + "/model",
                                                                       ".gmdl",
                                                                       true);
    for (const std::string &modelPath: modelsPaths)
    {
        const ModelBuffer buf = LoadModel(Options::GetAssetsPath() + "/model/" + modelPath);
        modelBuffers[modelPath.substr(0, modelPath.length() - 5)] = buf;
    }

    return true;
}

void MapRenderer::Destroy()
{
    glDeleteProgram(genericProgram);
    glDeleteProgram(lineProgram);
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

void MapRenderer::RenderViewport(const Viewport &vp)
{
    glClearColor(0, 0, 0, 1);
    // glDisable(GL_SCISSOR_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLineWidth(1);

    glm::mat4 view = vp.GetMatrix();

    glDisable(GL_DEPTH_TEST);

    if (MapEditor::drawGrid)
    {
        const float gridSpacing = MapEditor::GRID_SPACING_VALUES.at(MapEditor::gridSpacingIndex);
        const int numInstances = static_cast<int>(MapEditor::MAP_SIZE * 2 / gridSpacing);

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
                             glm::mat4 &matrix,
                             const float thickness)
{
    glUseProgram(genericProgram);

    glLineWidth(thickness);

    glUniform4fv(glGetUniformLocation(genericProgram, "alb"), 1, color.GetDataPointer());
    glUniformMatrix4fv(glGetUniformLocation(genericProgram, "VIEW_MATRIX"), 1, GL_FALSE, glm::value_ptr(matrix));
    glUniformMatrix4fv(glGetUniformLocation(genericProgram, "WORLD_MATRIX"), 1, GL_FALSE, glm::value_ptr(identity));

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

void MapRenderer::RenderBillboardPoint(const glm::vec3 position, const float pointSize, Color color, glm::mat4 &matrix)
{
    glUseProgram(genericProgram);

    glPointSize(pointSize);

    glUniform4fv(glGetUniformLocation(genericProgram, "alb"), 1, color.GetDataPointer());
    glUniformMatrix4fv(glGetUniformLocation(genericProgram, "VIEW_MATRIX"), 1, GL_FALSE, glm::value_ptr(matrix));
    glUniformMatrix4fv(glGetUniformLocation(genericProgram, "WORLD_MATRIX"), 1, GL_FALSE, glm::value_ptr(identity));

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

void MapRenderer::RenderBillboardSprite(const glm::vec3 position,
                                        const float pointSize,
                                        const std::string &texture,
                                        Color color,
                                        glm::mat4 &matrix)
{
    glUseProgram(spriteProgram);

    glPointSize(pointSize);

    OpenGLImGuiTextureAssetCache *cache = dynamic_cast<OpenGLImGuiTextureAssetCache *>(SharedMgr::textureCache.get());

    GLuint textureId = -1;
    const Error::ErrorCode e = cache->GetTextureGLuint("texture/" + texture + ".gtex", textureId);
    assert(e == Error::ErrorCode::OK); // TODO handle properly

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glUniform1i(glGetUniformLocation(spriteProgram, "sprite"), 0);
    glUniform4fv(glGetUniformLocation(spriteProgram, "color"), 1, color.GetDataPointer());
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
                                   glm::mat4 &matrix,
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

void MapRenderer::RenderActor(const Actor &a, glm::mat4 &matrix)
{
    const glm::vec3 pos = glm::vec3(a.position.at(0), a.position.at(1), a.position.at(2));
    const glm::vec3 actorRotation = glm::vec3(a.rotation[0], a.rotation[1], a.rotation[2]);
    const ActorDefinition &definition = SharedMgr::actorDefinitions.at(a.className);
    Color c = definition.renderDefinition.color;
    if (!definition.renderDefinition.colorSourceParam.empty() &&
        a.params.contains(definition.renderDefinition.colorSourceParam))
    {
        c = a.params.at(definition.renderDefinition.colorSourceParam).Get<Color>(definition.renderDefinition.color);
    }
    std::string texture = definition.renderDefinition.texture;
    if (!definition.renderDefinition.textureSourceParam.empty() &&
        a.params.contains(definition.renderDefinition.textureSourceParam))
    {
        texture = a.params.at(definition.renderDefinition.textureSourceParam)
                          .Get<std::string>(definition.renderDefinition.texture);
    }

    if (texture.empty())
    {
        RenderBillboardPoint(pos, 10, c, matrix);
    } else
    {
        RenderBillboardSprite(pos, 20, texture, c, matrix);
    }

    if (definition.renderDefinition.model.empty() && definition.renderDefinition.modelSourceParam.empty())
    {
        RenderUnitVector(pos, actorRotation, c, matrix, 2, 1);
    } else
    {
        std::string model = definition.renderDefinition.model;
        if (!definition.renderDefinition.modelSourceParam.empty() &&
            a.params.contains(definition.renderDefinition.modelSourceParam))
        {
            model = a.params.at(definition.renderDefinition.modelSourceParam)
                            .Get<std::string>(definition.renderDefinition.model);
        }
        if (!modelBuffers.contains(model))
        {
            model = "error";
        }
        glm::mat4 worldMatrix = glm::identity<glm::mat4>();
        worldMatrix = glm::translate(worldMatrix, pos);
        worldMatrix = glm::rotate(worldMatrix, glm::radians(actorRotation.y), glm::vec3(0, 1, 0));
        worldMatrix = glm::rotate(worldMatrix, glm::radians(actorRotation.x), glm::vec3(1, 0, 0));
        worldMatrix = glm::rotate(worldMatrix, glm::radians(actorRotation.z), glm::vec3(0, 0, 1));

        RenderModel(modelBuffers.at(model), matrix, worldMatrix, c);
    }
}

MapRenderer::ModelBuffer MapRenderer::LoadModel(const std::string &path)
{
    ModelBuffer buf{};
    const Error::ErrorCode e = ModelAsset::CreateFromAsset(path, buf.model);
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

    const ModelLod &lod = buf.model.GetLod(0);
    for (size_t i = 0; i < lod.indexCounts.size(); i++)
    {
        GLuint ebo = 0;
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(lod.indexCounts.at(i) * sizeof(uint32_t)),
                     lod.materialIndices.at(i).data(),
                     GL_STATIC_DRAW);
        buf.ebos.push_back(ebo);
    }

    return buf;
}

void MapRenderer::RenderModel(ModelBuffer &buffer, glm::mat4 &viewMatrix, glm::mat4 &worldMatrix, Color &c)
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glUseProgram(genericProgram);

    glLineWidth(1);

    glUniform4fv(glGetUniformLocation(genericProgram, "alb"), 1, c.GetDataPointer());
    glUniformMatrix4fv(glGetUniformLocation(genericProgram, "VIEW_MATRIX"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(glGetUniformLocation(genericProgram, "WORLD_MATRIX"), 1, GL_FALSE, glm::value_ptr(worldMatrix));

    glBindVertexArray(buffer.vao);
    glBindBuffer(GL_ARRAY_BUFFER, buffer.vbo);

    const GLint posAttrib = glGetAttribLocation(genericProgram, "VERTEX");
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), nullptr);
    glEnableVertexAttribArray(posAttrib);

    for (size_t i = 0; i < buffer.ebos.size(); i++)
    {
        const GLuint ebo = buffer.ebos.at(i);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glDrawElements(GL_TRIANGLES,
                       static_cast<GLsizei>(buffer.model.GetLod(0).indexCounts.at(i)),
                       GL_UNSIGNED_INT,
                       nullptr);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
