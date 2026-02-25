//
// Created by droc101 on 9/19/25.
//

#pragma once

#include <vector>
#include "../Viewport.h"
#include "EditorTool.h"

class AddPolygonTool final: public EditorTool
{
    public:
        AddPolygonTool() = default;
        ~AddPolygonTool() override = default;

        void RenderViewport(Viewport &vp) override;

        void RenderToolWindow() override;

    private:
        bool isDrawing = false;
        std::vector<glm::vec2> points{};
        float floor = 1.0f;
        float ceiling = -1.0f;
};
