//
// Created by droc101 on 1/8/26.
//

#pragma once

#include <cstdint>
#include <libassets/type/RenderDefinitionValue.h>
#include <libassets/util/Error.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

class Actor;

class RenderDefinition
{
    public:
        enum class RenderDefinitionType : uint8_t
        {
            RD_TYPE_UNKNOWN,
            RD_TYPE_BOX,
            RD_TYPE_MODEL,
            RD_TYPE_ORIENTATION,
            RD_TYPE_POINT,
            RD_TYPE_SPRITE,
            RD_TYPE_WALL,
            RD_TYPE_CIRCLE,
        };

        RenderDefinition();
        explicit RenderDefinition(const nlohmann::json &json);

        virtual ~RenderDefinition() = default;

        [[nodiscard]] static std::unique_ptr<RenderDefinition> Create(const nlohmann::json &json, Error::ErrorCode &e);

        [[nodiscard]] RenderDefinitionType GetType() const;

        bool IsGizmo(const Actor &actor) const;

    protected:
        RenderDefinitionType type = RenderDefinitionType::RD_TYPE_UNKNOWN;
        RenderDefinitionValue<bool> gizmo;

    private:
        static RenderDefinitionType ParseType(const std::string &type);
};
