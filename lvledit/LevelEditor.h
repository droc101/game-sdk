//
// Created by droc101 on 9/6/25.
//

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <libassets/asset/LevelAsset.h>
#include <memory>
#include "tools/EditorTool.h"
#include "tools/SelectTool.h"

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
        };

        static inline EditorToolType toolType = EditorToolType::SELECT;

        static inline LevelAsset level{};

        static inline int gridSpacingIndex = 2;
        static inline bool drawGrid = true;
        static inline bool drawAxisHelper = true;
        static inline bool drawWorldBorder = true;
        static inline bool drawViewportInfo = true;
        static inline bool snapToGrid = true;
        static inline bool showSidebar = true;

        static constexpr size_t HOVER_DISTANCE_PIXELS = 10;
        static constexpr size_t TOOLBAR_HEIGHT = 48;
        static constexpr size_t SIDEBAR_WIDTH = 300;
        static constexpr std::array<float, 9> GRID_SPACING_VALUES = {0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0, 32.0, 64.0};
        static constexpr float LEVEL_HALF_SIZE = 512;
        static constexpr float LEVEL_SIZE = LEVEL_HALF_SIZE * 2;

        static constexpr const char *SELECT_ICON_NAME = "editor/icon_select";
        static constexpr const char *PRIMITIVE_ICON_NAME = "editor/icon_primitive";
        static constexpr const char *POLYGON_ICON_NAME = "editor/icon_polygon";

        static inline std::unique_ptr<EditorTool> tool = std::unique_ptr<EditorTool>(new SelectTool());

        static inline WallMaterial mat{};

        [[nodiscard]] static float SnapToGrid(float f);

        [[nodiscard]] static glm::vec3 SnapToGrid(glm::vec3 v);

        [[nodiscard]] static glm::vec2 SnapToGrid(glm::vec2 v);

        [[nodiscard]] static bool IsPointInBounds(glm::vec3 p);

        [[nodiscard]] static float VecDistanceToLine2D(glm::vec2 lineStart, glm::vec2 lineEnd, glm::vec2 testPoint);

        static std::array<float, 4> CalculateBBox(const std::vector<glm::vec2> &points);

        static std::array<float, 4> CalculateBBox(const std::vector<std::array<float, 2>> &points);

        static void MaterialToolWindow(WallMaterial &wallMat);
};
