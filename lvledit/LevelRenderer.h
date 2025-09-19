//
// Created by droc101 on 9/5/25.
//

#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "GLHelper.h"
#include "Viewport.h"
#include "libassets/util/Color.h"

class LevelRenderer
{
    public:
        LevelRenderer() = delete;

        static bool Init();
        static void Destroy();

        static void RenderViewport(const Viewport &vp);

        static void RenderLine(glm::vec3 start, glm::vec3 end, Color color, glm::mat4 &matrix, float thickness);

        static void RenderBillboardPoint(glm::vec3 position, float pointSize, Color color, glm::mat4 &matrix);

    private:
        static inline GLuint genericProgram = 0;
        static inline GLuint lineProgram = 0;
        static inline GLuint gridProgram = 0;

        inline static GLHelper::GL_Buffer axisHelperBuffer{};
        inline static GLHelper::GL_Buffer worldBorderBuffer{};

        inline static GLHelper::GL_IndexedBuffer workBuffer{};
        inline static GLHelper::GL_Buffer workBufferNonIndexed{};
};
