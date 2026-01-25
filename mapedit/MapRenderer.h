//
// Created by droc101 on 9/5/25.
//

#pragma once

#include <GL/glew.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <libassets/asset/ModelAsset.h>
#include <libassets/type/Actor.h>
#include <libassets/type/Color.h>
#include <string>
#include <unordered_map>
#include <vector>
#include "GLHelper.h"
#include "Viewport.h"

class MapRenderer
{
    public:
        MapRenderer() = delete;

        static bool Init();
        static void Destroy();

        static void RenderViewport(const Viewport &vp);

        static void RenderLine(glm::vec3 start, glm::vec3 end, Color color, glm::mat4 &matrix, float thickness);

        static void RenderBillboardPoint(glm::vec3 position, float pointSize, Color color, glm::mat4 &matrix);

        static void RenderBillboardSprite(glm::vec3 position,
                                          float pointSize,
                                          const std::string &texture,
                                          Color color,
                                          glm::mat4 &matrix);

        static void RenderUnitVector(glm::vec3 origin,
                                     glm::vec3 eulerAngles,
                                     Color color,
                                     glm::mat4 &matrix,
                                     float thickness,
                                     float length);

        static void RenderActor(const Actor &a, glm::mat4 &matrix);

    private:
        struct ModelBuffer
        {
                ModelAsset model;
                GLuint vao;
                GLuint vbo;
                std::vector<GLuint> ebos;
        };

        static inline GLuint genericProgram = 0;
        static inline GLuint lineProgram = 0;
        static inline GLuint gridProgram = 0;
        static inline GLuint spriteProgram = 0;

        static inline GLHelper::GL_Buffer axisHelperBuffer{};
        static inline GLHelper::GL_Buffer worldBorderBuffer{};

        static inline GLHelper::GL_IndexedBuffer workBuffer{};
        static inline GLHelper::GL_Buffer workBufferNonIndexed{};

        static inline std::unordered_map<std::string, ModelBuffer> modelBuffers{};

        static inline glm::mat4 identity = glm::identity<glm::mat4>();

        static ModelBuffer LoadModel(const std::string &path);

        static void RenderModel(ModelBuffer &buffer, glm::mat4 &viewMatrix, glm::mat4 &worldMatrix, Color &c);
};
