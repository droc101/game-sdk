//
// Created by droc101 on 6/26/25.
//

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <libassets/util/DataWriter.h>
#include <string>
#include <vector>

void DataWriter::CopyToVector(std::vector<uint8_t> &vector) const
{
    assert(vector.empty());
    vector.insert(vector.begin(), data.begin(), data.end());
}

void DataWriter::WriteString(const std::string &str)
{
    const size_t strLength = str.length() + 1;
    Write<size_t>(strLength);
    WriteBuffer<const char>(str.c_str(), strLength);
}


size_t DataWriter::GetBufferSize() const
{
    return data.size();
}
