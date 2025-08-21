//
// Created by droc101 on 6/27/25.
//

#pragma once

#include <cstdint>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <libassets/asset/ModelAsset.h>
#include <libassets/util/Error.h>
#include <vector>

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

        [[nodiscard]] static bool Init();

        static void Destroy();

        static void Render();

        static void LoadModel(ModelAsset &&newModel);

        static void UnloadModel();

        [[nodiscard]] static ModelAsset &GetModel();

        static void UpdateView(float pitchDegrees, float yawDegrees, float cameraDistance);

        static void UpdateViewRel(float pitchDegrees, float yawDegrees, float cameraDistance);

        static void ResizeWindow(GLsizei width, GLsizei height);

        static inline int lodIndex = 0;
        static inline int skinIndex = 0;

        static inline bool cullBackfaces = true;
        static inline DisplayMode displayMode = DisplayMode::TEXTURED_SHADED;
        static inline bool showUnitCube = true;
        static inline bool wireframe = false;
        static inline bool showBoundingBox = false;

        static constexpr int PANEL_SIZE = 100;

        static inline uint32_t EVENT_RELOAD_MODEL;
        static constexpr int32_t EVENT_RELOAD_MODEL_CODE_GMDL = 0;
        static constexpr int32_t EVENT_RELOAD_MODEL_CODE_IMPORT_MODEL = 1;
        static constexpr int32_t EVENT_RELOAD_MODEL_CODE_IMPORT_LOD = 2;

        static inline uint32_t EVENT_SAVE_MODEL;

    private:
        struct GLModelLod
        {
            GLuint vao{};
            GLuint vbo{};
            std::vector<GLuint> ebos{};
        };

        static inline ModelAsset model{};

        static inline GLuint cubeVao = 0;
        static inline GLuint cubeVbo = 0;

        static inline GLuint bboxVao = 0;
        static inline GLuint bboxVbo = 0;
        static inline GLuint bboxEbo = 0;

        static inline std::vector<GLModelLod> lods{};

        static inline GLuint program = 0;
        static inline GLuint linesProgram = 0;

        static inline float pitch = 0;
        static inline float yaw = 0;
        static inline float distance = 0;

        static inline glm::mat4 projection{};
        static inline glm::mat4 view{};

        static inline GLsizei windowWidth = 800;
        static inline GLsizei windowHeight = 600;
        static inline float windowAspect = 4.0 / 3.0;

        [[nodiscard]] static Error::ErrorCode CreateShader(const char *filename, GLenum type, GLuint &outShader);

        [[nodiscard]] static Error::ErrorCode CreateProgram(const char *fragmentFilename,
                                                            const char *vertexFilename,
                                                            GLuint &outProgram);

        static void UpdateMatrix();

        static void LoadCube();

        static void LoadBBox();
};
