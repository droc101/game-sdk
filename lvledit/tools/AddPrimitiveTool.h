//
// Created by droc101 on 9/21/25.
//

#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
#include <libassets/type/WallMaterial.h>
#include "../Viewport.h"
#include "EditorTool.h"
#include "Options.h"

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
            NGON
        };

        static constexpr std::array<const char *, 3> PRIMITIVE_NAMES = {
            "Rectangle",
            "Triangle",
            "Ngon",
        };

        bool hasDrawnShape = false;
        bool isDragging = false;
        glm::vec3 shapeStart{};
        glm::vec3 shapeEnd{};
        float ceiling = 1;
        float floor = -1;

        static inline int32_t ngonSides = 16;
        static inline float ngonStartAngle = 0;
        static inline PrimitiveType primitive = PrimitiveType::RECTANGLE;

        static std::vector<glm::vec2> buildNgon(int n,
                                                const glm::vec2 &p0,
                                                const glm::vec2 &p1,
                                                float startAngleRadians = 1.57079632679489661923f); // PI/2

        static std::vector<glm::vec2> buildRect(const glm::vec2 &p0, const glm::vec2 &p1);

        static std::vector<glm::vec2> buildTri(const glm::vec2 &p0, const glm::vec2 &p1);
};
