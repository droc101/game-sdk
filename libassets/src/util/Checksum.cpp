//
// Created by droc101 on 5/20/26.
//

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <libassets/util/Checksum.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <vector>

uint16_t Checksum::Calculate(const std::vector<uint8_t> &bytes, const size_t startOffset)
{
    assert(startOffset < bytes.size());
    uint16_t checksum = 5873 + ((bytes.size() - startOffset) % 2367);
    for (size_t i = startOffset; i < bytes.size(); i++)
    {
        checksum += bytes.at(i);
    }
    return checksum;
}

uint16_t Checksum::Calculate(const DataWriter &writer)
{
    return Calculate(writer.data);
}

uint16_t Checksum::Calculate(const DataReader &reader, const size_t startOffset)
{
    return Calculate(reader.bytes, startOffset);
}
