//
// Created by droc101 on 9/7/25.
//

#pragma once
#include "../Viewport.h"
#include "EditorTool.h"
#include <cstddef>

class TestTool final : public EditorTool
{
    public:
        TestTool() = default;
        ~TestTool() override = default;

        void RenderViewport(Viewport &vp) override;

        void RenderToolWindow() override;

    private:
        bool draggingVertex = false;
        size_t sectorIndex = 0;
        size_t vertexIndex = 0;

};
