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

        [[nodiscard]] uint8_t ReadU8();

        [[nodiscard]] int8_t Read8();

        [[nodiscard]] uint16_t ReadU16();

        [[nodiscard]] int16_t Read16();

        [[nodiscard]] uint32_t ReadU32();

        [[nodiscard]] int32_t Read32();

        [[nodiscard]] uint64_t ReadU64();

        [[nodiscard]] int64_t Read64();

        [[nodiscard]] float ReadFloat();

        [[nodiscard]] double ReadDouble();

        void ReadString(std::string &buffer, size_t characterCount);

        template<typename T> void ReadToBuffer(std::vector<T> &buffer, const size_t numberToRead)
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
