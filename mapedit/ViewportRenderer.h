//
// Created by droc101 on 4/26/26.
//

#pragma once

#include <cstddef>
#include <glm/vec3.hpp>
#include <libassets/type/Color.h>
#include <string>
#include <vector>
#include "tools/EditorTool.h"
#include "Viewport.h"

class ViewportRenderer
{
    public:

        struct ViewportRenderNewPrimitive
        {
                std::vector<glm::vec2> points;
                float floor;
                float ceiling;
                glm::vec3 aabbStart;
                glm::vec3 aabbEnd;
        };

        struct ViewportRenderNewPolygon
        {
                std::vector<glm::vec2> points;
                float floor;
                float ceiling;
        };

        struct ViewportRenderNewActor
        {
                std::string className;
                glm::vec3 position;
                glm::vec3 rotation;
        };

        struct ViewportRenderPoint
        {
                glm::vec3 pos;
                Color color;
                float size;
        };

        struct ViewportRenderSettings
        {
                bool sectorFocusMode = false;
                size_t focusedSectorIndex = 0;

                EditorTool::ItemType hoverType = EditorTool::ItemType::NONE;
                size_t hoverIndex = 0;

                EditorTool::ItemType selectionType = EditorTool::ItemType::NONE;
                size_t selectionIndex = 0;
                size_t selectionVertexIndex = 0;

                ViewportRenderPoint *point = nullptr;
                ViewportRenderNewPrimitive *newPrimitive = nullptr;
                ViewportRenderNewActor *newActor = nullptr;
                ViewportRenderNewPolygon *newPolygon = nullptr;
        };

        ViewportRenderer() = delete;

        static void RenderViewport(Viewport &vp, const ViewportRenderSettings &settings);

    private:
        static void RenderSector(const Viewport &vp,
                                 const ViewportRenderSettings &settings,
                                 size_t sectorIndex,
                                 glm::mat4 &matrix);
        static void RenderPoint(const ViewportRenderPoint *point, glm::mat4 &matrix);
        static void RenderNewPrimitive(Viewport &vp, const ViewportRenderNewPrimitive *prim, glm::mat4 &matrix);
        static void RenderNewActor(Viewport &vp, const ViewportRenderNewActor *actor, glm::mat4 &matrix);
        static void RenderNewPolygon(const Viewport &vp, const ViewportRenderNewPolygon *poly, glm::mat4 &matrix);
};
