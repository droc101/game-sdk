//
// Created by droc101 on 9/5/25.
//

#include <cstddef>
#include <cstdint>
#include <libassets/type/IOConnection.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <nlohmann/json.hpp>

IOConnection::IOConnection(const nlohmann::ordered_json &j)
{
    targetName = j.value("targetName", "");
    targetInput = j.value("targetInput", "");
    sourceOutput = j.value("sourceOutput", "");
    numRefires = j.value("numRefires", 0);
    overridesParam = j.value("overridesParam", false);
}

IOConnection::IOConnection(DataReader &reader)
{
    reader.ReadStringWithSize(sourceOutput);
    reader.ReadStringWithSize(targetName);
    reader.ReadStringWithSize(targetInput);
    overridesParam = reader.Read<uint8_t>() == 1;
    param = Param(reader);
    numRefires = reader.Read<size_t>();
}


void IOConnection::Write(DataWriter &writer) const
{
    writer.WriteString(sourceOutput);
    writer.WriteString(targetName);
    writer.WriteString(targetInput);
    writer.Write<uint8_t>(overridesParam ? 1 : 0);
    param.Write(writer);
    writer.Write<size_t>(numRefires);
}

nlohmann::ordered_json IOConnection::GenerateJson() const
{
    nlohmann::ordered_json j{};
    j["targetName"] = targetName;
    j["targetInput"] = targetInput;
    j["sourceOutput"] = sourceOutput;
    j["numRefires"] = numRefires;
    j["overridesParam"] = overridesParam;
    j["param"] = param.GetJson();
    return j;
}
