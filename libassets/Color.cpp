//
// Created by droc101 on 7/18/25.
//

#include "Color.h"

Color::Color(DataReader &reader, const bool useFloats)
{
    if (useFloats)
    {
        color[0] = reader.ReadFloat();
        color[1] = reader.ReadFloat();
        color[2] = reader.ReadFloat();
        color[3] = reader.ReadFloat();
    } else
    {
        const uint32_t rgba = reader.ReadU32();
        color = {
                // a r g b
                (static_cast<float>((rgba >> 16) & 0xFF)) / 255.0f,
                (static_cast<float>((rgba >> 8) & 0xFF)) / 255.0f,
                (static_cast<float>((rgba) & 0xFF)) / 255.0f,
                (static_cast<float>((rgba >> 24) & 0xFF)) / 255.0f,
        };
    }
}

float *Color::GetData()
{
    return color.data();
}

void Color::WriteFloats(DataWriter &writer) const
{
    writer.Write<float>(color[3]);
    writer.Write<float>(color[0]);
    writer.Write<float>(color[1]);
    writer.Write<float>(color[2]);
}

void Color::WriteUint32(DataWriter &writer) const
{
    const uint32_t rgba = (static_cast<uint32_t>(color[3] * 255.0f) << 24) |
                          (static_cast<uint32_t>(color[0] * 255.0f) << 16) |
                          (static_cast<uint32_t>(color[1] * 255.0f) << 8) |
                          (static_cast<uint32_t>(color[2] * 255.0f));
    writer.Write<uint32_t>(rgba);
}

Color::Color(const uint32_t rgba)
{
    color = {
            (static_cast<float>((rgba >> 16) & 0xFF)) / 255.0f,
            (static_cast<float>((rgba >> 8) & 0xFF)) / 255.0f,
            (static_cast<float>((rgba) & 0xFF)) / 255.0f,
            (static_cast<float>((rgba >> 24) & 0xFF)) / 255.0f,
    };
}

Color::Color(const std::array<float, 4> rgba)
{
    color = rgba;
}
