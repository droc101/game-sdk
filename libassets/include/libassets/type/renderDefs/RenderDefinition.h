//
// Created by droc101 on 1/8/26.
//

#pragma once

#include <libassets/type/Color.h>
#include <libassets/util/Error.h>
#include <nlohmann/json.hpp>
#include <string>

class RenderDefinition
{
    public:
        std::string modelSourceParam{};
        std::string colorSourceParam{};
        std::string textureSourceParam{};
        std::string model{};
        std::string texture{};
        Color color{};
        bool directional = true;

        virtual ~RenderDefinition() = default;

        [[nodiscard]] static RenderDefinition Create(const nlohmann::json &json, Error::ErrorCode &e);
};
