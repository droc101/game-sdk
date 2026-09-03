//
// Created by droc101 on 9/21/25.
//

#pragma once

#include <array>
#include <cstdint>
#include <libassets/type/Axis.h>
#include <vector>
#include "../Viewport.h"
#include "EditorTool.h"

class AddPrimitiveTool final: public EditorTool
{
    public:
        AddPrimitiveTool() = default;
        ~AddPrimitiveTool() override = default;

        void RenderViewport(Viewport &vp) override;

        void RenderToolWindow() override;

    private:
        enum class PrimitiveType : uint8_t
        {
            RECTANGLE,
            TRIANGLE,
            NGON,
        };

        static constexpr std::array<const char *, 3> PRIMITIVE_NAMES = {
            "Rectangle",
            "Triangle",
            "Ngon",
        };

        enum class DragMode : uint8_t
        {
            NOT_DRAGGING,
            DRAGGING_PRIMARY_BOX,
            DRAGGING_START_DEPTH,
            DRAGGING_END_DEPTH,
        };

        static inline bool hasDrawnShape = false;
        DragMode dragMode = DragMode::NOT_DRAGGING;
        static inline glm::vec2 shapeStart{};
        static inline glm::vec2 shapeEnd{};
        static inline float startDepth = 16;
        static inline float endDepth = -16;
        static inline Axis axis = Axis::Y;

        static inline int32_t ngonSides = 16;
        static inline float ngonStartAngle = 0;
        static inline PrimitiveType primitive = PrimitiveType::RECTANGLE;

        [[nodiscard]] std::vector<glm::vec2> GetPoints() const;

        static std::vector<glm::vec2> BuildNgon(int n,
                                                const glm::vec2 &p0,
                                                const glm::vec2 &p1,
                                                float startAngleRadians = 1.57079632679489661923f); // PI/2

        static std::vector<glm::vec2> BuildRect(const glm::vec2 &p0, const glm::vec2 &p1);

        static std::vector<glm::vec2> BuildTriangle(const glm::vec2 &p0, const glm::vec2 &p1);

        void AddBrush();
};
