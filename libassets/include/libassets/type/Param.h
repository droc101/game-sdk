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
#include <unordered_map>
#include <variant>
#include <vector>

class Param;

using ParamVector = std::vector<Param>;
using KvList = std::unordered_map<std::string, Param>;

template<typename T> concept ParamTypeTemplate = std::same_as<T, uint8_t> ||
                                                 std::same_as<T, int32_t> ||
                                                 std::same_as<T, float> ||
                                                 std::same_as<T, bool> ||
                                                 std::same_as<T, std::string> ||
                                                 std::same_as<T, Color> ||
                                                 std::same_as<T, KvList> ||
                                                 std::same_as<T, ParamVector>;

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
            PARAM_TYPE_COLOR,
            PARAM_TYPE_KV_LIST,
            PARAM_TYPE_ARRAY
        };

        static inline const std::unordered_map<ParamType, std::string> paramTypeNames =
                {{ParamType::PARAM_TYPE_BYTE, "byte"},
                 {ParamType::PARAM_TYPE_INTEGER, "int"},
                 {ParamType::PARAM_TYPE_FLOAT, "float"},
                 {ParamType::PARAM_TYPE_BOOL, "bool"},
                 {ParamType::PARAM_TYPE_STRING, "string"},
                 {ParamType::PARAM_TYPE_NONE, "none"},
                 {ParamType::PARAM_TYPE_COLOR, "Color"},
                 {ParamType::PARAM_TYPE_ARRAY, "Array"},
                 {ParamType::PARAM_TYPE_KV_LIST, "KvList"}};

        Param();
        explicit Param(DataReader &reader);
        template<ParamTypeTemplate T> explicit Param(T value)
        {
            Set<T>(value);
        }
        explicit Param(nlohmann::ordered_json j);
        bool operator==(const Param &param) const;

        void Write(DataWriter &writer) const;

        static ParamType ParseType(const std::string &type);

        void Clear();

        void ClearToType(ParamType dataType);

        [[nodiscard]] ParamType GetType() const;

        [[nodiscard]] nlohmann::ordered_json GetJson() const;

        [[nodiscard]] std::string GetTypeName() const;

        template<ParamTypeTemplate T> [[nodiscard]] T Get(T defaultValue) const
        {
            if ((std::same_as<T, uint8_t> && type != ParamType::PARAM_TYPE_BYTE) ||
                (std::same_as<T, int32_t> && type != ParamType::PARAM_TYPE_INTEGER) ||
                (std::same_as<T, float> && type != ParamType::PARAM_TYPE_FLOAT) ||
                (std::same_as<T, bool> && type != ParamType::PARAM_TYPE_BOOL) ||
                (std::same_as<T, std::string> && type != ParamType::PARAM_TYPE_STRING) ||
                (std::same_as<T, Color> && type != ParamType::PARAM_TYPE_COLOR) ||
                (std::same_as<T, KvList> && type != ParamType::PARAM_TYPE_KV_LIST) ||
                (std::same_as<T, ParamVector> && type != ParamType::PARAM_TYPE_ARRAY))
            {
                return defaultValue;
            }
            return std::get<T>(value);
        }

        template<ParamTypeTemplate T> [[nodiscard]] T &GetRef(T defaultValue)
        {
            if ((std::same_as<T, uint8_t> && type != ParamType::PARAM_TYPE_BYTE) ||
                (std::same_as<T, int32_t> && type != ParamType::PARAM_TYPE_INTEGER) ||
                (std::same_as<T, float> && type != ParamType::PARAM_TYPE_FLOAT) ||
                (std::same_as<T, bool> && type != ParamType::PARAM_TYPE_BOOL) ||
                (std::same_as<T, std::string> && type != ParamType::PARAM_TYPE_STRING) ||
                (std::same_as<T, Color> && type != ParamType::PARAM_TYPE_COLOR) ||
                (std::same_as<T, KvList> && type != ParamType::PARAM_TYPE_KV_LIST) ||
                (std::same_as<T, ParamVector> && type != ParamType::PARAM_TYPE_ARRAY))
            {
                return defaultValue;
            }
            return std::get<T>(value);
        }

        template<ParamTypeTemplate T> [[nodiscard]] T *GetPointer()
        {
            if ((std::same_as<T, uint8_t> && type != ParamType::PARAM_TYPE_BYTE) ||
                (std::same_as<T, int32_t> && type != ParamType::PARAM_TYPE_INTEGER) ||
                (std::same_as<T, float> && type != ParamType::PARAM_TYPE_FLOAT) ||
                (std::same_as<T, bool> && type != ParamType::PARAM_TYPE_BOOL) ||
                (std::same_as<T, std::string> && type != ParamType::PARAM_TYPE_STRING) ||
                (std::same_as<T, Color> && type != ParamType::PARAM_TYPE_COLOR) ||
                (std::same_as<T, KvList> && type != ParamType::PARAM_TYPE_KV_LIST) ||
                (std::same_as<T, ParamVector> && type != ParamType::PARAM_TYPE_ARRAY))
            {
                return nullptr;
            }
            return &std::get<T>(value);
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
            } else if (std::same_as<T, KvList>)
            {
                value = newValue;
                type = ParamType::PARAM_TYPE_KV_LIST;
            } else if (std::same_as<T, ParamVector>)
            {
                value = newValue;
                type = ParamType::PARAM_TYPE_ARRAY;
            }
        }

        static void WriteKvList(DataWriter &writer, const KvList &list);

        static KvList ReadKvList(DataReader &reader);

        static nlohmann::ordered_json GenerateKvListJson(const KvList &list);

        static KvList KvListFromJson(const nlohmann::ordered_json &json);

        Param *ArrayElementPointer(const size_t index);
        Param *KvListElementPointer(const std::string &key);

    private:
        ParamType type = ParamType::PARAM_TYPE_NONE;
        std::variant<uint8_t, int32_t, float, bool, std::string, Color, KvList, ParamVector> value;
};
