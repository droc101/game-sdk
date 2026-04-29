//
// Created by droc101 on 1/9/26.
//

#include <libassets/type/renderDefs/BoxRenderDefinition.h>
#include <libassets/type/renderDefs/ModelRenderDefinition.h>
#include <libassets/type/renderDefs/OrientationRenderDefinition.h>
#include <libassets/type/renderDefs/PointRenderDefinition.h>
#include <libassets/type/renderDefs/RenderDefinition.h>
#include <libassets/type/renderDefs/SpriteRenderDefinition.h>
#include <libassets/util/Error.h>
#include <libassets/util/Logger.h>
#include <memory>
#include <string>

std::unique_ptr<RenderDefinition> RenderDefinition::Create(const nlohmann::json &json, Error::ErrorCode &e)
{
    std::unique_ptr<RenderDefinition> output = nullptr;
    const RenderDefinitionType type = ParseType(json.value("type", "point"));
    if (type == RenderDefinitionType::RD_TYPE_UNKNOWN)
    {
        Logger::Error("Failed to parse type of render definition:\n{}", json.dump(4).c_str());
        e = Error::ErrorCode::INCORRECT_FORMAT;
        return nullptr;
    }

    // RD_TYPE_UNKNOWN handled above
    // ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement
    // ReSharper disable once CppIncompleteSwitchStatement
    switch (type)
    {
        case RenderDefinitionType::RD_TYPE_BOX:
            output = std::make_unique<BoxRenderDefinition>(json);
            break;
        case RenderDefinitionType::RD_TYPE_MODEL:
            output = std::make_unique<ModelRenderDefinition>(json);
            break;
        case RenderDefinitionType::RD_TYPE_ORIENTATION:
            output = std::make_unique<OrientationRenderDefinition>(json);
            break;
        case RenderDefinitionType::RD_TYPE_POINT:
            output = std::make_unique<PointRenderDefinition>(json);
            break;
        case RenderDefinitionType::RD_TYPE_SPRITE:
            output = std::make_unique<SpriteRenderDefinition>(json);
            break;
    }

    output->type = type;

    e = Error::ErrorCode::OK;
    return output;
}

RenderDefinition::RenderDefinitionType RenderDefinition::GetType() const
{
    return type;
}

RenderDefinition::RenderDefinitionType RenderDefinition::ParseType(const std::string &type)
{
    if (type == "box")
    {
        return RenderDefinitionType::RD_TYPE_BOX;
    }
    if (type == "model")
    {
        return RenderDefinitionType::RD_TYPE_MODEL;
    }
    if (type == "orientation" || type == "direction")
    {
        return RenderDefinitionType::RD_TYPE_ORIENTATION;
    }
    if (type == "point")
    {
        return RenderDefinitionType::RD_TYPE_POINT;
    }
    if (type == "sprite" || type == "texture")
    {
        return RenderDefinitionType::RD_TYPE_SPRITE;
    }
    return RenderDefinitionType::RD_TYPE_UNKNOWN;
}
