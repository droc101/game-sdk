//
// Created by droc101 on 7/18/25.
//

#include <array>
#include <cstdint>
#include <libassets/type/Color.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>

Color::Color(DataReader &reader, const bool useFloats)
{
    if (useFloats)
    {
        color.at(0) = reader.Read<float>();
        color.at(1) = reader.Read<float>();
        color.at(2) = reader.Read<float>();
        color.at(3) = reader.Read<float>();
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

Color::Color(nlohmann::ordered_json j)
{
    color = {
        j["r"],
        j["g"],
        j["b"],
        j["a"],
    };
}


float *Color::GetDataPointer()
{
    return color.data();
}

void Color::WriteFloats(DataWriter &writer) const
{
    writer.Write<float>(color.at(0));
    writer.Write<float>(color.at(1));
    writer.Write<float>(color.at(2));
    writer.Write<float>(color.at(3));
}

void Color::WriteUint32(DataWriter &writer) const
{
    const uint32_t rgba = GetUint32();
    writer.Write<uint32_t>(rgba);
}

uint32_t Color::GetUint32() const
{
    return (static_cast<uint32_t>(color.at(0) * 255.0f) << 24) |
           (static_cast<uint32_t>(color.at(1) * 255.0f) << 16) |
           (static_cast<uint32_t>(color.at(2) * 255.0f) << 8) |
           (static_cast<uint32_t>(color.at(3) * 255.0f));
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
    color.at(0) = r;
    color.at(1) = g;
    color.at(2) = b;
    color.at(3) = a;
}

std::array<float, 4> Color::CopyData() const
{
    return color;
}

nlohmann::ordered_json Color::GenerateJson() const
{
    nlohmann::ordered_json j{};
    j["r"] = color.at(0);
    j["g"] = color.at(1);
    j["b"] = color.at(2);
    j["a"] = color.at(3);
    return j;
}

void Color::DiscardAlpha()
{
    color.at(3) = 1.0f;
}
