//
// Created by droc101 on 6/26/25.
//

#ifndef DATAWRITER_H
#define DATAWRITER_H
#include <cstdint>
#include <cstring>
#include <vector>

class DataWriter {

    public:
        DataWriter() = default;

        /**
         * Write a value to the buffer
         * @tparam T The type of value
         * @warning This is only indented for primitive types like @c uint32_t
         * @param value The value to write
         */
        template <typename T> void Write(T value)
        {
            std::array<uint8_t, sizeof(T)> buf = std::array<uint8_t, sizeof(T)>();
            std::memcpy(buf.data(), &value, sizeof(T));
            data.insert(data.end(), buf.data(), buf.data() + sizeof(T));
        }

        /**
         * Write an array of same-type values to the buffer
         * @tparam T The type of value in the array
         * @warning This is only indented for primitive types like @c uint32_t
         * @param buffer The array
         * @param length The number of elements in the array
         */
        template <typename T> void WriteBuffer(T *buffer, size_t length)
        {
            uint8_t *buf = new uint8_t[sizeof(T)*length];
            std::memcpy(buf, buffer, sizeof(T)*length);
            data.insert(data.end(), buf, buf + sizeof(T)*length);
            delete[] buf;
        }

        [[nodiscard]] uint8_t *GetBuffer() const;

        [[nodiscard]] size_t GetBufferSize() const;
    private:
        std::vector<uint8_t> data = std::vector<uint8_t>();
};

#endif //DATAWRITER_H
