//
// Created by droc101 on 6/26/25.
//

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/vec2.hpp>
#include <libassets/libassets.h>
#include <libassets/util/Primitive.h>
#include <string>
#include <vector>

class DataWriter
{
    public:
        DataWriter() = default;

        /**
         * Write a value to the buffer
         * @tparam T The type of value
         * @param value The value to write
         */
        template<Primitive T> void Write(T value)
        {
            const uint8_t *valueAsUint = reinterpret_cast<const uint8_t *>(&value);
            data.insert(data.end(), valueAsUint, valueAsUint + sizeof(T));
        }

        /**
         * Write an array of same-type values to the buffer
         * @tparam T The type of value in the array
         * @param buffer The array
         * @param length The number of elements in the array
         */
        template<Primitive T> void WriteBuffer(T *buffer, const size_t length)
        {
            const uint8_t *buf = reinterpret_cast<const uint8_t *>(buffer);
            data.insert(data.end(), buf, buf + length * sizeof(T));
        }

        template<Primitive T> void WriteBuffer(const std::vector<T> &buffer)
        {
            const uint8_t *bufferData = reinterpret_cast<const uint8_t *>(buffer.data());
            data.insert(data.end(), bufferData, bufferData + buffer.size() * sizeof(T));
        }

        template<Primitive T, size_t length> void WriteBuffer(const std::array<T, length> &buffer)
        {
            const uint8_t *bufferData = reinterpret_cast<const uint8_t *>(buffer.data());
            data.insert(data.end(), bufferData, bufferData + length * sizeof(T));
        }

        void CopyToVector(std::vector<uint8_t> &vector) const;

        void WriteString(const std::string &str);

        [[nodiscard]] size_t GetBufferSize() const;

        void WriteVec2(const glm::vec2 &vec);

        void WriteVec3(const glm::vec3 &vec);

    private:
        std::vector<uint8_t> data{};
};
