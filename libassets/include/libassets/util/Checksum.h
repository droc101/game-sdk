//
// Created by droc101 on 5/20/26.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <vector>

class Checksum
{
    public:
        /**
         * Calculate the checksum of a DataReader starting at a given offset
         * @param reader The DataReader to checksum
         * @param startOffset The start offset, defaulting to 0
         */
        static uint16_t Calculate(const DataReader &reader, size_t startOffset = 0);

        /**
         * Calculate the checksum of the data in a DataWriter
         */
        static uint16_t Calculate(const DataWriter &writer);

        /**
         * Calculate the checksum of a uint8_t vector starting at a given offset
         * @param bytes The vector to checksum
         * @param startOffset The start offset, defaulting to 0
         */
        static uint16_t Calculate(const std::vector<uint8_t> &bytes, size_t startOffset = 0);
};
