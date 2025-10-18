//
// Created by droc101 on 7/18/25.
//

#pragma once

#include <cstdint>
#include <libassets/type/Color.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <string>

class Material
{
    public:
        enum class MaterialShader : uint32_t // NOLINT(*-enum-size)
        {
            SHADER_SKY,
            SHADER_UNSHADED,
            SHADER_SHADED
        };

        Material() = default;

        explicit Material(DataReader &reader);

        Material(const std::string &texture, uint32_t color, MaterialShader shader);

        void Write(DataWriter &writer) const;

        std::string texture{};

        Color color{};

        MaterialShader shader{};
};
