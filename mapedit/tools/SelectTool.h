//
// Created by droc101 on 10/3/25.
//

#pragma once

#include <cstddef>
#include <tuple>
#include <vector>
#include "../Viewport.h"
#include "EditorTool.h"

class SelectTool final: public EditorTool
{
    public:
        SelectTool() = default;
        ~SelectTool() override = default;

        void HandleDrag(const Viewport &vp, bool isHovered, glm::vec3 worldSpaceHover);

        std::vector<std::tuple<ItemType, size_t, float>> DetermineHoveredItem(const Viewport &vp,
                                                                              bool isHovered,
                                                                              const glm::vec3 &worldSpaceHover);

        void ProcessViewportSelectMode(const Viewport &vp, bool isHovered, const glm::vec3 &worldSpaceHover);

        void RenderViewport(Viewport &vp) override;

        void RenderToolWindow() override;

        [[nodiscard]] bool IsCopyableSelected() const;

        void Copy() const;

        void Cut();

        void Paste();

        [[nodiscard]] bool HasSelection() const;

        [[nodiscard]] glm::vec3 SelectionCenter() const;

    private:
        ItemType hoverType = ItemType::NONE;
        size_t hoverIndex = 0;

        ItemType selectionType = ItemType::NONE;
        size_t selectionIndex = 0;

        glm::vec2 brushDragMouseOffset{};

        bool draggingRotationGizmo = false;
        float rotationGizmoLastAngle = 0.0f;
        float rotationGizmoSelectionAngle = 0.0f;

        std::vector<std::tuple<ItemType, size_t, float>> menuHoveredItems{};

        bool dragging = false;

        static float WrapAndSnapAngle(float angle);
};
