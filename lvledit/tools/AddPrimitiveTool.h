//
// Created by droc101 on 9/21/25.
//

#pragma once
#include <array>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

#include "AddPolygonTool.h"
#include "EditorTool.h"


class AddPrimitiveTool final: public EditorTool
{
    public:
        AddPrimitiveTool() = default;
        ~AddPrimitiveTool() override = default;

        void RenderViewport(Viewport &vp) override;

        void RenderToolWindow() override;
    private:
        bool hasDrawnShape = false;
        bool isDragging = false;
        glm::vec3 shapeStart;
        glm::vec3 shapeEnd;
        int32_t sides = 4;
        float ceiling = 1;
        float floor = -1;

        static std::vector<glm::vec2> buildNgon(int n,
                                                const glm::vec2 &p0,
                                                const glm::vec2 &p1,
                                                float startAngleRadians = M_PI_2f);
};
