//
// Created by droc101 on 6/26/25.
//

#include <libassets/util/DataReader.h>
#include <cassert>

DataReader::DataReader(const size_t dataSize): bytes(dataSize)
{
    size = dataSize;
    offset = 0;
}

void DataReader::Seek(const std::ptrdiff_t relativeOffset)
{
    offset += relativeOffset;
}

void DataReader::SeekAbsolute(const size_t position)
{
    offset = position;
}

size_t DataReader::TotalSize() const
{
    return size;
}

size_t DataReader::RemainingSize() const
{
    return size - offset;
}

void DataReader::ReadString(std::string &buffer, const size_t characterCount)
{
    if (offset + sizeof(char) * characterCount > size)
    {
        throw std::runtime_error(std::format(
                "Attempting to read past the end of a buffer (buffer size {}, cursor position {}, read size {}",
                size,
                offset,
                sizeof(char) * characterCount));
    }
    if (!buffer.empty())
    {
        throw std::runtime_error("Attempting to read into a non-empty buffer!");
    }

    buffer.insert(buffer.begin(), &bytes.at(offset), &bytes.at(offset + characterCount - 1));
    offset += sizeof(char) * characterCount;
}
