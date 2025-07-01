//
// Created by droc101 on 6/27/25.
//

#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <libassets/ModelAsset.h>
#include <string>
#include <unordered_map>

class ModelRenderer
{
    public:
        enum class DisplayMode : uint8_t
        {
            COLORED,
            COLORED_SHADED,
            TEXTURED,
            TEXTURED_SHADED,
            UV,
            NORMAL,
        };

        ModelRenderer() = delete;

        static void Init();

        static void Destroy();

        static void Render();

        static void LoadModel(const ModelAsset &newModel);

        static void UnloadModel();

        static const ModelAsset *GetModel();

        static void UpdateView(float pitchDeg, float yawDeg, float distance);

        static void UpdateViewRel(float pitchDeg, float yawDeg, float distance);

        static void ResizeWindow(GLsizei width, GLsizei height);

        static inline int lod;
        static inline int skin;

        static inline bool cullBackfaces = true;
        static inline DisplayMode displayMode = DisplayMode::TEXTURED_SHADED;
        static inline bool showUnitCube = true;
        static inline bool wireframe = false;

        static constexpr int PANEL_SIZE = 100;

    private:
        struct GLModelLod
        {
            GLuint vao{};
            GLuint vbo{};
            std::vector<GLuint> ebos{};
        };

        static inline ModelAsset model;

        static inline GLuint cubeVao;
        static inline GLuint cubeVbo;

        static inline std::vector<GLModelLod> lods{};
        static inline std::unordered_map<std::string, GLuint> textureBuffers{};

        static inline GLuint program;
        static inline GLuint cubeProgram;

        static inline float pitch;
        static inline float yaw;
        static inline float distance;
        static inline glm::mat4 projection;

        static inline GLsizei windowWidth = 800;
        static inline GLsizei windowHeight = 600;
        static inline float windowAspect = 4.0 / 3.0;

        static GLuint CreateShader(const char *filename, GLenum type);

        static GLuint CreateTexture(const char *filename);

        static GLuint CreateProgram(const char *fragFilename, const char *vertFilename);

        static void UpdateMatrix();

        static void LoadCube();
};
