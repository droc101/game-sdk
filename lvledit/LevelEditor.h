//
// Created by droc101 on 9/6/25.
//

#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include "libassets/asset/LevelAsset.h"
#include "tools/EditorTool.h"
#include "tools/VertexTool.h"


class LevelEditor
{
    public:
        LevelEditor() = delete;

        enum class EditorToolType : uint8_t
        {
            SELECT,
            ADD_PRIMITIVE,
            ADD_POLYGON,
            ADD_ACTOR,
            EDIT_SECTOR
        };

        static inline EditorToolType toolType = EditorToolType::EDIT_SECTOR;

        static inline LevelAsset level{};

        static inline int gridSpacingIndex = 2;
        static inline bool drawGrid = true;
        static inline bool drawAxisHelper = true;
        static inline bool drawWorldBorder = true;
        static inline bool drawViewportInfo = true;
        static inline bool snapToGrid = true;
        static inline bool showSidebar = true;

        static constexpr size_t HOVER_DISTANCE_PIXELS = 5;
        static constexpr std::array<float, 9> GRID_SPACING_VALUES = {0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0, 32.0, 64.0};
        static constexpr float LEVEL_HALF_SIZE = 512;
        static constexpr float LEVEL_SIZE = LEVEL_HALF_SIZE * 2;

        static inline std::unique_ptr<EditorTool> tool = std::unique_ptr<EditorTool>(new VertexTool());

        static inline std::string texture = "";

        [[nodiscard]] static float SnapToGrid(float f);

        [[nodiscard]] static glm::vec3 SnapToGrid(glm::vec3 v);

        [[nodiscard]] static glm::vec2 SnapToGrid(glm::vec2 v);

        [[nodiscard]] static bool IsPointInBounds(glm::vec3 p);

        [[nodiscard]] static float VecDistanceToLine2D(glm::vec2 lineStart, glm::vec2 lineEnd, glm::vec2 testPoint);

        static std::array<float, 4> CalculateBBox(const std::vector<glm::vec2> &points);

        static std::array<float, 4> CalculateBBox(const std::vector<std::array<float, 2>> &points);
};
