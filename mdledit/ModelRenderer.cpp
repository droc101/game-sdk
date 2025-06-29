//
// Created by droc101 on 6/27/25.
//

#include "ModelRenderer.h"
#include <cstdio>
#include <fstream>
#include <iosfwd>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Options.h"
#include "libassets/TextureAsset.h"

GLuint ModelRenderer::CreateShader(const char* filename, const GLenum type)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        printf("Could not open file %s\n", filename);
        return -1;
    }
    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    char* shaderSource = new char[size + 1];
    file.read(shaderSource, size);
    shaderSource[size] = 0;
    file.close();
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &shaderSource, nullptr);
    delete[] shaderSource;
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == 0)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        fprintf(stderr, "Shader compile failed: %s\n", infoLog);
        glDeleteShader(shader);
        assert(success != 0);
        return -1;
    }
    return shader;
}

GLuint ModelRenderer::CreateTexture(const char *filename)
{
    const TextureAsset ta = TextureAsset::CreateFromAsset(filename);

    GLuint gt;
    const uint32_t *pixels = ta.GetPixelsRGBA();
    glGenTextures(1, &gt);
    glBindTexture(GL_TEXTURE_2D, gt);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(ta.GetWidth()), static_cast<GLsizei>(ta.GetHeight()), 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    delete[] pixels;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    return gt;
}


void ModelRenderer::Init()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    const GLuint vertexShader = CreateShader("assets/model.vert", GL_VERTEX_SHADER);
    const GLuint fragmentShader = CreateShader("assets/model.frag", GL_FRAGMENT_SHADER);
    if (vertexShader == -1 || fragmentShader == -1)
    {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        printf("Could not create shader program\n");
        assert(vertexShader != -1 || fragmentShader != -1);
        return;
    }

    program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == 0)
    {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        fprintf(stderr, "Program link failed: %s\n", infoLog);
        glDeleteProgram(program);
        assert(false);
    }

    UpdateView(0,0,1);

    const char* renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
    const char* version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
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
    for (const std::pair<std::string, GLuint> tex: texture_buffers)
    {
        glDeleteTextures(1, &tex.second);
    }
    texture_buffers.clear();
}

void ModelRenderer::UpdateView(float pitchDeg, float yawDeg, float distance)
{
    pitch = glm::radians(pitchDeg);
    yaw = glm::radians(yawDeg);
    ModelRenderer::distance = distance;
    if (ModelRenderer::distance < 0.1) ModelRenderer::distance = 0.1;
    pitch = glm::clamp<float>(pitch, -90, 90);
    UpdateMatrix();
}

void ModelRenderer::UpdateViewRel(float pitchDeg, float yawDeg, float distance)
{
    pitch += glm::radians(pitchDeg);
    yaw += glm::radians(yawDeg);
    ModelRenderer::distance += distance;
    if (ModelRenderer::distance < 0.1) ModelRenderer::distance = 0.1;
    pitch = glm::clamp<float>(pitch, -90, 90);
    UpdateMatrix();
}

void ModelRenderer::UnloadModel()
{
    texture_buffers.clear();
    for (const GLModelLod &i: lods)
    {
        glDeleteVertexArrays(1, &i.vao);
        glDeleteBuffers(1, &i.vbo);
        for (const GLuint buffer : i.ebos)
        {

            glDeleteBuffers(1, &buffer);
        }
    }
    lods.clear();
}

void ModelRenderer::LoadModel(const char *filepath)
{
    UnloadModel();
    model = ModelAsset::CreateFromAsset(filepath);
    lod = 0;
    skin = 0;

    for (size_t i = 0; i < model.GetLodCount(); i++)
    {
        ModelAsset::ModelLod lod = model.GetLod(i);
        GLModelLod glod = GLModelLod();
        glGenVertexArrays(1, &glod.vao);
        glBindVertexArray(glod.vao);
        glGenBuffers(1, &glod.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, glod.vbo);
        size_t vertexBufferSize;
        const uint8_t *memBuf = model.GetVertexBuffer(i, &vertexBufferSize);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertexBufferSize), memBuf, GL_STATIC_DRAW);
        delete[] memBuf;

        for (size_t j = 0; j < model.GetSkinCount(); j++)
        {
            for (size_t k = 0; k < model.GetMaterialCount(); k++)
            {
                const ModelAsset::Material mat = model.GetSkin(j)[k];
                std::string const texture = std::string(mat.texture);
                std::string const textureFull = Options::gamePath.data() + std::string("/assets/") + texture;
                if (!texture_buffers.contains(texture))
                {
                    const GLuint glTex = CreateTexture(textureFull.data());

                    texture_buffers.insert({texture, glTex});
                }
            }
        }

        for (size_t j = 0; j < model.GetMaterialCount(); j++)
        {
            GLuint ebo;
            glGenBuffers(1, &ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(lod.indexCounts.at(j) * sizeof(uint32_t)), lod.indices.at(j).data(), GL_STATIC_DRAW);
            glod.ebos.push_back(ebo);
        }

        lods.emplace_back(glod);
    }

    UpdateView(0,0,1);
}

void ModelRenderer::Render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_SCISSOR_TEST);

    glUseProgram(program);
    glViewport(0, 250, windowWidth, windowHeight - 250);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const GLModelLod& glod = lods.at(lod);
    glBindVertexArray(glod.vao);

    glBindBuffer(GL_ARRAY_BUFFER, glod.vbo);
    const GLint posAttrib = glGetAttribLocation(program, "VERTEX");
    const GLint uvAttrib = glGetAttribLocation(program, "VERTEX_UV");
    const GLint normAttrib = glGetAttribLocation(program, "VERTEX_NORMAL");
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
    glVertexAttribPointer(uvAttrib, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void *>(3 * sizeof(float)));
    glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void *>(5 * sizeof(float)));
    glEnableVertexAttribArray(posAttrib);
    glEnableVertexAttribArray(uvAttrib);
    glEnableVertexAttribArray(normAttrib);

    glUniformMatrix4fv(glGetUniformLocation(program, "PROJECTION"), 1, GL_FALSE, glm::value_ptr(projection));

    for (size_t i = 0; i < model.GetMaterialCount(); i++)
    {
        ModelAsset::Material const mat = model.GetSkin(skin)[i];
        const uint32_t argb = mat.color;

        std::array<float, 4> color = {
            (static_cast<float>((argb >> 16) & 0xFF)) / 255.0f,
            (static_cast<float>((argb >> 8) & 0xFF)) / 255.0f,
            (static_cast<float>((argb) & 0xFF)) / 255.0f,
            (static_cast<float>((argb >> 24) & 0xFF)) / 255.0f,
        };
        glUniform3fv(glGetUniformLocation(program, "ALBEDO"), 1, color.data());

        const GLuint texture = texture_buffers.at(mat.texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1ui(glGetUniformLocation(program, "ALBEDO_TEXTURE"), texture);

        const GLuint ebo = glod.ebos.at(i);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(model.GetLod(lod).indexCounts.at(i)), GL_UNSIGNED_INT, nullptr);
    }
}

void ModelRenderer::ResizeWindow(GLsizei w, GLsizei h)
{
    windowWidth = w;
    windowHeight = h;
    windowAspect = static_cast<float>(w) / static_cast<float>(h - 250);
    UpdateMatrix();
}

void ModelRenderer::UpdateMatrix()
{
    const glm::mat4 persp = glm::perspective<float>(90.0, windowAspect, 0.05f, 1000.0f);

    const float x = distance * cosf(pitch) * sinf(yaw);
    const float y = distance * sinf(pitch);
    const float z = distance * cosf(pitch) * cosf(yaw);
    const glm::vec3 cameraPos = glm::vec3(x, y, z);
    const glm::mat4 look = glm::lookAt(cameraPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    projection = persp * look;
}

int *ModelRenderer::GetLOD()
{
    return &lod;
}

int *ModelRenderer::GetSkin()
{
    return &skin;
}
