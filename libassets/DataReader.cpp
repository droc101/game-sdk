//
// Created by droc101 on 6/26/25.
//

#include "include/libassets/DataReader.h"
#include <cassert>
#include <cstring>

DataReader::DataReader(const uint8_t *bytes, const size_t dataSize)
{
    this->bytes = bytes;
    this->size = dataSize;
    this->offset = 0;
}

DataReader::~DataReader()
{
    delete[] bytes;
}


void DataReader::Seek(const std::ptrdiff_t offset)
{
    this->offset += offset;
}

void DataReader::SeekAbsolute(const size_t position)
{
    offset = position;
}

int8_t DataReader::Read8()
{
    assert(offset + sizeof(int8_t) <= size);
    const int8_t i = static_cast<int8_t>(bytes[offset]);
    offset += sizeof(int8_t);
    return i;
}

uint8_t DataReader::ReadU8()
{
    assert(offset + sizeof(uint8_t) <= size);
    const uint8_t i = bytes[offset];
    offset += sizeof(uint8_t);
    return i;
}

int16_t DataReader::Read16()
{
    assert(offset + sizeof(int16_t) <= size);
    int16_t i;
    std::memcpy(&i, bytes + offset, sizeof(int16_t));
    offset += sizeof(int16_t);
    return i;
}
uint8_t DataReader::ReadU16()
{
    assert(offset + sizeof(uint16_t) <= size);
    uint16_t i;
    std::memcpy(&i, bytes + offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    return i;
}

int32_t DataReader::Read32()
{
    assert(offset + sizeof(int32_t) <= size);
    int32_t i;
    std::memcpy(&i, bytes + offset, sizeof(int32_t));
    offset += sizeof(int32_t);
    return i;
}

uint32_t DataReader::ReadU32()
{
    assert(offset + sizeof(uint32_t) <= size);
    uint32_t i;
    std::memcpy(&i, bytes + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    return i;
}

int64_t DataReader::Read64()
{
    assert(offset + sizeof(int64_t) <= size);
    int64_t i;
    std::memcpy(&i, bytes + offset, sizeof(int64_t));
    offset += sizeof(int64_t);
    return i;
}

uint64_t DataReader::ReadU64()
{
    assert(offset + sizeof(uint64_t) <= size);
    uint64_t i;
    std::memcpy(&i, bytes + offset, sizeof(uint64_t));
    offset += sizeof(uint64_t);
    return i;
}

float DataReader::ReadFloat()
{
    assert(offset + sizeof(float) <= size);
    float i;
    std::memcpy(&i, bytes + offset, sizeof(float));
    offset += sizeof(float);
    return i;
}

double DataReader::ReadDouble()
{
    assert(offset + sizeof(double) <= size);
    double i;
    std::memcpy(&i, bytes + offset, sizeof(double));
    offset += sizeof(double);
    return i;
}

char *DataReader::ReadString(const size_t length)
{
    assert(offset + sizeof(char) * length <= size);
    char *str = new char[length];
    std::memcpy(str, bytes + offset, sizeof(char) * length);
    offset += sizeof(char) * length;
    return str;
}

uint8_t *DataReader::ReadBytes(size_t length)
{
    assert(offset + sizeof(uint8_t) * length <= size);
    uint8_t *buf = new uint8_t[length];
    std::memcpy(buf, bytes + offset, sizeof(uint8_t) * length);
    offset += sizeof(uint8_t) * length;
    return buf;
}
