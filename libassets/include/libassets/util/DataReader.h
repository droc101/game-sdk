//
// Created by droc101 on 6/26/25.
//

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <libassets/util/AssetReader.h>
#include <libassets/util/Primitive.h>

class DataReader
{
    public:
        friend AssetReader::AssetType AssetReader::Decompress(std::vector<uint8_t> &, DataReader &);

        DataReader() = default;

        explicit DataReader(size_t dataSize);

        void Seek(std::ptrdiff_t relativeOffset);

        void SeekAbsolute(size_t position);

        [[nodiscard]] size_t TotalSize() const;

        [[nodiscard]] size_t RemainingSize() const;

        void ReadString(std::string &buffer, size_t characterCount);

        template <Primitive T> [[nodiscard]] T Read()
        {
            assert(offset + sizeof(T) <= size);
            const T i = *reinterpret_cast<const T *>(&bytes.at(offset));
            offset += sizeof(T);
            return i;
        }

        template<Primitive T> void ReadToBuffer(std::vector<T> &buffer, const size_t numberToRead)
        {
            static_assert(sizeof(uint8_t) == 1);
            assert(offset + sizeof(T) * numberToRead <= size);
            assert(buffer.empty());
            T *offsetData = reinterpret_cast<T *>(bytes.data() + offset);
            buffer.insert(buffer.begin(), offsetData, offsetData + numberToRead);
            offset += numberToRead;
        }

    private:
        std::vector<uint8_t> bytes{};
        size_t size{};
        size_t offset{};
};
