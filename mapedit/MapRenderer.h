//
// Created by droc101 on 9/5/25.
//

#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <libassets/type/Color.h>
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

    private:
        static inline GLuint genericProgram = 0;
        static inline GLuint lineProgram = 0;
        static inline GLuint gridProgram = 0;

        static inline GLHelper::GL_Buffer axisHelperBuffer{};
        static inline GLHelper::GL_Buffer worldBorderBuffer{};

        static inline GLHelper::GL_IndexedBuffer workBuffer{};
        static inline GLHelper::GL_Buffer workBufferNonIndexed{};
};
