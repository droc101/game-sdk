//
// Created by droc101 on 9/6/25.
//

#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include "libassets/asset/LevelAsset.h"
#include "tools/TestTool.h"
#include "tools/EditorTool.h"


class LevelEditor
{
    public:
        LevelEditor() = delete;

        enum class EditorToolType: uint8_t
        {
            SELECT,
            ADD_SECTOR,
            ADD_ACTOR,
            EDIT_SECTOR
        };

        inline static EditorToolType toolType = EditorToolType::SELECT;

        inline static LevelAsset level{};

        inline static int gridSpacingIndex = 2;
        inline static bool drawGrid = true;
        inline static bool drawAxisHelper = true;
        inline static bool drawWorldBorder = true;
        inline static bool drawViewportInfo = true;
        inline static bool snapToGrid = true;

        static constexpr std::array<float, 9> gridSpacingValues = {
            0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0, 32.0, 64.0
        };

        inline static std::unique_ptr<EditorTool> tool = std::unique_ptr<EditorTool>(new TestTool());

        static float SnapToGrid(float f);

        [[nodiscard]] static glm::vec3 SnapToGrid(glm::vec3 v);

        [[nodiscard]] static bool IsPointInBounds(glm::vec3 p);

        /// NOT IMPLEMENTED
        [[nodiscard]] static float VecDistanceToLine3D(glm::vec3 lineStart, glm::vec3 lineEnd, glm::vec3 testPoint);

        [[nodiscard]] static float VecDistanceToLine2D(glm::vec2 lineStart, glm::vec2 lineEnd, glm::vec2 testPoint);
};
