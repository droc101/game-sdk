//
// Created by droc101 on 6/27/25.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <game_sdk/gl/GLHelper.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <libassets/asset/ModelAsset.h>
#include <libassets/type/Color.h>
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

        static void LoadHulls();

        static void LoadStaticCollision();

        static ImTextureID GetFramebufferTexture();

        static ImVec2 GetFramebufferSize();

        static inline int lodIndex = 0;
        static inline int skinIndex = 0;

        static inline bool cullBackfaces = true;
        static inline DisplayMode displayMode = DisplayMode::TEXTURED_SHADED;
        static inline bool showUnitCube = true;
        static inline bool wireframe = false;
        static inline bool showBoundingBox = false;
        static inline bool showCollisionModel = false;

        static inline Color backgroundColor = Color(0, 0, 0, 1);

    private:
        struct GLModelLod
        {
                GLuint vao{};
                GLuint vbo{};
                std::vector<GLuint> ebos{};
        };

        struct GLHull
        {
                GLuint vao{};
                GLuint vbo{};
                size_t elements{};
        };

        static inline ModelAsset model{};

        static inline GLHelper::GL_Buffer cubeBuffer;

        static inline GLHelper::GL_IndexedBuffer bboxBuffer;

        static inline GLuint staticCollisionVao = 0;
        static inline GLuint staticCollisionVbo = 0;

        static inline std::vector<GLHull> hulls{};

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

        static inline GLHelper::GL_Framebuffer framebuffer;

        static void UpdateMatrix();

        static void LoadCube();

        static void LoadBBox();
};
