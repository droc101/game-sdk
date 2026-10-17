//
// Created by droc101 on 9/5/25.
//

#pragma once

#include <cstdint>
#include <game_sdk/gl/GLHelper.h>
#include <GL/glew.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <libassets/asset/ModelAsset.h>
#include <libassets/type/Brush.h>
#include <libassets/type/Color.h>
#include <string>
#include <unordered_map>
#include <vector>
#include "Viewport.h"

class MapRenderer
{
    public:
        struct Wall
        {
                Color color;
                bool zAxisOrientation;

                glm::vec2 localCenter;
                glm::vec2 size;

                std::string texture;

                glm::vec2 uvOffset;
                glm::vec2 uvScale;

                bool unshaded;
        };

        struct ObjectBufferInfo
        {
                uint32_t objectType;
                uint32_t objectIndex;
                uint32_t objectSubIndex;
        };

        MapRenderer() = delete;

        static bool Init();
        static void Destroy();

        static void RenderViewportGrid(const Viewport &vp);

        static void RenderLine(glm::vec3 start, glm::vec3 end, Color color, const glm::mat4 &matrix, float thickness);

        static void RenderBillboardPoint(glm::vec3 position, float pointSize, Color color, const glm::mat4 &matrix);

        static void RenderBillboardPointObjectBuffer(glm::vec3 position,
                                                     float pointSize,
                                                     ObjectBufferInfo info,
                                                     const glm::mat4 &matrix);

        static void RenderBillboardSprite(glm::vec3 position,
                                          float pointSize,
                                          const std::string &texture,
                                          Color color,
                                          const glm::mat4 &matrix);

        static void RenderUnitVector(glm::vec3 origin,
                                     glm::vec3 eulerAngles,
                                     Color color,
                                     const glm::mat4 &matrix,
                                     float thickness,
                                     float length);

        static void RenderModel(std::string model,
                                const glm::mat4 &viewMatrix,
                                const glm::mat4 &worldMatrix,
                                const Color &modColor);

        static void RenderModelTextured(std::string model,
                                        const glm::mat4 &viewMatrix,
                                        const glm::mat4 &worldMatrix,
                                        uint32_t skin,
                                        const Color &modColor,
                                        const Color &overlayColor);

        static void RenderModelObjectBuffer(std::string model,
                                            const glm::mat4 &viewMatrix,
                                            const glm::mat4 &worldMatrix,
                                            ObjectBufferInfo info);

        static void RenderBrushFace(const Brush::TriangulatedFace &face,
                                    const glm::mat4 &viewMatrix,
                                    const glm::mat4 &worldMatrix,
                                    const Color &overlayColor);

        static void RenderBrushFaceObjectBuffer(const Brush::TriangulatedFace &face,
                                                const glm::mat4 &viewMatrix,
                                                const glm::mat4 &worldMatrix,
                                                ObjectBufferInfo info);

        static const ModelAsset &GetModel(std::string model);

        static void RenderWall(const glm::mat4 &viewMatrix,
                               const glm::mat4 &worldMatrix,
                               const Wall &wall,
                               const Color &overlayColor);

        static void RenderWallObjectBuffer(const glm::mat4 &viewMatrix,
                                           const glm::mat4 &worldMatrix,
                                           const Wall &wall,
                                           ObjectBufferInfo info);

    private:
        struct ModelBuffer
        {
                ModelAsset model;
                GLuint vao;
                GLuint vbo;
                std::vector<GLuint> ebos;
        };

        static inline GLuint uniformColorProgram = 0;
        static inline GLuint vertexColorProgram = 0;
        static inline GLuint gridProgram = 0;
        static inline GLuint spriteProgram = 0;
        static inline GLuint shadedModelProgram = 0;
        static inline GLuint brushProgram = 0;
        static inline GLuint worldBuffersProgram = 0;

        static inline GLHelper::GL_Buffer axisHelperBuffer{};
        static inline GLHelper::GL_Buffer worldBorderBuffer{};

        static inline GLHelper::GL_IndexedBuffer workBuffer{};
        static inline GLHelper::GL_Buffer workBufferNonIndexed{};

        static inline std::unordered_map<std::string, ModelBuffer> modelBuffers{};

        static inline glm::mat4 identity = glm::identity<glm::mat4>();

        static ModelBuffer LoadModel(const std::string &path);

        static void RenderModel(ModelBuffer &buffer,
                                const glm::mat4 &viewMatrix,
                                const glm::mat4 &worldMatrix,
                                const Color &c);

        static void RenderModelTextured(ModelBuffer &buffer,
                                        const glm::mat4 &viewMatrix,
                                        const glm::mat4 &worldMatrix,
                                        uint32_t skinIndex,
                                        const Color &modColor,
                                        const Color &overlayColor);

        static void RenderModelObjectBufferInternal(ModelBuffer &buffer,
                                                    const glm::mat4 &viewMatrix,
                                                    const glm::mat4 &worldMatrix,
                                                    ObjectBufferInfo info);
};
