//
// Created by droc101 on 6/26/25.
//

#ifndef DATAREADER_H
#define DATAREADER_H
#include <cstdint>
#include <cstddef>

class DataReader {

    public:

        /// @warning This @c DataReader will take control of @code bytes@endcode and @c delete them when it is destroyed.
        DataReader(const uint8_t *bytes, size_t dataSize);

        ~DataReader();

        void Seek(std::ptrdiff_t offset);

        void SeekAbsolute(size_t position);

        [[nodiscard]] uint8_t ReadU8();

        [[nodiscard]] int8_t Read8();

        [[nodiscard]] uint8_t ReadU16();

        [[nodiscard]] int16_t Read16();

        [[nodiscard]] uint32_t ReadU32();

        [[nodiscard]] int32_t Read32();

        [[nodiscard]] uint64_t ReadU64();

        [[nodiscard]] int64_t Read64();

        [[nodiscard]] float ReadFloat();

        [[nodiscard]] double ReadDouble();

        /// @warning The caller must @code delete[]@endcode the return value of this function
        [[nodiscard]] char* ReadString(size_t length);

        /// @warning The caller must @code delete[]@endcode the return value of this function
        [[nodiscard]] uint8_t *ReadBytes(size_t length);

    private:
        const uint8_t *bytes;
        size_t size;
        size_t offset;
};



#endif //DATAREADER_H
