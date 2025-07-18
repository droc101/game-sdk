//
// Created by droc101 on 6/26/25.
//

#include <libassets/util/DataWriter.h>
#include <cassert>

void DataWriter::CopyToVector(std::vector<uint8_t> &vector) const
{
    assert(vector.empty());
    vector.insert(vector.begin(), data.begin(), data.end());
}

size_t DataWriter::GetBufferSize() const
{
    return data.size();
}
