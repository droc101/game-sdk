//
// Created by droc101 on 6/26/25.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <format>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <libassets/util/Primitive.h>
#include <stdexcept>
#include <string>
#include <vector>

class AssetReader;
class Checksum;

class DataReader
{
    public:
        friend AssetReader;
        friend Checksum;

        DataReader() = default;

        explicit DataReader(size_t dataSize);

        explicit DataReader(const std::vector<uint8_t> &data);

        /**
         * Seek by a given offset
         */
        void Seek(std::ptrdiff_t relativeOffset);

        /**
         * Seek to an absolute position
         */
        void SeekAbsolute(size_t position);

        /**
         * Get the total size of the data
         */
        [[nodiscard]] size_t TotalSize() const;

        /**
         * Get the remaining size
         */
        [[nodiscard]] size_t RemainingSize() const;

        /**
         * Read a fixed-length string
         * @param buffer The buffer to read into
         * @param characterCount The size of the string
         */
        void ReadString(std::string &buffer, size_t characterCount);

        /**
         * Read a dynamic size string
         * @param buffer The buffer to read into
         */
        void ReadStringWithSize(std::string &buffer);

        /**
         * Read a vec2
         */
        [[nodiscard]] glm::vec2 ReadVec2();

        /**
         * Read a vec3
         */
        [[nodiscard]] glm::vec3 ReadVec3();

        /**
         * Read a primitive
         * @tparam T The data type to read
         */
        template<Primitive T> [[nodiscard]] T Read()
        {
            if (offset + sizeof(T) > size)
            {
                throw std::runtime_error(std::format("Attempting to read past the end of a buffer (buffer size {}, "
                                                     "cursor position {}, read size {}",
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
                throw std::runtime_error(std::format("Attempting to read past the end of a buffer (buffer size {}, "
                                                     "cursor position {}, read size {}",
                                                     size,
                                                     offset,
                                                     sizeof(T)));
            }
            offset += sizeof(T);
        }

        /**
         * Read an array of primitives
         * @tparam T The type to read
         * @param buffer The buffer to populate
         * @param numberToRead The number of elements to read
         */
        template<Primitive T> void ReadToVector(std::vector<T> &buffer, const size_t numberToRead)
        {
            if (offset + sizeof(T) * numberToRead > size)
            {
                throw std::runtime_error(std::format("Attempting to read past the end of a buffer (buffer size {}, "
                                                     "cursor position {}, read size {}",
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
