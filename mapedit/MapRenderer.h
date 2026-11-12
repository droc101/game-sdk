//
// Created by droc101 on 9/5/25.
//

#pragma once

#include <cstdint>
#include <game_sdk/gl/GLHelper.h>
#include <GL/glew.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <libassets/asset/ModelAsset.h>
#include <libassets/type/Brush.h>
#include <libassets/type/Color.h>
#include <string>
#include <unordered_map>
#include <vector>
#include "Viewport.h"

class MapRenderer
{
    public:
        struct Wall
        {
                Color color;
                bool zAxisOrientation;

                glm::vec2 localCenter;
                glm::vec2 size;

                std::string texture;

                glm::vec2 uvOffset;
                glm::vec2 uvScale;

                bool unshaded;
        };

        struct ObjectBufferInfo
        {
                uint32_t objectType;
                uint32_t objectIndex;
                uint32_t objectSubIndex;
        };

        MapRenderer() = delete;

        static bool Init();
        static void Destroy();

        static void RenderViewportGrid(const Viewport &vp);

        static void CmdLine(glm::vec3 start, glm::vec3 end, Color color, float thickness);

        static void CmdPoint(glm::vec3 position, float pointSize, Color color);

        static void CmdSprite(glm::vec3 position, float pointSize, const std::string &texture, Color color);

        static void CmdUnitVector(glm::vec3 origin, glm::vec3 eulerAngles, Color color, float thickness, float length);

        static void CmdModel(std::string model, const glm::mat4 &worldMatrix, const Color &modColor);

        static void CmdTexturedModel(std::string model,
                                     const glm::mat4 &worldMatrix,
                                     uint32_t skin,
                                     const Color &modColor,
                                     const Color &overlayColor);

        static void CmdBrushFace(const Brush::TriangulatedFace &face,
                                 const glm::mat4 &worldMatrix,
                                 const Color &overlayColor);

        static void CmdWall(const glm::mat4 &worldMatrix, const Wall &wall, const Color &overlayColor);

        static const ModelAsset &GetModel(std::string model);

        static void ExecuteCommands(const glm::mat4 &viewMatrix);

        static void CmdObjectBufferPoint(glm::vec3 position, float pointSize, ObjectBufferInfo info);

        static void CmdObjectBufferWall(const glm::mat4 &worldMatrix, const Wall &wall, ObjectBufferInfo info);

        static void CmdObjectBufferBrushFace(const Brush::TriangulatedFace &face,
                                             const glm::mat4 &worldMatrix,
                                             ObjectBufferInfo info);

        static void CmdObjectBufferModel(std::string model, const glm::mat4 &worldMatrix, ObjectBufferInfo info);

        static void ExecuteObjectBufferCommands(const glm::mat4 &viewMatrix);

    private:
        static constexpr uint32_t WALL_VERTEX_COUNT = 8;
        static constexpr uint32_t WALL_INDEX_COUNT = 12;

        struct ModelBuffer
        {
                ModelAsset model;
                GLuint vao;
                GLuint vbo;
                std::vector<GLuint> ebos;
        };

        struct WallVertex
        {
                glm::vec3 position;
                glm::vec2 uv;
                glm::vec3 normal;
                glm::vec4 color{1};
        };

        struct ObjectBufferWallVertex
        {
                glm::vec3 position;
                glm::vec3 normal;
        };

        struct LineCommand
        {
                glm::vec3 start;
                glm::vec3 end;
                float width;
                Color color;
        };

        struct PointCommand
        {
                glm::vec3 position;
                float size;
                Color color;
        };

        struct WallCommand
        {
                Wall wall;
                Color overlayColor;
                glm::mat4 worldMatrix;
        };

        struct ModelCommand
        {
                ModelBuffer &buffer;
                glm::mat4 worldMatrix;
                Color color;
        };

        struct TexturedModelCommand
        {
                ModelBuffer &buffer;
                glm::mat4 worldMatrix;
                uint32_t skinIndex;
                uint32_t lodIndex;
                Color modColor;
                Color overlayColor;
        };

        struct SpriteCommand
        {
                glm::vec3 position;
                float pointSize;
                std::string texture;
                Color color;
        };

        struct BrushFaceCommand
        {
                Brush::TriangulatedFace face;
                glm::mat4 worldMatrix;
                Color overlayColor;
        };

        static inline std::vector<LineCommand> lineCommands{};
        static inline std::vector<PointCommand> pointCommands{};
        static inline std::vector<WallCommand> wallCommands{};
        static inline std::vector<TexturedModelCommand> texturedModelCommands{};
        static inline std::vector<ModelCommand> modelCommands{};
        static inline std::vector<SpriteCommand> spriteCommands{};
        static inline std::vector<BrushFaceCommand> brushFaceCommands{};

        struct ObjectBufferPointCommand
        {
                glm::vec3 position;
                float pointSize;
                ObjectBufferInfo info;
        };

        struct ObjectBufferWallCommand
        {
                glm::mat4 worldMatrix;
                Wall wall;
                ObjectBufferInfo info;
        };

        struct ObjectBufferBrushFaceCommand
        {
                Brush::TriangulatedFace face;
                glm::mat4 worldMatrix;
                ObjectBufferInfo info;
        };

        struct ObjectBufferModelCommand
        {
                ModelBuffer &buffer;
                glm::mat4 worldMatrix;
                ObjectBufferInfo info;
                uint32_t lodIndex;
        };

        static inline std::vector<ObjectBufferPointCommand> objectBufferPointCommands{};
        static inline std::vector<ObjectBufferWallCommand> objectBufferWallCommands{};
        static inline std::vector<ObjectBufferBrushFaceCommand> objectBufferBrushFaceCommands{};
        static inline std::vector<ObjectBufferModelCommand> objectBufferModelCommands{};

        static inline GLuint uniformColorProgram = 0;
        static inline GLuint vertexColorProgram = 0;
        static inline GLuint gridProgram = 0;
        static inline GLuint spriteProgram = 0;
        static inline GLuint shadedModelProgram = 0;
        static inline GLuint brushProgram = 0;
        static inline GLuint worldBuffersProgram = 0;

        static inline GLHelper::GL_Buffer axisHelperBuffer{};
        static inline GLHelper::GL_Buffer worldBorderBuffer{};

        static inline GLHelper::GL_IndexedBuffer workBuffer{};
        static inline GLHelper::GL_Buffer workBufferNonIndexed{};

        static inline std::unordered_map<std::string, ModelBuffer> modelBuffers{};

        static inline glm::mat4 identity = glm::identity<glm::mat4>();

        static ModelBuffer LoadModel(const std::string &path);
};
