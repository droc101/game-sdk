//
// Created by droc101 on 9/5/25.
//

#include <cstddef>
#include <cstdint>
#include <libassets/type/Color.h>
#include <libassets/type/Param.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <string>
#include <unordered_map>
#include <utility>

Param::Param()
{
    type = ParamType::PARAM_TYPE_NONE;
}

Param::Param(DataReader &reader)
{
    type = static_cast<ParamType>(reader.Read<uint8_t>());
    std::string buf;
    std::size_t len;
    ParamVector vec{};
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
        case ParamType::PARAM_TYPE_ARRAY:
            len = reader.Read<size_t>();
            for (size_t i = 0; i < len; i++)
            {
                vec.emplace_back(reader);
            }
            Set<ParamVector>(vec);
            break;
        case ParamType::PARAM_TYPE_KV_LIST:
            Set<KvList>(ReadKvList(reader));
            break;
        case ParamType::PARAM_TYPE_UINT_64:
            Set<uint64_t>(reader.Read<uint64_t>());
            break;
        case ParamType::PARAM_TYPE_NONE:
        default:
            break;
    }
}

Param::Param(nlohmann::ordered_json j)
{
    ParamVector vec{};
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
        case ParamType::PARAM_TYPE_ARRAY:
            for (const nlohmann::ordered_json &element: j["data"])
            {
                vec.emplace_back(element);
            }
            Set<ParamVector>(vec);
            break;
        case ParamType::PARAM_TYPE_KV_LIST:
            Set<KvList>(KvListFromJson(j["data"]));
            break;
        case ParamType::PARAM_TYPE_UINT_64:
            Set<uint64_t>(j["value"]);
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
        case ParamType::PARAM_TYPE_ARRAY:
            return Get<ParamVector>(ParamVector()) == param.Get<ParamVector>(ParamVector());
        case ParamType::PARAM_TYPE_KV_LIST:
            return Get<KvList>(KvList()) == param.Get<KvList>(KvList());
        case ParamType::PARAM_TYPE_UINT_64:
            return Get<uint64_t>(0) == param.Get<uint64_t>(0);
        default:
            return true;
    }
}

void Param::Write(DataWriter &writer) const
{
    ParamVector vec;
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
        case ParamType::PARAM_TYPE_ARRAY:
            vec = Get<ParamVector>(ParamVector());
            writer.Write<size_t>(vec.size());
            for (const Param &p: vec)
            {
                p.Write(writer);
            }
            break;
        case ParamType::PARAM_TYPE_KV_LIST:
            WriteKvList(writer, Get<KvList>(KvList()));
            break;
        case ParamType::PARAM_TYPE_UINT_64:
            writer.Write<uint64_t>(Get<uint64_t>(0));
            break;
        case ParamType::PARAM_TYPE_NONE:
        default:
            break;
    }
}

void Param::WriteKvList(DataWriter &writer, const KvList &list)
{
    writer.Write<size_t>(list.size());
    for (const std::pair<const std::string, Param> &kv: list)
    {
        writer.WriteString(kv.first);
        kv.second.Write(writer);
    }
}

KvList Param::ReadKvList(DataReader &reader)
{
    KvList list{};
    const size_t len = reader.Read<size_t>();
    for (size_t i = 0; i < len; i++)
    {
        std::string key{};
        reader.ReadStringWithSize(key);
        list[key] = Param(reader);
    }
    return list;
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
    if (type == "array" || type == "list" || type == "vector")
    {
        return ParamType::PARAM_TYPE_ARRAY;
    }
    if (type == "dict" || type == "dictionary" || type == "object" || type == "map")
    {
        return ParamType::PARAM_TYPE_KV_LIST;
    }
    if (type == "uint64" || type == "uint64_t" || type == "size_t")
    {
        return ParamType::PARAM_TYPE_UINT_64;
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
        case ParamType::PARAM_TYPE_ARRAY:
            Set<ParamVector>(ParamVector());
            break;
        case ParamType::PARAM_TYPE_KV_LIST:
            Set<KvList>(KvList());
            break;
        case ParamType::PARAM_TYPE_UINT_64:
            Set<uint64_t>(0);
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
    nlohmann::ordered_json array;
    ParamVector vec;
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
        case ParamType::PARAM_TYPE_ARRAY:
            array = nlohmann::ordered_json::array();
            vec = Get<ParamVector>(ParamVector());
            for (const Param &p: vec)
            {
                array.push_back(p.GetJson());
            }
            j["data"] = array;
            j["type"] = "array";
            break;
        case ParamType::PARAM_TYPE_KV_LIST:
            j["data"] = GenerateKvListJson(Get<KvList>(KvList()));
            j["type"] = "dict";
            break;
        case ParamType::PARAM_TYPE_UINT_64:
            j["type"] = "uint64";
            j["value"] = Get<uint64_t>(0);
            break;
        case ParamType::PARAM_TYPE_NONE:
        default:
            j["type"] = "none";
            break;
    }
    return j;
}

std::string Param::GetTypeName() const
{
    return paramTypeNames.at(GetType());
}

nlohmann::ordered_json Param::GenerateKvListJson(const KvList &list)
{
    nlohmann::ordered_json json = nlohmann::ordered_json::object();
    for (const std::pair<const std::string, Param> &pair: list)
    {
        json[pair.first] = pair.second.GetJson();
    }
    return json;
}

KvList Param::KvListFromJson(const nlohmann::ordered_json &json)
{
    KvList params{};
    for (const auto &[key, value]: json.items())
    {
        params[key] = Param(value);
    }
    return params;
}

Param *Param::ArrayElementPointer(const size_t index)
{
    if (type != ParamType::PARAM_TYPE_ARRAY)
    {
        return nullptr;
    }
    ParamVector &vec = std::get<ParamVector>(value);
    if (index >= vec.size())
    {
        return nullptr;
    }
    return &vec.at(index);
}

Param *Param::KvListElementPointer(const std::string &key)
{
    if (type != ParamType::PARAM_TYPE_KV_LIST)
    {
        return nullptr;
    }
    KvList &list = std::get<KvList>(value);
    if (!list.contains(key))
    {
        return nullptr;
    }
    return &list.at(key);
}
