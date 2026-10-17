//
// Created by droc101 on 10/16/26.
//

#pragma once
#include <cstddef>
#include "../Viewport.h"
#include "EditorTool.h"

class FaceTool: public EditorTool
{
    public:
        void RenderViewport(Viewport &vp) override;
        void RenderToolWindow() override;

    private:
        bool hasSelection = false;
        size_t brushIndex = 0;
        size_t faceIndex = 0;
};
