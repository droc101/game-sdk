//
// Created by droc101 on 4/26/26.
//

#include "ViewportRenderer.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <game_sdk/gl/GLHelper.h>
#include <iterator>
#include <libassets/asset/ModelAsset.h>
#include <libassets/type/Actor.h>
#include <libassets/type/ActorDefinition.h>
#include <libassets/type/BoundingBox.h>
#include <libassets/type/Color.h>
#include <libassets/type/renderDefs/BoxRenderDefinition.h>
#include <libassets/type/renderDefs/ModelRenderDefinition.h>
#include <libassets/type/renderDefs/OrientationRenderDefinition.h>
#include <libassets/type/renderDefs/PointRenderDefinition.h>
#include <libassets/type/renderDefs/RenderDefinition.h>
#include <libassets/type/renderDefs/SpriteRenderDefinition.h>
#include <libassets/type/Sector.h>
#include <memory>
#include <vector>
#include "MapEditor.h"
#include "MapRenderer.h"
#include "tools/EditorTool.h"
#include "Viewport.h"

void ViewportRenderer::RenderViewport(Viewport &vp, const ViewportRenderSettings &settings)
{
    MapRenderer::RenderViewportGrid(vp);

    glm::mat4 matrix = vp.GetMatrix();

    for (const Actor &actor: MapEditor::map.actors)
    {
        RenderActor(actor, matrix, vp);
    }

    for (size_t sectorIndex = 0; sectorIndex < MapEditor::map.sectors.size(); sectorIndex++)
    {
        if (settings.sectorFocusMode && sectorIndex == settings.focusedSectorIndex)
        {
            continue;
        }
        if (settings.selectionType == EditorTool::ItemType::SECTOR && settings.selectionIndex == sectorIndex)
        {
            continue;
        }
        if (settings.hoverType == EditorTool::ItemType::SECTOR && settings.hoverIndex == sectorIndex)
        {
            continue;
        }
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

    if (settings.hoverType == EditorTool::ItemType::SECTOR)
    {
        GLHelper::ClearDepth();
        RenderSector(vp, settings, settings.hoverIndex, matrix);
    }

    if (settings.selectionType == EditorTool::ItemType::SECTOR)
    {
        GLHelper::ClearDepth();
        RenderSector(vp, settings, settings.selectionIndex, matrix);
    }

    if (settings.sectorFocusMode)
    {
        GLHelper::ClearDepth();
        RenderSector(vp, settings, settings.focusedSectorIndex, matrix);
    }
}

void ViewportRenderer::RenderSector(const Viewport &vp,
                                    const ViewportRenderSettings &settings,
                                    const size_t sectorIndex,
                                    const glm::mat4 &matrix)
{
    const Sector &sector = MapEditor::map.sectors.at(sectorIndex);

    if (MapEditor::culling && SectorIsCulled(sector, vp))
    {
        return;
    }

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

void ViewportRenderer::RenderPoint(const ViewportRenderPoint *point, const glm::mat4 &matrix)
{
    MapRenderer::RenderBillboardPoint(point->pos, point->size, point->color, matrix);
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

void ViewportRenderer::RenderNewActor(const Viewport &vp, const ViewportRenderNewActor *actor, const glm::mat4 &matrix)
{
    const ActorDefinition &newActorDef = MapEditor::adm.GetActorDefinition(actor->className);
    Actor tempActor{};
    tempActor.ApplyDefinition(newActorDef, true);
    tempActor.position = actor->position;
    tempActor.rotation = actor->rotation;
    tempActor.className = actor->className;
    RenderActor(tempActor, matrix, vp);
}

void ViewportRenderer::RenderNewPolygon(const Viewport &vp,
                                        const ViewportRenderNewPolygon *poly,
                                        const glm::mat4 &matrix)
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

bool ViewportRenderer::SectorIsCulled(const Sector &sector, const Viewport &vp)
{
    const glm::vec4 aabb = sector.GetAABB();

    const glm::vec2 cameraViewSize = vp.GetWorldSpaceSize();
    const glm::vec3 cameraPos = vp.GetCameraPos();

    if (vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ)
    {
        const glm::vec2 aabbTopLeft = {aabb.x - aabb.z, aabb.y - aabb.w};
        const glm::vec2 aabbBottomRight = {aabb.x + aabb.z, aabb.y + aabb.w};
        const glm::vec2 cameraTopLeft = {cameraPos.x - (cameraViewSize.x / 2), cameraPos.z - (cameraViewSize.y / 2)};
        const glm::vec2 cameraBottomRight = {cameraPos.x + (cameraViewSize.x / 2),
                                             cameraPos.z + (cameraViewSize.y / 2)};
        if (aabbBottomRight.x < cameraTopLeft.x ||
            aabbTopLeft.x > cameraBottomRight.x ||
            aabbBottomRight.y < cameraTopLeft.y ||
            aabbTopLeft.y > cameraBottomRight.y)
        {
            return true;
        }
    } else if (vp.GetType() == Viewport::ViewportType::FRONT_XY)
    {
        const glm::vec2 aabbTopLeft = {aabb.x - aabb.z, sector.floorHeight};
        const glm::vec2 aabbBottomRight = {aabb.x + aabb.z, sector.ceilingHeight};
        const glm::vec2 cameraTopLeft = {cameraPos.x - (cameraViewSize.x / 2), cameraPos.y - (cameraViewSize.y / 2)};
        const glm::vec2 cameraBottomRight = {cameraPos.x + (cameraViewSize.x / 2),
                                             cameraPos.y + (cameraViewSize.y / 2)};
        if (aabbBottomRight.x < cameraTopLeft.x ||
            aabbTopLeft.x > cameraBottomRight.x ||
            aabbBottomRight.y < cameraTopLeft.y ||
            aabbTopLeft.y > cameraBottomRight.y)
        {
            return true;
        }
    } else if (vp.GetType() == Viewport::ViewportType::SIDE_YZ)
    {
        const glm::vec2 aabbTopLeft = {sector.floorHeight, aabb.y - aabb.w};
        const glm::vec2 aabbBottomRight = {sector.ceilingHeight, aabb.y + aabb.w};
        const glm::vec2 cameraTopLeft = {cameraPos.y - (cameraViewSize.y / 2), cameraPos.z - (cameraViewSize.x / 2)};
        const glm::vec2 cameraBottomRight = {cameraPos.y + (cameraViewSize.y / 2),
                                             cameraPos.z + (cameraViewSize.x / 2)};
        if (aabbBottomRight.x < cameraTopLeft.x ||
            aabbTopLeft.x > cameraBottomRight.x ||
            aabbBottomRight.y < cameraTopLeft.y ||
            aabbTopLeft.y > cameraBottomRight.y)
        {
            return true;
        }
    }
    return false;
}

void ViewportRenderer::RenderActor(const Actor &a, const glm::mat4 &matrix, const Viewport &vp)
{
    if (MapEditor::culling && ActorIsCulled(a, vp))
    {
        return;
    }

    const ActorDefinition &definition = MapEditor::adm.GetActorDefinition(a.className);

    glm::mat4 worldMatrix = glm::identity<glm::mat4>();
    worldMatrix = glm::translate(worldMatrix, a.position);
    worldMatrix = glm::rotate(worldMatrix, glm::radians(a.rotation.y), glm::vec3(0, 1, 0));
    worldMatrix = glm::rotate(worldMatrix, glm::radians(a.rotation.x), glm::vec3(1, 0, 0));
    worldMatrix = glm::rotate(worldMatrix, glm::radians(a.rotation.z), glm::vec3(0, 0, 1));

    for (const std::shared_ptr<RenderDefinition> &rdef: definition.renderDefinitions)
    {
        switch (rdef.get()->GetType())
        {
            case RenderDefinition::RenderDefinitionType::RD_TYPE_BOX:
                RenderBoxRdef(dynamic_cast<BoxRenderDefinition *>(rdef.get()), a, worldMatrix, matrix);
                break;
            case RenderDefinition::RenderDefinitionType::RD_TYPE_MODEL:
                RenderModelRdef(dynamic_cast<ModelRenderDefinition *>(rdef.get()), a, worldMatrix, matrix);
                break;
            case RenderDefinition::RenderDefinitionType::RD_TYPE_ORIENTATION:
                RenderOrientationRdef(dynamic_cast<OrientationRenderDefinition *>(rdef.get()), a, matrix, vp);
                break;
            case RenderDefinition::RenderDefinitionType::RD_TYPE_POINT:
                RenderPointRdef(dynamic_cast<PointRenderDefinition *>(rdef.get()), a, matrix);
                break;
            case RenderDefinition::RenderDefinitionType::RD_TYPE_SPRITE:
                RenderSpriteRdef(dynamic_cast<SpriteRenderDefinition *>(rdef.get()), a, matrix);
                break;
            default:
            case RenderDefinition::RenderDefinitionType::RD_TYPE_UNKNOWN:
                assert(false); // should be impossible
                break;
        }
    }
}

bool ViewportRenderer::ActorIsCulled(const Actor &actor, const Viewport &vp)
{
    const ActorDefinition &definition = MapEditor::adm.GetActorDefinition(actor.className);

    glm::mat4 worldMatrix = glm::identity<glm::mat4>();
    worldMatrix = glm::translate(worldMatrix, actor.position);
    worldMatrix = glm::rotate(worldMatrix, glm::radians(actor.rotation.y), glm::vec3(0, 1, 0));
    worldMatrix = glm::rotate(worldMatrix, glm::radians(actor.rotation.x), glm::vec3(1, 0, 0));
    worldMatrix = glm::rotate(worldMatrix, glm::radians(actor.rotation.z), glm::vec3(0, 0, 1));

    std::vector<glm::vec3> boundingBoxPoints{}; // not aabb, just bb

    for (const std::shared_ptr<RenderDefinition> &rdef: definition.renderDefinitions)
    {
        const RenderDefinition::RenderDefinitionType type = rdef.get()->GetType();
        if (type == RenderDefinition::RenderDefinitionType::RD_TYPE_POINT ||
            type == RenderDefinition::RenderDefinitionType::RD_TYPE_ORIENTATION ||
            type == RenderDefinition::RenderDefinitionType::RD_TYPE_SPRITE)
        {
            std::ranges::copy(BoundingBox(actor.position, {1, 1, 1}).GetPoints(),
                              std::back_inserter(boundingBoxPoints));
        } else if (type == RenderDefinition::RenderDefinitionType::RD_TYPE_MODEL)
        {
            const ModelRenderDefinition *modelDef = dynamic_cast<ModelRenderDefinition *>(rdef.get());
            const ModelAsset &model = MapRenderer::GetModel(modelDef->GetModel(actor));
            const std::array<glm::vec3, 8> &modelBboxPoints = model.GetBoundingBox().GetPoints();
            for (const glm::vec3 &point: modelBboxPoints)
            {
                boundingBoxPoints.emplace_back(worldMatrix * glm::vec4(point, 1.0));
            }
        }
    }

    const BoundingBox bbox = BoundingBox(boundingBoxPoints);

    const glm::vec2 cameraViewSize = vp.GetWorldSpaceSize();
    const glm::vec3 cameraPos = vp.GetCameraPos();

    if (vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ)
    {
        const glm::vec2 aabbTopLeft = {bbox.origin.x - bbox.extents.x, bbox.origin.z - bbox.extents.z};
        const glm::vec2 aabbBottomRight = {bbox.origin.x + bbox.extents.x, bbox.origin.z + bbox.extents.z};
        const glm::vec2 cameraTopLeft = {cameraPos.x - (cameraViewSize.x / 2), cameraPos.z - (cameraViewSize.y / 2)};
        const glm::vec2 cameraBottomRight = {cameraPos.x + (cameraViewSize.x / 2),
                                             cameraPos.z + (cameraViewSize.y / 2)};
        if (aabbBottomRight.x < cameraTopLeft.x ||
            aabbTopLeft.x > cameraBottomRight.x ||
            aabbBottomRight.y < cameraTopLeft.y ||
            aabbTopLeft.y > cameraBottomRight.y)
        {
            return true;
        }
    } else if (vp.GetType() == Viewport::ViewportType::FRONT_XY)
    {
        const glm::vec2 aabbTopLeft = {bbox.origin.x - bbox.extents.x, bbox.origin.y - bbox.extents.y};
        const glm::vec2 aabbBottomRight = {bbox.origin.x + bbox.extents.x, bbox.origin.y + bbox.extents.y};
        const glm::vec2 cameraTopLeft = {cameraPos.x - (cameraViewSize.x / 2), cameraPos.y - (cameraViewSize.y / 2)};
        const glm::vec2 cameraBottomRight = {cameraPos.x + (cameraViewSize.x / 2),
                                             cameraPos.y + (cameraViewSize.y / 2)};
        if (aabbBottomRight.x < cameraTopLeft.x ||
            aabbTopLeft.x > cameraBottomRight.x ||
            aabbBottomRight.y < cameraTopLeft.y ||
            aabbTopLeft.y > cameraBottomRight.y)
        {
            return true;
        }
    } else if (vp.GetType() == Viewport::ViewportType::SIDE_YZ)
    {
        const glm::vec2 aabbTopLeft = {bbox.origin.y - bbox.extents.y, bbox.origin.z - bbox.extents.z};
        const glm::vec2 aabbBottomRight = {bbox.origin.y + bbox.extents.y, bbox.origin.z + bbox.extents.z};
        const glm::vec2 cameraTopLeft = {cameraPos.y - (cameraViewSize.y / 2), cameraPos.z - (cameraViewSize.x / 2)};
        const glm::vec2 cameraBottomRight = {cameraPos.y + (cameraViewSize.y / 2),
                                             cameraPos.z + (cameraViewSize.x / 2)};
        if (aabbBottomRight.x < cameraTopLeft.x ||
            aabbTopLeft.x > cameraBottomRight.x ||
            aabbBottomRight.y < cameraTopLeft.y ||
            aabbTopLeft.y > cameraBottomRight.y)
        {
            return true;
        }
    }

    return false;
}

void ViewportRenderer::RenderBoxRdef(const BoxRenderDefinition *rdef,
                                     const Actor &actor,
                                     const glm::mat4 &worldMatrix,
                                     const glm::mat4 &matrix)
{
    const Color c = rdef->GetColor(actor);
    const glm::vec3 boxExtents = rdef->GetExtents(actor);
    const std::array<glm::vec2, 4> boxPoints = {
        glm::vec2(-boxExtents.x / 2.0f, -boxExtents.z / 2.0f),
        glm::vec2(-boxExtents.x / 2.0f, boxExtents.z / 2.0f),
        glm::vec2(boxExtents.x / 2.0f, boxExtents.z / 2.0f),
        glm::vec2(boxExtents.x / 2.0f, -boxExtents.z / 2.0f),
    };
    for (size_t i = 0; i < boxPoints.size(); i++)
    {
        const size_t nextIndex = (i + 1) % boxPoints.size();
        const glm::vec3 startPointCeil = worldMatrix * glm::vec4(boxPoints.at(i).x, boxExtents.y, boxPoints.at(i).y, 1);
        const glm::vec3 startPointFloor = worldMatrix *
                                          glm::vec4(boxPoints.at(i).x, -boxExtents.y, boxPoints.at(i).y, 1);
        const glm::vec3 endPointCeil = worldMatrix *
                                       glm::vec4(boxPoints.at(nextIndex).x, boxExtents.y, boxPoints.at(nextIndex).y, 1);
        const glm::vec3 endPointFloor = worldMatrix * glm::vec4(boxPoints.at(nextIndex).x,
                                                                -boxExtents.y,
                                                                boxPoints.at(nextIndex).y,
                                                                1);
        MapRenderer::RenderLine(startPointCeil, endPointCeil, c, matrix, 2);
        MapRenderer::RenderLine(startPointFloor, endPointFloor, c, matrix, 2);
        MapRenderer::RenderLine(startPointCeil, startPointFloor, c, matrix, 2);
    }
}

void ViewportRenderer::RenderModelRdef(const ModelRenderDefinition *rdef,
                                       const Actor &actor,
                                       const glm::mat4 &worldMatrix,
                                       const glm::mat4 &matrix)
{
    if (MapEditor::drawModels)
    {
        MapRenderer::RenderModel(rdef->GetModel(actor), matrix, worldMatrix, rdef->GetColor(actor));
    }
}

void ViewportRenderer::RenderOrientationRdef(const OrientationRenderDefinition *rdef,
                                             const Actor &actor,
                                             const glm::mat4 &matrix,
                                             const Viewport &vp)
{
    MapRenderer::RenderUnitVector(actor.position, actor.rotation, rdef->GetColor(actor), matrix, 2, vp.GetZoom() / 20);
}

void ViewportRenderer::RenderPointRdef(const PointRenderDefinition *rdef, const Actor &actor, const glm::mat4 &matrix)
{
    MapRenderer::RenderBillboardPoint(actor.position, rdef->GetPointSize(actor), rdef->GetColor(actor), matrix);
}

void ViewportRenderer::RenderSpriteRdef(const SpriteRenderDefinition *rdef, const Actor &actor, const glm::mat4 &matrix)
{
    MapRenderer::RenderBillboardSprite(actor.position,
                                       rdef->GetPointSize(actor),
                                       rdef->GetTexture(actor),
                                       rdef->GetTintColor(actor),
                                       matrix);
}
