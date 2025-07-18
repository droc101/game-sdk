//
// Created by droc101 on 7/18/25.
//

#ifndef COLOR_H
#define COLOR_H
#include <array>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>


class Color
{
    public:
        Color() = default;
        constexpr Color(const Color &other) = default;
        Color &operator=(const Color &other) = default;

        explicit Color(DataReader &reader, bool useFloats = false);

        explicit Color(uint32_t rgba);

        explicit Color(std::array<float, 4> rgba);

        void WriteFloats(DataWriter &writer) const;

        void WriteUint32(DataWriter &writer) const;

        float *GetData();

    private:
        std::array<float, 4> color = {1.0f, 1.0f, 1.0f, 1.0f};
};


#endif //COLOR_H
