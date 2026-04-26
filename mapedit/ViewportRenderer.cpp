//
// Created by droc101 on 4/26/26.
//

#include "ViewportRenderer.h"
#include <array>
#include <cstddef>
#include <libassets/type/Actor.h>
#include <libassets/type/ActorDefinition.h>
#include <libassets/type/Color.h>
#include <libassets/type/Sector.h>
#include "MapEditor.h"
#include "MapRenderer.h"
#include "tools/EditorTool.h"
#include "Viewport.h"

void ViewportRenderer::RenderViewport(Viewport &vp, const ViewportRenderSettings &settings)
{
    MapRenderer::RenderViewport(vp);

    glm::mat4 matrix = vp.GetMatrix();

    for (const Actor &actor: MapEditor::map.actors)
    {
        MapRenderer::RenderActor(actor, matrix, vp);
    }

    for (size_t sectorIndex = 0; sectorIndex < MapEditor::map.sectors.size(); sectorIndex++)
    {
        RenderSector(vp, settings, sectorIndex, matrix);
    }

    if (settings.point != nullptr)
    {
        RenderPoint(settings.point, matrix);
    }

    if (settings.newActor != nullptr)
    {
        RenderNewActor(vp, settings.newActor, matrix);
    }

    if (settings.newPrimitive != nullptr)
    {
        RenderNewPrimitive(vp, settings.newPrimitive, matrix);
    }

    if (settings.newPolygon != nullptr)
    {
        RenderNewPolygon(vp, settings.newPolygon, matrix);
    }
}

void ViewportRenderer::RenderSector(const Viewport &vp,
                                    const ViewportRenderSettings &settings,
                                    const size_t sectorIndex,
                                    glm::mat4 &matrix)
{
    const Sector &sector = MapEditor::map.sectors.at(sectorIndex);
    const bool isFocusedSector = settings.sectorFocusMode && settings.focusedSectorIndex == sectorIndex;

    Color c = Color(0.6, 0.6, 0.6, 1);
    if ((settings.selectionType == EditorTool::ItemType::SECTOR && settings.selectionIndex == sectorIndex) ||
        isFocusedSector)
    {
        c = Color(1, 1, 1, 1);
    } else if (settings.hoverType == EditorTool::ItemType::SECTOR && settings.hoverIndex == sectorIndex)
    {
        c = Color(.8, .8, .8, 1);
    }

    for (size_t vertexIndex = 0; vertexIndex < sector.points.size(); vertexIndex++)
    {
        const glm::vec2 &start2 = sector.points.at(vertexIndex);
        const glm::vec2 &end2 = sector.points.at((vertexIndex + 1) % sector.points.size());
        const glm::vec3 startCeiling = glm::vec3(start2.x, sector.ceilingHeight, start2.y);
        const glm::vec3 endCeiling = glm::vec3(end2.x, sector.ceilingHeight, end2.y);
        const glm::vec3 startFloor = glm::vec3(start2.x, sector.floorHeight, start2.y);
        const glm::vec3 endFloor = glm::vec3(end2.x, sector.floorHeight, end2.y);

        if (vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ)
        {
            float pointSize = 6;
            Color pointColor = Color(1, 0.7, 0.7, 1);
            if (isFocusedSector)
            {
                pointSize = 10;
                pointColor = Color(1, 0, 0, 1);
            }
            MapRenderer::RenderBillboardPoint(startCeiling + glm::vec3(0, 0.1, 0), pointSize, pointColor, matrix);
            if (settings.selectionType == EditorTool::ItemType::VERTEX &&
                settings.selectionVertexIndex == vertexIndex &&
                settings.selectionIndex == sectorIndex &&
                isFocusedSector)
            {
                MapRenderer::RenderBillboardPoint(startCeiling + glm::vec3(0, 0.05, 0), 12, Color(0, 1, 1, 1), matrix);
            }
        } else
        {
            MapRenderer::RenderLine(startFloor, endFloor, c, matrix, 4);
            MapRenderer::RenderLine(startCeiling, startFloor, c, matrix, 4);
            if (isFocusedSector &&
                settings.selectionType == EditorTool::ItemType::LINE &&
                settings.selectionVertexIndex == vertexIndex)
            {
                MapRenderer::RenderLine(startFloor, endFloor, Color(1, 0, 1, 1), matrix, 8);
            }
        }

        MapRenderer::RenderLine(startCeiling, endCeiling, c, matrix, 4);
        if (isFocusedSector &&
            settings.selectionType == EditorTool::ItemType::LINE &&
            settings.selectionVertexIndex == vertexIndex)
        {
            MapRenderer::RenderLine(startCeiling, endCeiling, Color(1, 0, 1, 1), matrix, 8);
        }
    }
}

void ViewportRenderer::RenderPoint(const ViewportRenderPoint *point, glm::mat4 &matrix)
{
    MapRenderer::RenderBillboardPoint(point->pos, point->size, point->color, matrix);
}

void ViewportRenderer::RenderNewActor(Viewport &vp, const ViewportRenderNewActor *actor, glm::mat4 &matrix)
{
    const ActorDefinition &newActorDef = MapEditor::adm.GetActorDefinition(actor->className);
    Actor tempActor{};
    tempActor.ApplyDefinition(newActorDef, true);
    tempActor.position = actor->position;
    tempActor.rotation = actor->rotation;
    tempActor.className = actor->className;
    MapRenderer::RenderActor(tempActor, matrix, vp);
}

void ViewportRenderer::RenderNewPolygon(const Viewport &vp, const ViewportRenderNewPolygon *poly, glm::mat4 &matrix)
{
    for (size_t vertexIndex = 0; vertexIndex < poly->points.size(); vertexIndex++)
    {
        const glm::vec2 &start2 = poly->points.at(vertexIndex);
        glm::vec2 end2 = poly->points.at((vertexIndex + 1) % poly->points.size());
        if (vertexIndex == poly->points.size() - 1)
        {
            const glm::vec3 worldSpaceHover = vp.GetWorldSpaceMousePos();
            end2 = MapEditor::SnapToGrid(glm::vec2(worldSpaceHover.x, worldSpaceHover.z));
        }
        const glm::vec3 startCeiling = glm::vec3(start2.x, poly->ceiling, start2.y);
        const glm::vec3 endCeiling = glm::vec3(end2.x, poly->ceiling, end2.y);
        const glm::vec3 startFloor = glm::vec3(start2.x, poly->floor, start2.y);
        const glm::vec3 endFloor = glm::vec3(end2.x, poly->floor, end2.y);

        if (vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ)
        {
            MapRenderer::RenderBillboardPoint(startCeiling + glm::vec3(0, 0.1, 0), 10, Color(1, 0, 0, 1), matrix);
        }
        if (vp.GetType() != Viewport::ViewportType::TOP_DOWN_XZ)
        {
            MapRenderer::RenderLine(startFloor, endFloor, Color(1, 1, 1, 1), matrix, 4);
            MapRenderer::RenderLine(startCeiling, startFloor, Color(.6, .6, .6, 1), matrix, 4);
        }

        MapRenderer::RenderLine(startCeiling, endCeiling, Color(1, 1, 1, 1), matrix, 4);
    }
}

void ViewportRenderer::RenderNewPrimitive(Viewport &vp, const ViewportRenderNewPrimitive *prim, glm::mat4 &matrix)
{
    const std::array<glm::vec3, 4> boxPoints = {
        prim->aabbStart,
        glm::vec3(prim->aabbStart.x, 0, prim->aabbEnd.z),
        prim->aabbEnd,
        glm::vec3(prim->aabbEnd.x, 0, prim->aabbStart.z),
    };
    for (size_t i = 0; i < boxPoints.size(); i++)
    {
        const size_t nextIndex = (i + 1) % boxPoints.size();
        const glm::vec3 startPointCeil = glm::vec3(boxPoints.at(i).x, prim->ceiling, boxPoints.at(i).z);
        const glm::vec3 startPointFloor = glm::vec3(boxPoints.at(i).x, prim->floor, boxPoints.at(i).z);
        const glm::vec3 endPointCeil = glm::vec3(boxPoints.at(nextIndex).x, prim->ceiling, boxPoints.at(nextIndex).z);
        const glm::vec3 endPointFloor = glm::vec3(boxPoints.at(nextIndex).x, prim->floor, boxPoints.at(nextIndex).z);
        MapRenderer::RenderLine(startPointCeil, endPointCeil, Color(.6, 6, 0, 1), matrix, 2);
        if (vp.GetType() != Viewport::ViewportType::TOP_DOWN_XZ)
        {
            MapRenderer::RenderLine(startPointFloor, endPointFloor, Color(.6, .6, 0, 1), matrix, 2);
            MapRenderer::RenderLine(startPointCeil, startPointFloor, Color(.3, .3, 0, 1), matrix, 1);
        }
    }

    for (size_t i = 0; i < prim->points.size(); i++)
    {
        const size_t nextIndex = (i + 1) % prim->points.size();
        const glm::vec3 startPointCeil = glm::vec3(prim->points.at(i).x, prim->ceiling, prim->points.at(i).y);
        const glm::vec3 startPointFloor = glm::vec3(prim->points.at(i).x, prim->floor, prim->points.at(i).y);
        const glm::vec3 endPointCeil = glm::vec3(prim->points.at(nextIndex).x,
                                                 prim->ceiling,
                                                 prim->points.at(nextIndex).y);
        const glm::vec3 endPointFloor = glm::vec3(prim->points.at(nextIndex).x,
                                                  prim->floor,
                                                  prim->points.at(nextIndex).y);
        if (vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ)
        {
            MapRenderer::RenderBillboardPoint(startPointCeil + glm::vec3(0, 0.1, 0), 10, Color(1, 0, 0, 1), matrix);
        }
        MapRenderer::RenderLine(startPointCeil, endPointCeil, Color(1, 1, 1, 1), matrix, 4);
        if (vp.GetType() != Viewport::ViewportType::TOP_DOWN_XZ)
        {
            MapRenderer::RenderLine(startPointFloor, endPointFloor, Color(1, 1, 1, 1), matrix, 4);
            MapRenderer::RenderLine(startPointCeil, startPointFloor, Color(.6, .6, .6, 1), matrix, 2);
        }
    }
}
