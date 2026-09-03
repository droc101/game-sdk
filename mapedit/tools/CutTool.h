//
// Created by droc101 on 9/2/26.
//

#pragma once
#include "EditorTool.h"


class CutTool final: public EditorTool
{
    public:
        void RenderViewport(Viewport &vp) override;
        void RenderToolWindow() override;
};
