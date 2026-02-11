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

        /**
         * Initialize OpenGL
         * @return true on success, false on failure
         */
        [[nodiscard]] static bool Init();

        /**
         * Create a shader from a file
         * @param filename The file containing GLSL source code
         * @param type The shader type (fragment, vertex, etc.)
         * @param outShader Where to store the shader ID
         */
        [[nodiscard]] static Error::ErrorCode CreateShader(const char *filename, GLenum type, GLuint &outShader);

        /**
         * Create a program from two shader filenames
         * @param fragmentFilename Fragment shader filename
         * @param vertexFilename Vertex shader filename
         * @param outProgram Where to store the program ID
         */
        [[nodiscard]] static Error::ErrorCode CreateProgram(const char *fragmentFilename,
                                                            const char *vertexFilename,
                                                            GLuint &outProgram);

        /**
         * Create a non-indexed buffer with a VAO
         */
        [[nodiscard]] static GL_Buffer CreateBuffer();

        /**
         * Create an indexed buffer with a VAO
         */
        [[nodiscard]] static GL_IndexedBuffer CreateIndexedBuffer();

        /**
         * Destroy a non-indexed buffer
         */
        static void DestroyBuffer(const GL_Buffer &buffer);

        /**
         * Destroy an indexed buffer
         */
        static void DestroyIndexedBuffer(const GL_IndexedBuffer &buffer);

        /**
         * Bind a non-indexed buffer
         */
        static void BindBuffer(const GL_Buffer &buffer);

        /**
         * Bind an indexed buffer
         */
        static void BindIndexedBuffer(const GL_IndexedBuffer &buffer);

        /**
         * Create a framebuffer
         * @param size Framebuffer size
         */
        [[nodiscard]] static GL_Framebuffer CreateFramebuffer(glm::vec2 size);

        /**
         * Resize a framebuffer
         * @param framebuffer The framebuffer to resize
         * @param newSize The new size
         */
        static void ResizeFramebuffer(GL_Framebuffer &framebuffer, glm::vec2 newSize);

        /**
         * Bind a framebuffer
         */
        static void BindFramebuffer(const GL_Framebuffer &framebuffer);

        /**
         * Destroy a framebuffer
         */
        static void DestroyFramebuffer(GL_Framebuffer &framebuffer);

        /**
         * Unbind any active framebuffer
         */
        static void UnbindFramebuffer();

        /**
         * Convert screen pixel coordinates to NDC space
         * @param screenPos Pixel coordinate
         * @param screenSize Screen size
         */
        [[nodiscard]] static glm::vec2 ScreenToNDC(glm::vec2 screenPos, glm::vec2 screenSize);
};
