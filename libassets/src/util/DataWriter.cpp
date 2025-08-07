//
// Created by droc101 on 6/26/25.
//

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <libassets/util/DataWriter.h>
#include <vector>

void DataWriter::CopyToVector(std::vector<uint8_t> &vector) const
{
    assert(vector.empty());
    vector.insert(vector.begin(), data.begin(), data.end());
}

size_t DataWriter::GetBufferSize() const
{
    return data.size();
}
