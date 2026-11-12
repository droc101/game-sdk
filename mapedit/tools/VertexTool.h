//
// Created by droc101 on 9/3/26.
//

#pragma once

#include "../Viewport.h"
#include "EditorTool.h"

class VertexTool final: public EditorTool
{
    public:
        void RenderViewport(Viewport &vp) override;
        void RenderToolWindow() override;
};
