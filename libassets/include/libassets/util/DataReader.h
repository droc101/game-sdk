//
// Created by droc101 on 6/26/25.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <format>
#include <stdexcept>
#include <string>
#include <vector>
#include <libassets/util/Primitive.h>

class AssetReader;

class DataReader
{
    public:
        friend AssetReader;

        DataReader() = default;

        explicit DataReader(size_t dataSize);

        explicit DataReader(const std::vector<uint8_t> &data);

        void Seek(std::ptrdiff_t relativeOffset);

        void SeekAbsolute(size_t position);

        [[nodiscard]] size_t TotalSize() const;

        [[nodiscard]] size_t RemainingSize() const;

        void ReadString(std::string &buffer, size_t characterCount);

        template<Primitive T> [[nodiscard]] T Read()
        {
            if (offset + sizeof(T) > size)
            {
                throw std::runtime_error(std::format(
                        "Attempting to read past the end of a buffer (buffer size {}, cursor position {}, read size {}",
                        size,
                        offset,
                        sizeof(T)));
            }
            const T i = *reinterpret_cast<const T *>(&bytes.at(offset));
            offset += sizeof(T);
            return i;
        }

        template<Primitive T> void Skip()
        {
            if (offset + sizeof(T) > size)
            {
                throw std::runtime_error(std::format(
                        "Attempting to read past the end of a buffer (buffer size {}, cursor position {}, read size {}",
                        size,
                        offset,
                        sizeof(T)));
            }
            offset += sizeof(T);
        }

        template<Primitive T> void ReadToBuffer(std::vector<T> &buffer, const size_t numberToRead)
        {
            if (offset + sizeof(T) * numberToRead > size)
            {
                throw std::runtime_error(std::format(
                        "Attempting to read past the end of a buffer (buffer size {}, cursor position {}, read size {}",
                        size,
                        offset,
                        sizeof(T) * numberToRead));
            }
            if (!buffer.empty())
            {
                throw std::runtime_error("Attempting to read into a non-empty buffer!");
            }
            T *offsetData = reinterpret_cast<T *>(bytes.data() + offset);
            buffer.insert(buffer.begin(), offsetData, offsetData + numberToRead);
            offset += numberToRead;
        }

    protected:
        std::vector<uint8_t> bytes{};
        size_t size{};
        size_t offset{};
};
