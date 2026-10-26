//
// Created by droc101 on 4/26/26.
//

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/vec3.hpp>
#include <libassets/type/Actor.h>
#include <libassets/type/Axis.h>
#include <libassets/type/BoundingBox.h>
#include <libassets/type/Brush.h>
#include <libassets/type/Color.h>
#include <libassets/type/renderDefs/BoxRenderDefinition.h>
#include <libassets/type/renderDefs/CircleRenderDefinition.h>
#include <libassets/type/renderDefs/ConeRenderDefinition.h>
#include <libassets/type/renderDefs/ModelRenderDefinition.h>
#include <libassets/type/renderDefs/OrientationRenderDefinition.h>
#include <libassets/type/renderDefs/PointRenderDefinition.h>
#include <libassets/type/renderDefs/SpriteRenderDefinition.h>
#include <libassets/type/renderDefs/WallRenderDefinition.h>
#include <string>
#include <utility>
#include <vector>
#include "tools/EditorTool.h"

class Viewport;

class ViewportRenderer
{
    public:

        /// A new primitive, rendered as a closed polygon
        struct ViewportRenderNewPrimitive
        {
                /// The points in the new primitive
                std::vector<glm::vec2> points;
                float startDepth;
                float endDepth;
                /// The AABB start of the new primitive
                glm::vec2 aabbStart;
                /// The AABB end of the new primitive
                glm::vec2 aabbEnd;
                Axis axis;
        };

        /// A new polygon, rendered without the final line connecting the first and last points.
        /// An additional line will be drawn between the final point and the mouse position
        struct ViewportRenderNewPolygon
        {
                /// The points in the polygon
                std::vector<glm::vec2> points;
                float startDepth;
                float endDepth;
                Axis axis;
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

        struct ViewportRenderRotationGizmo
        {
                glm::vec3 position;
                float radiusInPx;
        };

        class ViewportRenderScaleGizmo
        {
            public:
                enum class Handle : uint8_t
                {
                    NONE,
                    TOP_LEFT,
                    TOP_RIGHT,
                    BOTTOM_LEFT,
                    BOTTOM_RIGHT,
                    TOP_CENTER,
                    BOTTOM_CENTER,
                    LEFT_CENTER,
                    RIGHT_CENTER,
                };

                ViewportRenderScaleGizmo() = delete;
                ViewportRenderScaleGizmo(Viewport &viewport, const Brush &brush);

                void Render(const glm::mat4 &matrix) const;

                [[nodiscard]] Handle HoveredHandle() const;

                void ScaleVertex(glm::vec3 &vertex,
                                 const BoundingBox &startBounds,
                                 const glm::vec3 &worldDrag,
                                 Handle draggedHandle) const;

            private:
                static constexpr float POINT_SIZE = 10;
                static constexpr float LINE_THICKNESS = 2;

                static constexpr std::array<std::pair<uint32_t, uint32_t>, 8> LINES = {
                    std::pair{0, 1},
                    std::pair{0, 2},
                    std::pair{2, 3},
                    std::pair{0, 4},
                    std::pair{1, 5},
                    std::pair{4, 5},
                    std::pair{2, 6},
                    std::pair{5, 7},
                };

                [[nodiscard]] Handle GetHandleFromBoundingBoxPointIndex(uint32_t index) const;

                [[nodiscard]] Handle GetHandleFromLineIndex(uint32_t index) const;

                /**
                 * Get which side of the bounding box a handle is on, per axis
                 * @return -1 for the min side, 1 for the max side, or 0 if the handle doesn't scale that axis
                 */
                [[nodiscard]] glm::vec3 HandleDirection(Handle draggedHandle) const;

                Viewport &viewport;
                BoundingBox boundingBox;
        };

        struct ViewportRenderSettings
        {
                /// Whether a brush is focused for editing
                bool brushFocusMode = false;
                /// The index of the brush that is focused
                size_t focusedBrushIndex = 0;

                /// The type of element that is hovered
                EditorTool::ItemType hoverType = EditorTool::ItemType::NONE;
                /// The index of the hovered element
                size_t hoverIndex = 0;
                /// The index of the hovered vertex/line/face
                size_t hoverSubIndex = 0;

                /// The type of element that is selected
                EditorTool::ItemType selectionType = EditorTool::ItemType::NONE;
                /// The index of the selected element, or the brush index for LINE and VERTEX
                size_t selectionIndex = 0;
                /// The index of the selected vertex/line/face
                size_t selectionSubIndex = 0;

                ViewportRenderPoint *point = nullptr;
                ViewportRenderNewPrimitive *newPrimitive = nullptr;
                ViewportRenderNewActor *newActor = nullptr;
                ViewportRenderNewPolygon *newPolygon = nullptr;
                ViewportRenderRotationGizmo *rotationGizmo = nullptr;
                ViewportRenderScaleGizmo *scaleGizmo = nullptr;
        };

        ViewportRenderer() = delete;

        /**
         * Render a viewport with the given settings
         * @param vp The viewport to render
         * @param settings The settings to render with
         */
        static void RenderViewport(Viewport &vp, const ViewportRenderSettings &settings);

    private:
        static void RenderBrush(const Viewport &vp,
                                const ViewportRenderSettings &settings,
                                size_t brushIndex,
                                const glm::mat4 &matrix);
        static void RenderPoint(const ViewportRenderPoint *point, const glm::mat4 &matrix);
        static void RenderNewPrimitive(Viewport &vp, const ViewportRenderNewPrimitive *prim, glm::mat4 &matrix);
        static void RenderNewActor(const Viewport &vp, const ViewportRenderNewActor *actor, const glm::mat4 &matrix);
        static void RenderNewPolygon(const Viewport &vp, const ViewportRenderNewPolygon *poly, const glm::mat4 &matrix);
        static void RenderRotationGizmo(const Viewport &vp,
                                        const ViewportRenderRotationGizmo *rotationGizmo,
                                        const glm::mat4 &matrix);

        static bool BrushIsCulled(const Brush &brush, const Viewport &vp);

        static void RenderActor(const Actor &a, const glm::mat4 &matrix, const Viewport &vp, const Color &overlayColor);

        static bool ActorIsCulled(const Actor &actor, const Viewport &vp);

        static void RenderBoxRdef(BoxRenderDefinition *rdef,
                                  const Actor &actor,
                                  const glm::mat4 &worldMatrix,
                                  const glm::mat4 &matrix);

        static void RenderModelRdef(ModelRenderDefinition *rdef,
                                    const Actor &actor,
                                    const glm::mat4 &worldMatrix,
                                    const glm::mat4 &matrix,
                                    bool textured,
                                    const Color &overlayColor);

        static void RenderOrientationRdef(OrientationRenderDefinition *rdef,
                                          const Actor &actor,
                                          const glm::mat4 &matrix,
                                          const Viewport &vp);

        static void RenderPointRdef(PointRenderDefinition *rdef, const Actor &actor, const glm::mat4 &matrix);

        static void RenderSpriteRdef(SpriteRenderDefinition *rdef, const Actor &actor, const glm::mat4 &matrix);

        static void RenderWallRdef(WallRenderDefinition *rdef,
                                   const Actor &actor,
                                   const glm::mat4 &worldMatrix,
                                   const glm::mat4 &matrix,
                                   bool textured,
                                   const Color &overlayColor);

        // TODO: This only needs the viewport type, but because of the hacks required to make the scale gizmo work it's taking an entire viewport reference
        static void RenderCircleRdef(CircleRenderDefinition *rdef,
                                     const Actor &actor,
                                     const glm::mat4 &worldMatrix,
                                     const Viewport &vp);

        static void RenderConeRdef(ConeRenderDefinition *rdef,
                                   const Actor &actor,
                                   const glm::mat4 &matrix,
                                   const glm::mat4 &worldMatrix);
};
