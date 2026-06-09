//
// Created by droc101 on 4/28/26.
//

#pragma once

#include <concepts>
#include <cstdint>
#include <libassets/type/Color.h>
#include <libassets/type/Param.h>
#include <string>

template<typename T> concept RDVTypeTemplate = std::same_as<T, float> ||
                                               std::same_as<T, bool> ||
                                               std::same_as<T, std::string> ||
                                               std::same_as<T, Color> ||
                                               std::same_as<T, int32_t>;

template<RDVTypeTemplate T> class RenderDefinitionValue
{
    public:
        RenderDefinitionValue() = default;
        explicit RenderDefinitionValue(const T &defaultValue) = delete;
        RenderDefinitionValue(const nlohmann::json &json, const std::string &key, const T &defaultValue) = delete;

        virtual ~RenderDefinitionValue() = default;

        virtual T Get(const KvList &params, const T &defaultValue) = 0;
};
