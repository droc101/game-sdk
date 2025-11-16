//
// Created by droc101 on 7/18/25.
//

#pragma once

#include <array>
#include <cstdint>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <nlohmann/json.hpp>

class Color
{
    public:
        Color() = default;

        explicit Color(DataReader &reader, bool useFloats = false);

        explicit Color(uint32_t rgba);

        explicit Color(float r, float g, float b, float a);

        explicit Color(nlohmann::ordered_json j);

        constexpr Color &operator=(const Color &other) = default;
        constexpr bool operator==(const Color &other) const = default;

        void WriteFloats(DataWriter &writer) const;

        void WriteUint32(DataWriter &writer) const;

        float *GetDataPointer();

        [[nodiscard]] std::array<float, 4> CopyData() const;

        [[nodiscard]] nlohmann::ordered_json GenerateJson() const;

    private:
        std::array<float, 4> color = {1.0f, 1.0f, 1.0f, 1.0f};
};
