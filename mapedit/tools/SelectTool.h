//
// Created by droc101 on 10/3/25.
//

#pragma once

#include <cstddef>
#include <glm/gtc/quaternion.hpp>
#include <libassets/type/BoundingBox.h>
#include <tuple>
#include <vector>
#include "../Viewport.h"
#include "../ViewportRenderer.h"
#include "EditorTool.h"

class SelectTool final: public EditorTool
{
    public:
        SelectTool() = default;
        ~SelectTool() override = default;

        SelectTool(ItemType selectionType, size_t selectionIndex);

        void HandleDrag(const Viewport &vp, glm::vec3 worldSpaceHover);

        void DetermineHoveredItem(Viewport &vp, const glm::vec3 &worldSpaceHover);

        void ProcessViewportSelectMode(Viewport &vp, const glm::vec3 &worldSpaceHover);

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

        glm::vec3 brushDragStart{};
        glm::vec3 brushDragStartOrigin{};

        bool draggingRotationGizmo = false;
        glm::quat rotationGizmoStart{1.0f, 0.0f, 0.0f, 0.0f};
        glm::vec2 rotationGizmoLastMouseDirection{};
        float rotationGizmoAngle = 0.0f;

        bool draggingResizeGizmo = false;
        glm::vec3 resizeGizmoDragStart{};
        BoundingBox resizeGizmoStartBounds{};
        std::vector<glm::vec3> resizeGizmoStartVertices{};
        ResizeGizmoHelper::Handle resizeGizmoDraggedHandle{};

        std::vector<std::tuple<ItemType, size_t, float>> menuHoveredItems{};

        bool dragging = false;
};
