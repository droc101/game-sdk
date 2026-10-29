//
// Created by droc101 on 10/16/26.
//

#include "WorldBuffersRenderer.h"
#include <cstddef>
#include <cstdint>
#include <game_sdk/gl/GLHelper.h>
#include <libassets/type/Actor.h>
#include <libassets/type/ActorDefinition.h>
#include <libassets/type/Brush.h>
#include <libassets/type/renderDefs/ModelRenderDefinition.h>
#include <libassets/type/renderDefs/PointRenderDefinition.h>
#include <libassets/type/renderDefs/RenderDefinition.h>
#include <libassets/type/renderDefs/SpriteRenderDefinition.h>
#include <libassets/type/renderDefs/WallRenderDefinition.h>
#include <libassets/util/Colors.h>
#include <memory>
#include <vector>
#include "MapEditor.h"
#include "MapRenderer.h"
#include "tools/EditorTool.h"
#include "Viewport.h"

void WorldBuffersRenderer::RenderWorldBuffers(const Viewport &vp)
{
    vp.BindWorldBuffers();
    GLHelper::Clear();

    for (size_t actorIndex = 0; actorIndex < MapEditor::map.actors.size(); actorIndex++)
    {
        RenderActor(actorIndex);
    }

    for (size_t brushIndex = 0; brushIndex < MapEditor::map.brushes.size(); brushIndex++)
    {
        RenderBrush(brushIndex);
    }

    MapRenderer::ExecuteObjectBufferCommands(vp.GetMatrix());
}

void WorldBuffersRenderer::RenderBrush(const size_t brushIndex)
{
    Brush &brush = MapEditor::map.brushes.at(brushIndex);
    MapRenderer::ObjectBufferInfo info = {
        .objectType = static_cast<uint32_t>(EditorTool::ItemType::BRUSH),
        .objectIndex = static_cast<uint32_t>(brushIndex),
        .objectSubIndex = 0,
    };

    const glm::mat4 brushMatrix = brush.GetTransformMatrix();

    const std::vector<Brush::TriangulatedFace> &triangulatedFaces = brush.GetTriangulatedMesh();
    for (uint32_t i = 0; i < triangulatedFaces.size(); i++)
    {
        const Brush::TriangulatedFace &face = triangulatedFaces.at(i);
        info.objectSubIndex = i;
        MapRenderer::CmdObjectBufferBrushFace(face, brushMatrix, info);
    }
}

void WorldBuffersRenderer::RenderActor(const size_t actorIndex)
{
    const Actor &a = MapEditor::map.actors.at(actorIndex);
    const MapRenderer::ObjectBufferInfo info = {
        .objectType = static_cast<uint32_t>(EditorTool::ItemType::ACTOR),
        .objectIndex = static_cast<uint32_t>(actorIndex),
        .objectSubIndex = 0,
    };

    const ActorDefinition &definition = MapEditor::adm.GetActorDefinition(a.className);

    glm::mat4 worldMatrix = glm::identity<glm::mat4>();
    worldMatrix = glm::translate(worldMatrix, a.position);
    worldMatrix = glm::rotate(worldMatrix, glm::radians(a.rotation.y), glm::vec3(0, 1, 0));
    worldMatrix = glm::rotate(worldMatrix, glm::radians(a.rotation.x), glm::vec3(1, 0, 0));
    worldMatrix = glm::rotate(worldMatrix, -glm::radians(a.rotation.z), glm::vec3(0, 0, 1));

    for (const std::shared_ptr<RenderDefinition> &rdef: definition.renderDefinitions)
    {
        if (!MapEditor::options.drawGizmos && rdef.get()->IsGizmo(a))
        {
            continue;
        }
        const RenderDefinition::RenderDefinitionViewportMode vpMode = rdef.get()->GetViewportMode(a);
        if (vpMode == RenderDefinition::RenderDefinitionViewportMode::ONLY_2D)
        {
            continue;
        }
        switch (rdef.get()->GetType())
        {
            case RenderDefinition::RenderDefinitionType::RD_TYPE_MODEL:
                RenderModelRdef(dynamic_cast<ModelRenderDefinition *>(rdef.get()), a, worldMatrix, info);
                break;
            case RenderDefinition::RenderDefinitionType::RD_TYPE_POINT:
                RenderPointRdef(dynamic_cast<PointRenderDefinition *>(rdef.get()), a, info);
                break;
            case RenderDefinition::RenderDefinitionType::RD_TYPE_SPRITE:
                RenderSpriteRdef(dynamic_cast<SpriteRenderDefinition *>(rdef.get()), a, info);
                break;
            case RenderDefinition::RenderDefinitionType::RD_TYPE_WALL:
                RenderWallRdef(dynamic_cast<WallRenderDefinition *>(rdef.get()), a, worldMatrix, info);
                break;
            default:
                break;
        }
    }
}

void WorldBuffersRenderer::RenderPointRdef(PointRenderDefinition *rdef,
                                           const Actor &actor,
                                           const MapRenderer::ObjectBufferInfo info)
{
    MapRenderer::CmdObjectBufferPoint(actor.position, rdef->GetPointSize(actor), info);
}

void WorldBuffersRenderer::RenderSpriteRdef(SpriteRenderDefinition *rdef,
                                            const Actor &actor,
                                            const MapRenderer::ObjectBufferInfo info)
{
    MapRenderer::CmdObjectBufferPoint(actor.position, rdef->GetPointSize(actor), info);
}

void WorldBuffersRenderer::RenderWallRdef(WallRenderDefinition *rdef,
                                          const Actor &actor,
                                          const glm::mat4 &worldMatrix,
                                          const MapRenderer::ObjectBufferInfo info)
{
    const MapRenderer::Wall wall = {
        .color = Colors::WHITE,
        .zAxisOrientation = rdef->GetZAxisOrientation(actor),
        .localCenter = rdef->GetLocalCenter(actor),
        .size = rdef->GetSize(actor),
        .texture = "",
        .uvOffset = glm::vec2(0),
        .uvScale = glm::vec2(0),
        .unshaded = true,
    };
    MapRenderer::CmdObjectBufferWall(worldMatrix, wall, info);
}

void WorldBuffersRenderer::RenderModelRdef(ModelRenderDefinition *rdef,
                                           const Actor &actor,
                                           const glm::mat4 &worldMatrix,
                                           const MapRenderer::ObjectBufferInfo info)
{
    MapRenderer::CmdObjectBufferModel(rdef->GetModel(actor), worldMatrix, info);
}
