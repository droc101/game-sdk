//
// Created by droc101 on 7/18/25.
//

#include <array>
#include <cstdint>
#include <libassets/util/Color.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>

Color::Color(DataReader &reader, const bool useFloats)
{
    if (useFloats)
    {
        color[0] = reader.Read<float>();
        color[1] = reader.Read<float>();
        color[2] = reader.Read<float>();
        color[3] = reader.Read<float>();
    } else
    {
        const uint32_t rgba = reader.Read<uint32_t>();
        color = {
                // a r g b
                (static_cast<float>((rgba >> 24) & 0xFF)) / 255.0f,
                (static_cast<float>((rgba >> 16) & 0xFF)) / 255.0f,
                (static_cast<float>((rgba >> 8) & 0xFF)) / 255.0f,
                (static_cast<float>((rgba >> 0) & 0xFF)) / 255.0f,
        };
    }
}

float *Color::GetDataPointer()
{
    return color.data();
}

void Color::WriteFloats(DataWriter &writer) const
{
    writer.Write<float>(color[0]);
    writer.Write<float>(color[1]);
    writer.Write<float>(color[2]);
    writer.Write<float>(color[3]);
}

void Color::WriteUint32(DataWriter &writer) const
{
    const uint32_t rgba = (static_cast<uint32_t>(color[0] * 255.0f) << 24) |
                          (static_cast<uint32_t>(color[1] * 255.0f) << 16) |
                          (static_cast<uint32_t>(color[2] * 255.0f) << 8) |
                          (static_cast<uint32_t>(color[3] * 255.0f));
    writer.Write<uint32_t>(rgba);
}

Color::Color(const uint32_t rgba)
{
    color = {
            (static_cast<float>((rgba >> 24) & 0xFF)) / 255.0f,
            (static_cast<float>((rgba >> 16) & 0xFF)) / 255.0f,
            (static_cast<float>((rgba >> 8) & 0xFF)) / 255.0f,
            (static_cast<float>((rgba >> 0) & 0xFF)) / 255.0f,
    };
}

Color::Color(const float r, const float g, const float b, const float a)
{
    color[0] = r;
    color[1] = g;
    color[2] = b;
    color[3] = a;
}

std::array<float, 4> Color::CopyData() const
{
    return color;
}
