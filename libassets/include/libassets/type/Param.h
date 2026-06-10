//
// Created by droc101 on 9/5/25.
//

#pragma once

#include <concepts>
#include <cstddef>
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
                                                 std::same_as<T, ParamVector> ||
                                                 std::same_as<T, uint64_t> ||
                                                 std::same_as<T, glm::vec2> ||
                                                 std::same_as<T, glm::vec3>;

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
            PARAM_TYPE_ARRAY,
            PARAM_TYPE_UINT_64,
            PARAM_TYPE_VEC2,
            PARAM_TYPE_VEC3
        };

        Param();
        explicit Param(DataReader &reader);
        template<ParamTypeTemplate T> explicit Param(T value)
        {
            Set<T>(value);
        }
        explicit Param(nlohmann::ordered_json j);
        bool operator==(const Param &param) const;

        static inline const std::unordered_map<ParamType, std::string> PARAM_TYPE_NAMES = {
            {ParamType::PARAM_TYPE_BYTE, "byte"},
            {ParamType::PARAM_TYPE_INTEGER, "int"},
            {ParamType::PARAM_TYPE_FLOAT, "float"},
            {ParamType::PARAM_TYPE_BOOL, "bool"},
            {ParamType::PARAM_TYPE_STRING, "string"},
            {ParamType::PARAM_TYPE_NONE, "none"},
            {ParamType::PARAM_TYPE_COLOR, "Color"},
            {ParamType::PARAM_TYPE_ARRAY, "Array"},
            {ParamType::PARAM_TYPE_KV_LIST, "KvList"},
            {ParamType::PARAM_TYPE_UINT_64, "uint64"},
            {ParamType::PARAM_TYPE_VEC2, "vec2"},
            {ParamType::PARAM_TYPE_VEC3, "vec3"},
        };

        void Write(DataWriter &writer) const;

        /**
         * Get the ParamType from it's string representation
         */
        static ParamType ParseType(const std::string &type);

        /**
         * Clear this param
         */
        void Clear();

        /**
         * Reset this param to a default value for a given type
         * @param dataType The type to reset to
         */
        void ClearToType(ParamType dataType);

        /**
         * Get the type of this param
         */
        [[nodiscard]] ParamType GetType() const;

        /**
         * Get a JSON representation of this param
         */
        [[nodiscard]] nlohmann::ordered_json GetJson() const;

        /**
         * Get a string representation of this param's type
         */
        [[nodiscard]] std::string GetTypeName() const;

        /**
         * Get the value of this param
         * @tparam T The type you wish to get
         * @param defaultValue The default value to return if the value cannot be retrieved
         */
        template<ParamTypeTemplate T> [[nodiscard]] T Get(T defaultValue) const
        {
            if ((std::same_as<T, uint8_t> && type != ParamType::PARAM_TYPE_BYTE) ||
                (std::same_as<T, int32_t> && type != ParamType::PARAM_TYPE_INTEGER) ||
                (std::same_as<T, float> && type != ParamType::PARAM_TYPE_FLOAT) ||
                (std::same_as<T, bool> && type != ParamType::PARAM_TYPE_BOOL) ||
                (std::same_as<T, std::string> && type != ParamType::PARAM_TYPE_STRING) ||
                (std::same_as<T, Color> && type != ParamType::PARAM_TYPE_COLOR) ||
                (std::same_as<T, KvList> && type != ParamType::PARAM_TYPE_KV_LIST) ||
                (std::same_as<T, ParamVector> && type != ParamType::PARAM_TYPE_ARRAY) ||
                (std::same_as<T, uint64_t> && type != ParamType::PARAM_TYPE_UINT_64) ||
                (std::same_as<T, glm::vec2> && type != ParamType::PARAM_TYPE_VEC2) ||
                (std::same_as<T, glm::vec3> && type != ParamType::PARAM_TYPE_VEC3))
            {
                return defaultValue;
            }
            return std::get<T>(value);
        }

        /**
         * Get a reference to the value of this param
         * @tparam T The type you wish to get
         * @param defaultValue The default value to return if the value cannot be retrieved
         */
        template<ParamTypeTemplate T> [[nodiscard]] T &GetRef(T &defaultValue)
        {
            if ((std::same_as<T, uint8_t> && type != ParamType::PARAM_TYPE_BYTE) ||
                (std::same_as<T, int32_t> && type != ParamType::PARAM_TYPE_INTEGER) ||
                (std::same_as<T, float> && type != ParamType::PARAM_TYPE_FLOAT) ||
                (std::same_as<T, bool> && type != ParamType::PARAM_TYPE_BOOL) ||
                (std::same_as<T, std::string> && type != ParamType::PARAM_TYPE_STRING) ||
                (std::same_as<T, Color> && type != ParamType::PARAM_TYPE_COLOR) ||
                (std::same_as<T, KvList> && type != ParamType::PARAM_TYPE_KV_LIST) ||
                (std::same_as<T, ParamVector> && type != ParamType::PARAM_TYPE_ARRAY) ||
                (std::same_as<T, uint64_t> && type != ParamType::PARAM_TYPE_UINT_64) ||
                (std::same_as<T, glm::vec2> && type != ParamType::PARAM_TYPE_VEC2) ||
                (std::same_as<T, glm::vec3> && type != ParamType::PARAM_TYPE_VEC3))
            {
                return defaultValue;
            }
            return std::get<T>(value);
        }

        /**
         * Get a pointer to the value of this param
         * @tparam T The type you wish to get
         * @note This can return nullptr
         */
        template<ParamTypeTemplate T> [[nodiscard]] T *GetPointer()
        {
            if ((std::same_as<T, uint8_t> && type != ParamType::PARAM_TYPE_BYTE) ||
                (std::same_as<T, int32_t> && type != ParamType::PARAM_TYPE_INTEGER) ||
                (std::same_as<T, float> && type != ParamType::PARAM_TYPE_FLOAT) ||
                (std::same_as<T, bool> && type != ParamType::PARAM_TYPE_BOOL) ||
                (std::same_as<T, std::string> && type != ParamType::PARAM_TYPE_STRING) ||
                (std::same_as<T, Color> && type != ParamType::PARAM_TYPE_COLOR) ||
                (std::same_as<T, KvList> && type != ParamType::PARAM_TYPE_KV_LIST) ||
                (std::same_as<T, ParamVector> && type != ParamType::PARAM_TYPE_ARRAY) ||
                (std::same_as<T, uint64_t> && type != ParamType::PARAM_TYPE_UINT_64) ||
                (std::same_as<T, glm::vec2> && type != ParamType::PARAM_TYPE_VEC2) ||
                (std::same_as<T, glm::vec3> && type != ParamType::PARAM_TYPE_VEC3))
            {
                return nullptr;
            }
            return &std::get<T>(value);
        }

        /**
         * Set the value of this param
         * @tparam T The type of value to set
         * @param newValue The value to set
         */
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
            } else if (std::same_as<T, uint64_t>)
            {
                value = newValue;
                type = ParamType::PARAM_TYPE_UINT_64;
            } else if (std::same_as<T, glm::vec2>)
            {
                value = newValue;
                type = ParamType::PARAM_TYPE_VEC2;
            } else if (std::same_as<T, glm::vec3>)
            {
                value = newValue;
                type = ParamType::PARAM_TYPE_VEC3;
            }
        }

        /**
         * Write a KvList to a DataWriter
         */
        static void WriteKvList(DataWriter &writer, const KvList &list);

        /**
         * Read a KvList from a DataReader
         */
        static KvList ReadKvList(DataReader &reader);

        /**
         * Create a JSON representation of a KVList
         */
        static nlohmann::ordered_json GenerateKvListJson(const KvList &list);

        /**
         * Create a KvList from JSON
         */
        static KvList KvListFromJson(const nlohmann::ordered_json &json);

        /**
         * Get the pointer to an element of an array param
         */
        Param *ArrayElementPointer(size_t index);

        /**
         * Get the pointer to a value in a KvList
         * @param key The key to get the value of
         */
        Param *KvListElementPointer(const std::string &key);

        /**
         * Get a string representation of this param
         * @return
         */
        [[nodiscard]] std::string ToString() const;

    private:
        ParamType type = ParamType::PARAM_TYPE_NONE;
        std::variant<uint8_t,
                     int32_t,
                     float,
                     bool,
                     std::string,
                     Color,
                     KvList,
                     ParamVector,
                     uint64_t,
                     glm::vec2,
                     glm::vec3>
                value;
};
