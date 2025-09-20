//
// Created by droc101 on 9/7/25.
//

#pragma once
#include "../Viewport.h"


class EditorTool
{
    public:
        EditorTool() = default;
        virtual ~EditorTool() = default;

        virtual void RenderViewport(Viewport &vp) = 0;

        virtual void RenderToolWindow() = 0;
};
