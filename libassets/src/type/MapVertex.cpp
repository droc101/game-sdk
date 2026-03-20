//
// Created by NBT22 on 3/14/26.
//

#include <libassets/type/MapVertex.h>
#include <libassets/util/DataWriter.h>

void MapVertex::Write(DataWriter &writer) const
{
    writer.WriteVec3(position);
    writer.WriteVec2(uv);
    writer.WriteVec2(lightmapUv);
}
