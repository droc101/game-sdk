//
// Created by droc101 on 4/26/26.
//

#pragma once

#include <cstddef>
#include <glm/vec3.hpp>
#include <libassets/type/Color.h>
#include <libassets/type/Sector.h>
#include <string>
#include <vector>
#include "tools/EditorTool.h"
#include "Viewport.h"

class ViewportRenderer
{
    public:

        /// A new primitive, rendered as a closed polygon
        struct ViewportRenderNewPrimitive
        {
                /// The points in the new primitive
                std::vector<glm::vec2> points;
                /// The floor height of the new primitive
                float floor;
                /// The ceiling height of the new primitive
                float ceiling;
                /// The AABB start of the new primitive
                glm::vec3 aabbStart;
                /// The AABB end of the new primitive
                glm::vec3 aabbEnd;
        };

        /// A new polygon, rendered without the final line connecting the first and last points.
        /// An additional line will be drawn between the final point and the mouse position
        struct ViewportRenderNewPolygon
        {
                /// The points in the polygon
                std::vector<glm::vec2> points;
                /// The floor height of the new polygon
                float floor;
                /// The ceiling height of the new polygon
                float ceiling;
        };

        /// A new actor, rendered according to the class's definition with default params
        struct ViewportRenderNewActor
        {
                /// The class name of the new actor
                std::string className;
                /// The new actor's position
                glm::vec3 position;
                /// The new actor's rotation
                glm::vec3 rotation;
        };

        /// A single point in 3D space
        struct ViewportRenderPoint
        {
                /// The position of the point
                glm::vec3 pos;
                /// The color of the point
                Color color;
                /// The size (in pixels) of the point
                float size;
        };

        struct ViewportRenderSettings
        {
                /// Whether a sector is focused for editing
                bool sectorFocusMode = false;
                /// The index of the sector that is focused
                size_t focusedSectorIndex = 0;

                /// The type of element that is hovered
                EditorTool::ItemType hoverType = EditorTool::ItemType::NONE;
                /// The index of the hovered element
                size_t hoverIndex = 0;

                /// The type of element that is selected
                EditorTool::ItemType selectionType = EditorTool::ItemType::NONE;
                /// The index of the selected element, or the sector index for LINE and VERTEX
                size_t selectionIndex = 0;
                /// The index of the selected vertex/line
                size_t selectionVertexIndex = 0;

                ViewportRenderPoint *point = nullptr;
                ViewportRenderNewPrimitive *newPrimitive = nullptr;
                ViewportRenderNewActor *newActor = nullptr;
                ViewportRenderNewPolygon *newPolygon = nullptr;
        };

        ViewportRenderer() = delete;

        /**
         * Render a viewport with the given settings
         * @param vp The viewport to render
         * @param settings The settings to render with
         */
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

        static bool SectorIsCulled(const Sector &sector, const Viewport &vp);
};
