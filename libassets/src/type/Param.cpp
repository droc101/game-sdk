//
// Created by droc101 on 9/5/25.
//

#include <cstdint>
#include <libassets/type/Color.h>
#include <libassets/type/Param.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <string>

Param::Param()
{
    type = ParamType::PARAM_TYPE_NONE;
}

Param::Param(DataReader &reader)
{
    type = static_cast<ParamType>(reader.Read<uint8_t>());
    std::string buf;
    switch (type)
    {
        case ParamType::PARAM_TYPE_BYTE:
            Set<uint8_t>(reader.Read<uint8_t>());
            break;
        case ParamType::PARAM_TYPE_INTEGER:
            Set<int32_t>(reader.Read<int32_t>());
            break;
        case ParamType::PARAM_TYPE_FLOAT:
            Set<float>(reader.Read<float>());
            break;
        case ParamType::PARAM_TYPE_BOOL:
            Set<bool>(reader.Read<bool>());
            break;
        case ParamType::PARAM_TYPE_STRING:
            reader.ReadStringWithSize(buf);
            Set<std::string>(buf);
            break;
        case ParamType::PARAM_TYPE_COLOR:
            Set<Color>(Color(reader, true));
            break;
        case ParamType::PARAM_TYPE_NONE:
        default:
            break;
    }
}

Param::Param(nlohmann::ordered_json j)
{
    type = ParseType(j.value("type", "none"));
    switch (type)
    {
        case ParamType::PARAM_TYPE_BYTE:
            Set<uint8_t>(j["value"]);
            break;
        case ParamType::PARAM_TYPE_INTEGER:
            Set<int32_t>(j["value"]);
            break;
        case ParamType::PARAM_TYPE_FLOAT:
            Set<float>(j["value"]);
            break;
        case ParamType::PARAM_TYPE_BOOL:
            Set<bool>(j["value"]);
            break;
        case ParamType::PARAM_TYPE_STRING:
            Set<std::string>(j["value"]);
            break;
        case ParamType::PARAM_TYPE_COLOR:
            Set<Color>(Color(j["value"]));
            break;
        case ParamType::PARAM_TYPE_NONE:
        default:
            break;
    }
}


bool Param::operator==(const Param &param) const
{
    if (type != param.type)
    {
        return false;
    }
    switch (param.type)
    {
        case ParamType::PARAM_TYPE_BOOL:
            return Get<bool>(false) == param.Get<bool>(false);
        case ParamType::PARAM_TYPE_BYTE:
            return Get<uint8_t>(0) == param.Get<uint8_t>(0);
        case ParamType::PARAM_TYPE_INTEGER:
            return Get<int32_t>(0) == param.Get<int32_t>(0);
        case ParamType::PARAM_TYPE_FLOAT:
            return Get<float>(0) == param.Get<float>(0);
        case ParamType::PARAM_TYPE_STRING:
            return Get<std::string>("") == param.Get<std::string>("");
        case ParamType::PARAM_TYPE_COLOR:
            return Get<Color>(Color(-1)) == param.Get<Color>(Color(-1));
        default:
            return true;
    }
}

void Param::Write(DataWriter &writer) const
{
    writer.Write<uint8_t>(static_cast<uint8_t>(type));
    switch (type)
    {
        case ParamType::PARAM_TYPE_BYTE:
            writer.Write<uint8_t>(Get<uint8_t>(0));
            break;
        case ParamType::PARAM_TYPE_INTEGER:
            writer.Write<int32_t>(Get<int32_t>(0));
            break;
        case ParamType::PARAM_TYPE_FLOAT:
            writer.Write<float>(Get<float>(0.0f));
            break;
        case ParamType::PARAM_TYPE_BOOL:
            writer.Write<bool>(Get<bool>(false));
            break;
        case ParamType::PARAM_TYPE_STRING:
            writer.WriteString(Get<std::string>(""));
            break;
        case ParamType::PARAM_TYPE_COLOR:
            Get<Color>(Color()).WriteFloats(writer);
            break;
        case ParamType::PARAM_TYPE_NONE:
        default:
            break;
    }
}

Param::ParamType Param::ParseType(const std::string &type)
{
    if (type == "byte")
    {
        return ParamType::PARAM_TYPE_BYTE;
    }
    if (type == "int" || type == "integer")
    {
        return ParamType::PARAM_TYPE_INTEGER;
    }
    if (type == "float" || type == "decimal")
    {
        return ParamType::PARAM_TYPE_FLOAT;
    }
    if (type == "bool" || type == "boolean")
    {
        return ParamType::PARAM_TYPE_BOOL;
    }
    if (type == "string" || type == "str")
    {
        return ParamType::PARAM_TYPE_STRING;
    }
    if (type == "color" || type == "col")
    {
        return ParamType::PARAM_TYPE_COLOR;
    }
    return ParamType::PARAM_TYPE_NONE;
}

void Param::Clear()
{
    type = ParamType::PARAM_TYPE_NONE;
    value = static_cast<uint8_t>(0);
}

void Param::ClearToType(const ParamType dataType)
{
    switch (dataType)
    {
        case ParamType::PARAM_TYPE_BYTE:
            Set<uint8_t>(0);
            break;
        case ParamType::PARAM_TYPE_INTEGER:
            Set<int32_t>(0);
            break;
        case ParamType::PARAM_TYPE_FLOAT:
            Set<float>(0);
            break;
        case ParamType::PARAM_TYPE_BOOL:
            Set<bool>(false);
            break;
        case ParamType::PARAM_TYPE_STRING:
            Set<std::string>("");
            break;
        case ParamType::PARAM_TYPE_COLOR:
            Set<Color>(Color(-1));
            break;
        case ParamType::PARAM_TYPE_NONE:
        default:
            Clear();
            break;
    }
}


Param::ParamType Param::GetType() const
{
    return type;
}

nlohmann::ordered_json Param::GetJson() const
{
    nlohmann::ordered_json j{};
    switch (GetType())
    {
        case ParamType::PARAM_TYPE_BYTE:
            j["type"] = "byte";
            j["value"] = Get<uint8_t>(0);
            break;
        case ParamType::PARAM_TYPE_INTEGER:
            j["type"] = "int";
            j["value"] = Get<int32_t>(0);
            break;
        case ParamType::PARAM_TYPE_FLOAT:
            j["type"] = "float";
            j["value"] = Get<float>(0);
            break;
        case ParamType::PARAM_TYPE_BOOL:
            j["type"] = "bool";
            j["value"] = Get<bool>(false);
            break;
        case ParamType::PARAM_TYPE_STRING:
            j["type"] = "string";
            j["value"] = Get<std::string>("");
            break;
        case ParamType::PARAM_TYPE_COLOR:
            j["type"] = "color";
            j["value"] = Get<Color>(Color(-1)).GenerateJson();
            break;
        case ParamType::PARAM_TYPE_NONE:
        default:
            j["type"] = "none";
            break;
    }
    return j;
}
