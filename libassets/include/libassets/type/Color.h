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

        /**
         * Read a Color from a DataReader
         * @param reader The DataReader to read from
         * @param useFloats Whether to use uint8_t or float components
         */
        explicit Color(DataReader &reader, bool useFloats = false);

        /**
         * Create a Color from an RGBA uint32_t
         */
        explicit Color(uint32_t rgba);

        /**
         * Create a Color from R,G,B,A float components
         * @param r Red
         * @param g Green
         * @param b Blue
         * @param a Alpha
         */
        explicit Color(float r, float g, float b, float a);

        /**
         * Create a Color from JSON
         */
        explicit Color(nlohmann::ordered_json j);

        /**
         * Create a Color from a hex code string
         */
        explicit Color(const std::string &hexCode);

        constexpr Color &operator=(const Color &other) = default;
        constexpr bool operator==(const Color &other) const = default;

        /**
         * Write this Color to a DataReader using floats
         */
        void WriteFloats(DataWriter &writer) const;

        /**
         * Write this Color to a DataReader using uint32_ts
         */
        void WriteUint32(DataWriter &writer) const;

        /**
         * Get this color as an 0xRRGGBBAA uint32_t
         */
        [[nodiscard]] uint32_t GetUint32() const;

        /**
         * Get a pointer to this Color's data [R,G,B,A
         */
        float *GetDataPointer();
        /**
         * Get a pointer to this Color's data [R,G,B,A
         */
        [[nodiscard]] const float *GetDataPointer() const;

        /**
         * Copy this Color's data to a new array
         */
        [[nodiscard]] std::array<float, 4> CopyData() const;

        /**
         * Generate JSON for this color
         */
        [[nodiscard]] nlohmann::ordered_json GenerateJson() const;

        /**
         * Discard the alpha channel of this color, setting it to 1.0f
         */
        void DiscardAlpha();

        /**
         * Get the red component of this color
         */
        float &R();
        /**
         * Get the green component of this color
         */
        float &G();
        /**
         * Get the blue component of this color
         */
        float &B();
        /**
         * Get the alpha component of this color
         */
        float &A();

        /**
         * Get the red component of this color
         */
        [[nodiscard]] float R() const;
        /**
         * Get the green component of this color
         */
        [[nodiscard]] float G() const;
        /**
         * Get the blue component of this color
         */
        [[nodiscard]] float B() const;
        /**
         * Get the alpha component of this color
         */
        [[nodiscard]] float A() const;

    private:
        std::array<float, 4> color = {1.0f, 1.0f, 1.0f, 1.0f};
};
