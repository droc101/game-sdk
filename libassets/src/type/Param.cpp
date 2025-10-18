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

void Param::Write(DataWriter &writer)
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
