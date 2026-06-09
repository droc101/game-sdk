//
// Created by droc101 on 7/18/25.
//

#pragma once

#include <array>
#include <cstdint>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <nlohmann/json.hpp>
#include <string>

class Color
{
    public:
        Color() = default;

        explicit Color(DataReader &reader, bool useFloats = false);

        explicit Color(uint32_t rgba);

        explicit Color(float r, float g, float b, float a);

        explicit Color(nlohmann::ordered_json j);

        explicit Color(const std::string &hexCode);

        constexpr Color &operator=(const Color &other) = default;
        constexpr bool operator==(const Color &other) const = default;

        void WriteFloats(DataWriter &writer) const;

        void WriteUint32(DataWriter &writer) const;

        [[nodiscard]] uint32_t GetUint32() const;

        float *GetDataPointer();
        [[nodiscard]] const float *GetDataPointer() const;

        [[nodiscard]] std::array<float, 4> CopyData() const;

        [[nodiscard]] nlohmann::ordered_json GenerateJson() const;

        void DiscardAlpha();

        float &R();
        float &G();
        float &B();
        float &A();

        [[nodiscard]] float R() const;
        [[nodiscard]] float G() const;
        [[nodiscard]] float B() const;
        [[nodiscard]] float A() const;

    private:
        std::array<float, 4> color = {1.0f, 1.0f, 1.0f, 1.0f};
};
