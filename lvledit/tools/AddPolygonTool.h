//
// Created by droc101 on 9/19/25.
//

#pragma once

#include <cstddef>
#include <cstdint>
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
};
