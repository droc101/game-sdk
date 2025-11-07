//
// Created by droc101 on 9/5/25.
//

#pragma once

#include <concepts>
#include <cstdint>
#include <libassets/type/Color.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <string>
#include <variant>

template<typename T> concept ParamTypeTemplate = std::same_as<T, uint8_t> ||
                                                 std::same_as<T, int32_t> ||
                                                 std::same_as<T, float> ||
                                                 std::same_as<T, bool> ||
                                                 std::same_as<T, std::string> ||
                                                 std::same_as<T, Color>;

class Param
{
    public:
        enum class ParamType : uint8_t
        {
            PARAM_TYPE_BYTE,
            PARAM_TYPE_INTEGER,
            PARAM_TYPE_FLOAT,
            PARAM_TYPE_BOOL,
            PARAM_TYPE_STRING,
            PARAM_TYPE_NONE,
            PARAM_TYPE_COLOR
        };

        Param();
        explicit Param(DataReader &reader);
        template<ParamTypeTemplate T> explicit Param(T value)
        {
            Set<T>(value);
        }
        bool operator==(const Param & param) const;

        void Write(DataWriter &writer) const;

        static ParamType ParseType(const std::string &type);

        void Clear();

        void ClearToType(const ParamType dataType);

        [[nodiscard]] ParamType GetType() const;

        template<ParamTypeTemplate T> [[nodiscard]] T Get(T defaultValue) const
        {
            if ((std::same_as<T, uint8_t> && type != ParamType::PARAM_TYPE_BYTE) ||
                (std::same_as<T, int32_t> && type != ParamType::PARAM_TYPE_INTEGER) ||
                (std::same_as<T, float> && type != ParamType::PARAM_TYPE_FLOAT) ||
                (std::same_as<T, bool> && type != ParamType::PARAM_TYPE_BOOL) ||
                (std::same_as<T, std::string> && type != ParamType::PARAM_TYPE_STRING) ||
                (std::same_as<T, Color> && type != ParamType::PARAM_TYPE_COLOR))
            {
                return defaultValue;
            }
            return std::get<T>(value);
        }

        template<ParamTypeTemplate T> void Set(T newValue)
        {
            if (std::same_as<T, uint8_t>)
            {
                value = newValue;
                type = ParamType::PARAM_TYPE_BYTE;
            } else if (std::same_as<T, int32_t>)
            {
                value = newValue;
                type = ParamType::PARAM_TYPE_INTEGER;
            } else if (std::same_as<T, float>)
            {
                value = newValue;
                type = ParamType::PARAM_TYPE_FLOAT;
            } else if (std::same_as<T, bool>)
            {
                value = newValue;
                type = ParamType::PARAM_TYPE_BOOL;
            } else if (std::same_as<T, std::string>)
            {
                value = newValue;
                type = ParamType::PARAM_TYPE_STRING;
            } else if (std::same_as<T, Color>)
            {
                value = newValue;
                type = ParamType::PARAM_TYPE_COLOR;
            }
        }

    private:
        union ParamValue
        {
                uint8_t byteValue;
                int32_t intValue;
                float floatValue;
                bool boolValue;
                std::string stringValue;
                Color colorValue;
        };

        ParamType type = ParamType::PARAM_TYPE_NONE;
        std::variant<uint8_t, int32_t, float, bool, std::string, Color> value;
};
