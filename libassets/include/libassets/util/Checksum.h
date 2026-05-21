//
// Created by droc101 on 5/20/26.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>

class Checksum
{
    public:
        static uint16_t Calculate(const DataReader &reader, size_t startOffset = 0);
        static uint16_t Calculate(const DataWriter &writer);
        static uint16_t Calculate(const std::vector<uint8_t> &bytes, size_t startOffset = 0);
};
