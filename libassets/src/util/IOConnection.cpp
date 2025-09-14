//
// Created by droc101 on 9/5/25.
//

#include <libassets/util/IOConnection.h>

void IOConnection::Write(DataWriter &writer) const
{
    writer.WriteString(sourceOutput);
    writer.WriteString(targetName);
    writer.WriteString(targetInput);
    // TODO write param
}

