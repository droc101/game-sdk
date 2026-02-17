//
// Created by droc101 on 2/17/26.
//

#ifndef GAME_SDK_MODELVIEWER_H
#define GAME_SDK_MODELVIEWER_H

#include <cstddef>
#include <cstdint>
#include <game_sdk/gl/GLHelper.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <libassets/asset/ModelAsset.h>
#include <libassets/type/Color.h>
#include <vector>

class ModelViewer
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

        ModelViewer() = default;

        [[nodiscard]] static bool GlobalInit();

        static void GlobalDestroy();

        /**
         * Initialize the model viewer's internal GL objects
         * @return Whether the initialization succeeded
         */
        [[nodiscard]] bool Init();

        /**
         * Destroy the model viewer's internal GL objects
         */
        void Destroy();


        /**
         * Set the model viewer's model
         * @param newModel The model to view
         */
        void SetModel(ModelAsset &&newModel);

        void ReloadModel();

        /**
         * Get the model
         */
        [[nodiscard]] ModelAsset &GetModel();

        /**
         * Set the view
         * @param pitchDegrees Pitch in degrees
         * @param yawDegrees Yaw in degrees
         * @param cameraDistance Camera distance
         */
        void UpdateView(float pitchDegrees, float yawDegrees, float cameraDistance);

        /**
         * Update the view
         * @param pitchDegrees Pitch change in degrees
         * @param yawDegrees Yaw change in degrees
         * @param cameraDistance Camera distance change
         */
        void UpdateViewRel(float pitchDegrees, float yawDegrees, float cameraDistance);


        /**
         * Render an ImGui window containing this model viewer
         * @param title The window title
         * @param additionalFlags Additional window flags
         */
        void RenderWindow(const char *title, ImGuiWindowFlags additionalFlags);

        /**
         * Render an ImGui child window containing this model viewer
         * @param title The window title
         * @param size
         * @param additionalChildFlags Additional child window flags
         * @param additionalWindowFlags Additional window flags
         */
        void RenderChildWindow(const char *title,
                               ImVec2 size,
                               ImGuiChildFlags additionalChildFlags,
                               ImGuiWindowFlags additionalWindowFlags);

        int lodIndex = 0;
        int skinIndex = 0;

        bool cullBackfaces = true;
        DisplayMode displayMode = DisplayMode::TEXTURED_SHADED;
        bool showUnitCube = true;
        bool wireframe = false;
        bool showBoundingBox = false;
        bool showCollisionModel = false;

        Color backgroundColor = Color(0, 0, 0, 1);

    private:
        class ModelViewerShared
        {
            public:
                static inline GLuint program = 0;
                static inline GLuint linesProgram = 0;

                static inline GLHelper::GL_Buffer cubeBuffer{};

                static ModelViewerShared &Get();
            private:
                ModelViewerShared() = default;
        };

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

        ModelAsset model{};

        GLHelper::GL_IndexedBuffer bboxBuffer{};

        GLuint staticCollisionVao = 0;
        GLuint staticCollisionVbo = 0;

        std::vector<GLHull> hulls{};

        std::vector<GLModelLod> lods{};

        float pitch = 0;
        float yaw = 0;
        float distance = 0;

        glm::mat4 projection{};
        glm::mat4 view{};

        GLsizei windowWidth = 800;
        GLsizei windowHeight = 600;
        float windowAspect = 4.0 / 3.0;

        GLHelper::GL_Framebuffer framebuffer;

        bool initDone = false;
        bool dragging = false;

        void DestroyModel();

        void RenderImGui();

        void RenderFramebuffer();

        void ResizeWindow(GLsizei width, GLsizei height);

        void ClampView();

        [[nodiscard]] ImTextureID GetFramebufferTexture() const;

        [[nodiscard]] ImVec2 GetFramebufferSize() const;

        void UpdateMatrix();

        static void LoadCube();

        void LoadBBox();

        void LoadHulls();

        void LoadStaticCollision();
};


#endif //GAME_SDK_MODELVIEWER_H
