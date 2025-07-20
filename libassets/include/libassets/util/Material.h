//
// Created by droc101 on 7/18/25.
//

#ifndef MATERIAL_H
#define MATERIAL_H

#include <libassets/util/Color.h>
#include <libassets/util/DataReader.h>

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
        Material(std::string texture, uint32_t color, MaterialShader shader);

        void Write(DataWriter &writer) const;

        std::string texture{};
        Color color{};
        MaterialShader shader{};
};


#endif //MATERIAL_H
