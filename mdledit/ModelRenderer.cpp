//
// Created by droc101 on 6/27/25.
//

#include "ModelRenderer.h"
#include <cstdio>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iosfwd>
#include "../shared/Options.h"
#include "GLDebug.h"
#include <libassets/util/DataWriter.h>
#include <libassets/asset/TextureAsset.h>
#include "OpenGLImGuiTextureAssetCache.h"
#include "SharedMgr.h"

// #define GL_CHECK_ERROR if (glGetError() != GL_NO_ERROR) {printf(reinterpret_cast<const char *>(glewGetErrorString(glGetError()))); fflush(stdout); __debugbreak();}

Error::ErrorCode ModelRenderer::CreateShader(const char *filename, const GLenum type, GLuint &out)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        return Error::ErrorCode::E_CANT_OPEN_FILE;
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
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == 0)
    {
        std::array<char, 512> infoLog{};
        glGetShaderInfoLog(shader, 512, nullptr, infoLog.data());
        fprintf(stderr, "Shader compile failed: %s\n", infoLog.data());
        glDeleteShader(shader);
        return Error::ErrorCode::E_UNKNOWN;
    }
    out = shader;
    return Error::ErrorCode::E_OK;
}

Error::ErrorCode ModelRenderer::CreateProgram(const char *fragFilename, const char *vertFilename, GLuint &outProgram)
{
    GLuint vertexShader;
    GLuint fragmentShader;
    const Error::ErrorCode ve = CreateShader(vertFilename, GL_VERTEX_SHADER, vertexShader);
    const Error::ErrorCode fe = CreateShader(fragFilename, GL_FRAGMENT_SHADER, fragmentShader);
    if (ve != Error::ErrorCode::E_OK || fe != Error::ErrorCode::E_OK)
    {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        printf("Could not create shader program\n");
        return Error::ErrorCode::E_UNKNOWN;
    }

    const GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == 0)
    {
        std::array<char, 512> infoLog{};
        glGetProgramInfoLog(program, 512, nullptr, infoLog.data());
        fprintf(stderr, "Program link failed: %s\n", infoLog.data());
        glDeleteProgram(program);
        return Error::ErrorCode::E_UNKNOWN;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    outProgram = program;
    return Error::ErrorCode::E_OK;
}


bool ModelRenderer::Init()
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
        printf("GLEW init failure -- we don't have opengl 3.0");
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

    const Error::ErrorCode me = CreateProgram("assets/model.frag", "assets/model.vert", program);
    const Error::ErrorCode ce = CreateProgram("assets/cube.frag", "assets/cube.vert", cubeProgram);
    if (me != Error::ErrorCode::E_OK || ce != Error::ErrorCode::E_OK) return false;

    LoadCube();

    UpdateView(0, 0, 1);

    const char *renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
    const char *version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version: %s\n", version);

    return true;
}

ModelAsset *ModelRenderer::GetModel()
{
    return &model;
}

void ModelRenderer::Destroy()
{
    UnloadModel();
    glDeleteProgram(program);
}

void ModelRenderer::UpdateView(const float pitchDeg, const float yawDeg, const float distance)
{
    pitch = glm::radians(pitchDeg);
    yaw = glm::radians(yawDeg);
    ModelRenderer::distance = distance;
    if (ModelRenderer::distance < 0.1)
    {
        ModelRenderer::distance = 0.1;
    }
    pitch = glm::clamp<float>(pitch, -90, 90);
    UpdateMatrix();
}

void ModelRenderer::UpdateViewRel(const float pitchDeg, const float yawDeg, const float distance)
{
    pitch += glm::radians(pitchDeg);
    yaw += glm::radians(yawDeg);
    ModelRenderer::distance += distance;
    if (ModelRenderer::distance < 0.1)
    {
        ModelRenderer::distance = 0.1;
    }
    pitch = glm::clamp<float>(pitch, -90, 90);
    UpdateMatrix();
}

void ModelRenderer::UnloadModel()
{
    for (const GLModelLod &i: lods)
    {
        glDeleteVertexArrays(1, &i.vao);
        glDeleteBuffers(1, &i.vbo);
        for (const GLuint buffer: i.ebos)
        {
            glDeleteBuffers(1, &buffer);
        }
    }
    lods.clear();
}

void ModelRenderer::LoadModel(ModelAsset &newModel)
{
    UnloadModel();
    model = newModel;
    lod = 0;
    skin = 0;

    for (size_t i = 0; i < newModel.GetLodCount(); i++)
    {
        const ModelLod &lod = newModel.GetLod(i);
        GLModelLod glod;
        glGenVertexArrays(1, &glod.vao);
        glBindVertexArray(glod.vao);
        glGenBuffers(1, &glod.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, glod.vbo);
        DataWriter writer;
        newModel.GetVertexBuffer(i, writer);
        std::vector<uint8_t> buffer;
        writer.CopyToVector(buffer);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(writer.GetBufferSize()), buffer.data(), GL_STATIC_DRAW);

        for (size_t j = 0; j < newModel.GetSkinCount(); j++)
        {
            for (size_t k = 0; k < newModel.GetMaterialCount(); k++)
            {
                const Material &mat = newModel.GetSkin(j)[k];
                // this just ensures it is loaded, we don't need it rn
                GLuint _;
                [[maybe_unused]] const Error::ErrorCode _e = dynamic_cast<OpenGLImGuiTextureAssetCache *>(
                    SharedMgr::textureCache)->GetTextureGLuint(mat.texture, _);
            }
        }

        for (size_t j = 0; j < newModel.GetMaterialCount(); j++)
        {
            GLuint ebo;
            glGenBuffers(1, &ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         static_cast<GLsizeiptr>(lod.indexCounts.at(j) * sizeof(uint32_t)),
                         lod.indices.at(j).data(),
                         GL_STATIC_DRAW);
            glod.ebos.push_back(ebo);
        }

        lods.push_back(glod);
    }

    UpdateView(0, 0, 1);
}

void ModelRenderer::Render()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    if (cullBackfaces)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    } else
    {
        glDisable(GL_CULL_FACE);
    }

    if (wireframe)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glUseProgram(program);
    glViewport(0, PANEL_SIZE, windowWidth, windowHeight - PANEL_SIZE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const GLModelLod &glod = lods.at(lod);
    glBindVertexArray(glod.vao);

    glBindBuffer(GL_ARRAY_BUFFER, glod.vbo);
    GLint posAttrib = glGetAttribLocation(program, "VERTEX");
    const GLint uvAttrib = glGetAttribLocation(program, "VERTEX_UV");
    const GLint normAttrib = glGetAttribLocation(program, "VERTEX_NORMAL");
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
    glVertexAttribPointer(uvAttrib,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          8 * sizeof(float),
                          reinterpret_cast<void *>(3 * sizeof(float)));
    glVertexAttribPointer(normAttrib,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          8 * sizeof(float),
                          reinterpret_cast<void *>(5 * sizeof(float)));
    glEnableVertexAttribArray(posAttrib);
    glEnableVertexAttribArray(uvAttrib);
    glEnableVertexAttribArray(normAttrib);

    glUniformMatrix4fv(glGetUniformLocation(program, "PROJECTION"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(program, "VIEW"), 1, GL_FALSE, glm::value_ptr(view));
    glUniform1i(glGetUniformLocation(program, "displayMode"), static_cast<GLint>(displayMode));

    for (size_t i = 0; i < model.GetMaterialCount(); i++)
    {
        Material &mat = model.GetSkin(skin)[i];
        glUniform3fv(glGetUniformLocation(program, "ALBEDO"), 1, mat.color.GetData());

        GLuint texture;
        const Error::ErrorCode e = dynamic_cast<OpenGLImGuiTextureAssetCache *>(SharedMgr::textureCache)->
                GetTextureGLuint(mat.texture, texture);
        if (e != Error::ErrorCode::E_OK) continue;
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1ui(glGetUniformLocation(program, "ALBEDO_TEXTURE"), texture);

        const GLuint ebo = glod.ebos.at(i);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glDrawElements(GL_TRIANGLES,
                       static_cast<GLsizei>(model.GetLod(lod).indexCounts.at(i)),
                       GL_UNSIGNED_INT,
                       nullptr);
    }

    if (showUnitCube)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glUseProgram(cubeProgram);
        glBindVertexArray(cubeVao);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVbo);
        posAttrib = glGetAttribLocation(cubeProgram, "VERTEX");
        glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        glEnableVertexAttribArray(posAttrib);
        glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "PROJECTION"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "VIEW"), 1, GL_FALSE, glm::value_ptr(view));
        glDrawArrays(GL_LINES, 0, 24);
    }
}

void ModelRenderer::ResizeWindow(const GLsizei width, const GLsizei height)
{
    windowWidth = width;
    windowHeight = height;
    windowAspect = static_cast<float>(width) / static_cast<float>(height - PANEL_SIZE);
    UpdateMatrix();
}

void ModelRenderer::UpdateMatrix()
{
    const glm::mat4 &persp = glm::perspective<float>(90.0, windowAspect, 0.01f, 1000.0f);

    const float x = distance * cosf(pitch) * sinf(yaw);
    const float y = distance * sinf(pitch);
    const float z = distance * cosf(pitch) * cosf(yaw);
    const glm::vec3 cameraPos{x, y, z};
    const glm::mat4 &look = glm::lookAt(cameraPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    projection = persp;
    view = look;
}

void ModelRenderer::LoadCube()
{
    // clang-format off
    constexpr std::array cubeVerts = {
        // Bottom face
        -0.5f, -0.5f, -0.5f,   0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,   0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,  -0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,  -0.5f, -0.5f, -0.5f,

        // Top face
        -0.5f, -0.5f,  0.5f,   0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,   0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,  -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,  -0.5f, -0.5f,  0.5f,

        // Vertical edges
        -0.5f, -0.5f, -0.5f,  -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f, -0.5f,   0.5f, -0.5f,  0.5f,
         0.5f,  0.5f, -0.5f,   0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,  -0.5f,  0.5f,  0.5f,
    };
    // clang-format on

    glGenVertexArrays(1, &cubeVao);
    glBindVertexArray(cubeVao);

    glGenBuffers(1, &cubeVbo);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * cubeVerts.size(), cubeVerts.data(), GL_STATIC_DRAW);
}
