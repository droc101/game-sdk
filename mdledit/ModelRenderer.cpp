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
#include "libassets/DataWriter.h"
#include "libassets/TextureAsset.h"

GLuint ModelRenderer::CreateShader(const char *filename, const GLenum type)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        printf("Could not open file %s\n", filename);
        return -1;
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
        assert(success != 0);
        return -1;
    }
    return shader;
}

GLuint ModelRenderer::CreateTexture(const char *filename)
{
    const TextureAsset &textureAsset = TextureAsset::CreateFromAsset(filename);
    std::vector<uint32_t> pixels;
    textureAsset.GetPixelsRGBA(pixels);

    GLuint glTexture;
    glGenTextures(1, &glTexture);
    glBindTexture(GL_TEXTURE_2D, glTexture);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 static_cast<GLsizei>(textureAsset.GetWidth()),
                 static_cast<GLsizei>(textureAsset.GetHeight()),
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    return glTexture;
}

GLuint ModelRenderer::CreateProgram(const char *fragFilename, const char *vertFilename)
{
    const GLuint vertexShader = CreateShader(vertFilename, GL_VERTEX_SHADER);
    const GLuint fragmentShader = CreateShader(fragFilename, GL_FRAGMENT_SHADER);
    if (vertexShader == -1u || fragmentShader == -1u)
    {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        printf("Could not create shader program\n");
        assert(vertexShader != -1u || fragmentShader != -1u);
        return -1;
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
        assert(success != 0);
        return -1;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}


void ModelRenderer::Init()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    program = CreateProgram("assets/model.frag", "assets/model.vert");
    cubeProgram = CreateProgram("assets/cube.frag", "assets/cube.vert");

    LoadCube();

    UpdateView(0, 0, 1);

    const char *renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
    const char *version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version: %s\n", version);
}

const ModelAsset *ModelRenderer::GetModel()
{
    return &model;
}

void ModelRenderer::Destroy()
{
    UnloadModel();
    glDeleteProgram(program);
    for (const std::pair<std::string, GLuint> tex: textureBuffers)
    {
        glDeleteTextures(1, &tex.second);
    }
    textureBuffers.clear();
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
    textureBuffers.clear();
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

void ModelRenderer::LoadModel(const ModelAsset & newModel)
{
    UnloadModel();
    model = newModel;
    lod = 0;
    skin = 0;

    for (size_t i = 0; i < newModel.GetLodCount(); i++)
    {
        const ModelAsset::ModelLod &lod = newModel.GetLod(i);
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
                const ModelAsset::Material &mat = newModel.GetSkin(j)[k];
                if (!textureBuffers.contains(mat.texture))
                {
                    const std::string &texturePath = Options::gamePath.data() + std::string("/assets/") + mat.texture;
                    const GLuint glTex = CreateTexture(texturePath.c_str());

                    textureBuffers.insert({mat.texture, glTex});
                }
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

        lods.emplace_back(glod);
    }

    UpdateView(0, 0, 1);
}

void ModelRenderer::Render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
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

    glDisable(GL_STENCIL_TEST);
    glDisable(GL_SCISSOR_TEST);

    glUseProgram(program);
    glViewport(0, 250, windowWidth, windowHeight - 250);
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
    glUniform1i(glGetUniformLocation(program, "displayMode"), static_cast<GLint>(displayMode));

    for (size_t i = 0; i < model.GetMaterialCount(); i++)
    {
        const ModelAsset::Material &mat = model.GetSkin(skin)[i];
        const uint32_t argb = mat.color;

        std::array<float, 4> color = {
            (static_cast<float>((argb >> 16) & 0xFF)) / 255.0f,
            (static_cast<float>((argb >> 8) & 0xFF)) / 255.0f,
            (static_cast<float>((argb) & 0xFF)) / 255.0f,
            (static_cast<float>((argb >> 24) & 0xFF)) / 255.0f,
        };
        glUniform3fv(glGetUniformLocation(program, "ALBEDO"), 1, color.data());

        const GLuint texture = textureBuffers.at(mat.texture);
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
        glUseProgram(cubeProgram);
        glBindVertexArray(cubeVao);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVbo);
        posAttrib = glGetAttribLocation(cubeProgram, "VERTEX");
        glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        glEnableVertexAttribArray(posAttrib);
        glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "PROJECTION"), 1, GL_FALSE, glm::value_ptr(projection));
        glDrawArrays(GL_LINES, 0, 24);
    }
}

void ModelRenderer::ResizeWindow(const GLsizei width, const GLsizei height)
{
    windowWidth = width;
    windowHeight = height;
    windowAspect = static_cast<float>(width) / static_cast<float>(height - 250);
    UpdateMatrix();
}

void ModelRenderer::UpdateMatrix()
{
    const glm::mat4 &persp = glm::perspective<float>(90.0, windowAspect, 0.05f, 1000.0f);

    const float x = distance * cosf(pitch) * sinf(yaw);
    const float y = distance * sinf(pitch);
    const float z = distance * cosf(pitch) * cosf(yaw);
    const glm::vec3 cameraPos{x, y, z};
    const glm::mat4 &look = glm::lookAt(cameraPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    projection = persp * look;
}

void ModelRenderer::LoadCube()
{
    // clang-format off
    constexpr std::array<GLfloat, 72> cubeVerts = {
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
