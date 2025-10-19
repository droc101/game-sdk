//
// Created by droc101 on 9/5/25.
//

#include <libassets/type/IOConnection.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>

IOConnection::IOConnection(DataReader &reader)
{
    reader.ReadStringWithSize(sourceOutput);
    reader.ReadStringWithSize(targetName);
    reader.ReadStringWithSize(targetInput);
    param = Param(reader);
}


void IOConnection::Write(DataWriter &writer) const
{
    writer.WriteString(sourceOutput);
    writer.WriteString(targetName);
    writer.WriteString(targetInput);
    param.Write(writer);
}
