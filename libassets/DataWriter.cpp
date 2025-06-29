//
// Created by droc101 on 6/26/25.
//

#include "include/libassets/DataWriter.h"

uint8_t *DataWriter::GetBuffer() const
{
    uint8_t *dataCopy = new uint8_t[data.size()];
    std::memcpy(dataCopy, data.data(), data.size());
    return dataCopy;
}

size_t DataWriter::GetBufferSize() const
{
    return data.size();
}

