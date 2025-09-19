//
// Created by droc101 on 9/5/25.
//

#include "GLHelper.h"
#include <array>
#include <cstdio>
#include <fstream>
#include <fstream>
#include <vector>
#include <ios>
#include <glm/vec2.hpp>
#include "GLDebug.h"
#include "libassets/util/Error.h"

bool GLHelper::Init()
{
    glewExperimental = GL_TRUE; // Please expose OpenGL 3.x+ interfaces
    const GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        printf("GLEW init failure");
        return false;
    }

    // Ensure we have GL 3.3 or higher
    if (!GLEW_VERSION_3_3)
    {
        printf("GLEW init failure -- we don't have opengl 3.3");
        return false;
    }

#ifdef BUILDSTYLE_DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(GLDebug::GL_DebugMessageCallback, nullptr);
#endif

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    const char *renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
    const char *version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version: %s\n", version);

    return true;
}

Error::ErrorCode GLHelper::CreateShader(const char *filename, const GLenum type, GLuint &outShader)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        printf("Failed to open shader file %s!\n", filename);
        return Error::ErrorCode::CANT_OPEN_FILE;
    }
    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> shaderSource(size + 1);
    file.read(shaderSource.data(), size);
    file.close();
    const GLuint shader = glCreateShader(type);
    const char *shaderSourceData = shaderSource.data();
    glShaderSource(shader, 1, &shaderSourceData, nullptr);
    glCompileShader(shader);
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == 0)
    {
        std::array<char, 512> infoLog{};
        glGetShaderInfoLog(shader, 512, nullptr, infoLog.data());
        printf("Shader compile failed: %s\n", infoLog.data());
        glDeleteShader(shader);
        return Error::ErrorCode::UNKNOWN;
    }
    outShader = shader;
    return Error::ErrorCode::OK;
}

Error::ErrorCode GLHelper::CreateProgram(const char *fragmentFilename,
                                         const char *vertexFilename,
                                         GLuint &outProgram)
{
    GLuint vertexShader = 0;
    GLuint fragmentShader = 0;
    const Error::ErrorCode vertexShaderErrorCode = CreateShader(vertexFilename, GL_VERTEX_SHADER, vertexShader);
    const Error::ErrorCode fragmentShaderErrorCode = CreateShader(fragmentFilename, GL_FRAGMENT_SHADER, fragmentShader);
    if (vertexShaderErrorCode != Error::ErrorCode::OK || fragmentShaderErrorCode != Error::ErrorCode::OK)
    {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        printf("Could not create shader program\n");
        return Error::ErrorCode::UNKNOWN;
    }

    const GLuint newProgram = glCreateProgram();
    glAttachShader(newProgram, vertexShader);
    glAttachShader(newProgram, fragmentShader);
    glLinkProgram(newProgram);
    GLint success = 0;
    glGetProgramiv(newProgram, GL_LINK_STATUS, &success);
    if (success == 0)
    {
        std::array<char, 512> infoLog{};
        glGetProgramInfoLog(newProgram, 512, nullptr, infoLog.data());
        fprintf(stderr, "Program link failed: %s\n", infoLog.data());
        glDeleteProgram(newProgram);
        return Error::ErrorCode::UNKNOWN;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    outProgram = newProgram;
    return Error::ErrorCode::OK;
}

GLHelper::GL_Buffer GLHelper::CreateBuffer()
{
    GL_Buffer buf{};
    glGenVertexArrays(1, &buf.vao);
    glBindVertexArray(buf.vao);

    glGenBuffers(1, &buf.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, buf.vbo);

    return buf;
}

GLHelper::GL_IndexedBuffer GLHelper::CreateIndexedBuffer()
{
    GL_IndexedBuffer buf{};
    glGenVertexArrays(1, &buf.vao);
    glBindVertexArray(buf.vao);

    glGenBuffers(1, &buf.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, buf.vbo);

    glGenBuffers(1, &buf.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf.ebo);

    return buf;
}

void GLHelper::DestroyBuffer(const GL_Buffer &buffer)
{
    glDeleteVertexArrays(1, &buffer.vao);
    glDeleteBuffers(1, &buffer.vbo);
}

void GLHelper::DestroyIndexedBuffer(const GL_IndexedBuffer &buffer)
{
    glDeleteVertexArrays(1, &buffer.vao);
    glDeleteBuffers(1, &buffer.vbo);
    glDeleteBuffers(1, &buffer.ebo);
}

void GLHelper::BindBuffer(const GL_Buffer &buffer)
{
    glBindVertexArray(buffer.vao);
    glBindBuffer(GL_ARRAY_BUFFER, buffer.vbo);
}

void GLHelper::BindIndexedBuffer(const GL_IndexedBuffer &buffer)
{
    glBindVertexArray(buffer.vao);
    glBindBuffer(GL_ARRAY_BUFFER, buffer.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer.ebo);
}

glm::vec2 GLHelper::ScreenToNDC(const glm::vec2 screenPos, const glm::vec2 screenSize)
{
    return {screenPos.x / screenSize.x * 2.0f - 1.0f, 1.0f - screenPos.y / screenSize.y * 2.0f};
}
