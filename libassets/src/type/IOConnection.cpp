//
// Created by droc101 on 9/5/25.
//

#include <cstddef>
#include <cstdint>
#include <libassets/type/IOConnection.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>

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
