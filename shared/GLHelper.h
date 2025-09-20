//
// Created by droc101 on 9/5/25.
//

#pragma once
#include <GL/glew.h>
#include <glm/vec2.hpp>
#include "libassets/util/Error.h"


class GLHelper
{
    public:
        struct GL_IndexedBuffer
        {
                GLuint vao;
                GLuint vbo;
                GLuint ebo;
        };

        struct GL_Buffer
        {
                GLuint vao;
                GLuint vbo;
        };


        GLHelper() = delete;

        [[nodiscard]] static bool Init();

        [[nodiscard]] static Error::ErrorCode CreateShader(const char *filename, GLenum type, GLuint &outShader);

        [[nodiscard]] static Error::ErrorCode CreateProgram(const char *fragmentFilename,
                                                            const char *vertexFilename,
                                                            GLuint &outProgram);

        [[nodiscard]] static GL_Buffer CreateBuffer();
        [[nodiscard]] static GL_IndexedBuffer CreateIndexedBuffer();
        static void DestroyBuffer(const GL_Buffer &buffer);
        static void DestroyIndexedBuffer(const GL_IndexedBuffer &buffer);
        static void BindBuffer(const GL_Buffer &buffer);
        static void BindIndexedBuffer(const GL_IndexedBuffer &buffer);

        [[nodiscard]] static glm::vec2 ScreenToNDC(glm::vec2 screenPos, glm::vec2 screenSize);
};
