//
// Created by droc101 on 6/9/26.
//

#pragma once

#include <libassets/type/Param.h>
#include <libassets/type/renderDefs/values/RenderDefinitionValue.h>
#include <string>

template<typename T> class BasicDefinitionValue: public RenderDefinitionValue<T>
{
    public:
        BasicDefinitionValue() = default;
        explicit BasicDefinitionValue(const T &defaultValue)
        {
            this->usesParam = false;
            this->value = defaultValue;
        }

        T Get(const KvList &params, const T &defaultValue) override
        {
            if (usesParam)
            {
                if (params.contains(paramName))
                {
                    return params.at(paramName).Get<T>(defaultValue);
                }
            } else
            {
                return value;
            }
            return defaultValue;
        }

    protected:
        bool usesParam = false;
        std::string paramName{};
        T value{};
};
