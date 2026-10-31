//
// Created by droc101 on 10/16/26.
//

#pragma once
#include <cstddef>
#include <libassets/type/Actor.h>
#include <libassets/type/renderDefs/ModelRenderDefinition.h>
#include <libassets/type/renderDefs/PointRenderDefinition.h>
#include <libassets/type/renderDefs/SpriteRenderDefinition.h>
#include <libassets/type/renderDefs/WallRenderDefinition.h>
#include "MapRenderer.h"
#include "Viewport.h"

class WorldBuffersRenderer
{
    public:
        WorldBuffersRenderer() = delete;

        static void RenderWorldBuffers(const Viewport &vp);

    private:
        static void RenderBrush(size_t brushIndex);
        static void RenderActor(size_t actorIndex, bool isSelected);

        static void RenderModelRdef(ModelRenderDefinition *rdef,
                                    const Actor &actor,
                                    const glm::mat4 &worldMatrix,
                                    MapRenderer::ObjectBufferInfo info);
        static void RenderPointRdef(PointRenderDefinition *rdef,
                                    const Actor &actor,
                                    MapRenderer::ObjectBufferInfo info);

        static void RenderSpriteRdef(SpriteRenderDefinition *rdef,
                                     const Actor &actor,
                                     MapRenderer::ObjectBufferInfo info);

        static void RenderWallRdef(WallRenderDefinition *rdef,
                                   const Actor &actor,
                                   const glm::mat4 &worldMatrix,
                                   MapRenderer::ObjectBufferInfo info);
};
