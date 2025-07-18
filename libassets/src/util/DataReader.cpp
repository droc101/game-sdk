//
// Created by droc101 on 6/26/25.
//

#include <libassets/util/DataReader.h>
#include <algorithm>
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

int8_t DataReader::Read8()
{
    static_assert(sizeof(int8_t) == 1);
    assert(offset < size);
    const int8_t i = static_cast<int8_t>(bytes.at(offset));
    offset++;
    return i;
}

uint8_t DataReader::ReadU8()
{
    static_assert(sizeof(uint8_t) == 1);
    assert(offset < size);
    const uint8_t i = bytes.at(offset);
    offset++;
    return i;
}

int16_t DataReader::Read16()
{
    assert(offset + sizeof(int16_t) <= size);
    const int16_t i = *reinterpret_cast<const int16_t *>(&bytes.at(offset));
    offset += sizeof(int16_t);
    return i;
}
uint16_t DataReader::ReadU16()
{
    assert(offset + sizeof(uint16_t) <= size);
    const uint16_t i = *reinterpret_cast<const uint16_t *>(&bytes.at(offset));
    offset += sizeof(uint16_t);
    return i;
}

int32_t DataReader::Read32()
{
    assert(offset + sizeof(int32_t) <= size);
    const int32_t i = *reinterpret_cast<const int32_t *>(&bytes.at(offset));
    offset += sizeof(int32_t);
    return i;
}

uint32_t DataReader::ReadU32()
{
    assert(offset + sizeof(uint32_t) <= size);
    const uint32_t i = *reinterpret_cast<const uint32_t *>(&bytes.at(offset));
    offset += sizeof(uint32_t);
    return i;
}

int64_t DataReader::Read64()
{
    assert(offset + sizeof(int64_t) <= size);
    const int64_t i = *reinterpret_cast<const int64_t *>(&bytes.at(offset));
    offset += sizeof(int64_t);
    return i;
}

uint64_t DataReader::ReadU64()
{
    assert(offset + sizeof(uint64_t) <= size);
    const uint64_t i = *reinterpret_cast<const uint64_t *>(&bytes.at(offset));
    offset += sizeof(uint64_t);
    return i;
}

float DataReader::ReadFloat()
{
    assert(offset + sizeof(float) <= size);
    const float i = *reinterpret_cast<const float *>(&bytes.at(offset));
    offset += sizeof(float);
    return i;
}

double DataReader::ReadDouble()
{
    assert(offset + sizeof(double) <= size);
    const double i = *reinterpret_cast<const double *>(&bytes.at(offset));
    offset += sizeof(double);
    return i;
}

void DataReader::ReadString(std::string &buffer, const size_t characterCount)
{
    assert(offset + sizeof(char) * characterCount <= size);
    assert(buffer.empty());
    buffer.insert(buffer.begin(), &bytes.at(offset), &bytes.at(offset + characterCount - 1));
    offset += sizeof(char) * characterCount;
}
