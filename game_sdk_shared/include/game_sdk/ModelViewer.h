//
// Created by droc101 on 2/17/26.
//

#ifndef GAME_SDK_MODELVIEWER_H
#define GAME_SDK_MODELVIEWER_H

#include <game_sdk/Flycam.h>
#include <game_sdk/gl/GLHelper.h>
#include <GL/glew.h>
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

        /**
         * Initialize shared model viewer resources
         */
        [[nodiscard]] static bool GlobalInit();

        /**
         * Destroy shared model viewer resources
         */
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

        /**
         * Reload the current model
         */
        void ReloadModel();

        /**
         * Get the model
         */
        [[nodiscard]] ModelAsset &GetModel();

        void ResetView();


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

        Color backgroundColor = Color(0x191919ff);

    private:
        class ModelViewerShared
        {
            public:
                GLuint program = 0;
                GLuint linesProgram = 0;

                GLHelper::GL_Buffer cubeBuffer{};

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

        Flycam camera{};

        GLsizei windowWidth = 800;
        GLsizei windowHeight = 600;

        GLHelper::GL_Framebuffer framebuffer;

        bool initDone = false;
        bool dragging = false;

        void DestroyModel();

        void RenderImGui();

        void RenderFramebuffer();

        void ResizeWindow(GLsizei width, GLsizei height);

        [[nodiscard]] ImTextureID GetFramebufferTexture() const;

        [[nodiscard]] ImVec2 GetFramebufferSize() const;

        static void LoadCube();

        void LoadBBox();

        void LoadHulls();

        void LoadStaticCollision();
};


#endif //GAME_SDK_MODELVIEWER_H
