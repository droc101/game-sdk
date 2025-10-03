//
// Created by droc101 on 10/3/25.
//

#pragma once
#include "EditorTool.h"


class SelectTool final: public EditorTool
{
    public:
        SelectTool() = default;
        ~SelectTool() override = default;

        void RenderViewport(Viewport &vp) override;

        void RenderToolWindow() override;

    private:
        ItemType hoverType;
        size_t hoverIndex;
        ItemType selectionType;
        size_t selectionIndex;
};
