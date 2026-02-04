//
// Created by droc101 on 9/5/25.
//

#pragma once

#include <GL/glew.h>
#include <glm/vec2.hpp>
#include <libassets/util/Error.h>

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

        struct GL_Framebuffer
        {
                bool created = false;
                glm::vec2 size;
                GLuint colorTexture;
                GLuint fbo;
                GLuint rbo;
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

        [[nodiscard]] static GL_Framebuffer CreateFramebuffer(glm::vec2 size);
        static void ResizeFramebuffer(GL_Framebuffer &framebuffer, glm::vec2 newSize);
        static void BindFramebuffer(const GL_Framebuffer &framebuffer);
        static void DestroyFramebuffer(GL_Framebuffer &framebuffer);

        static void UnbindFramebuffer();

        [[nodiscard]] static glm::vec2 ScreenToNDC(glm::vec2 screenPos, glm::vec2 screenSize);
};
