//
// Created by droc101 on 11/16/25.
//

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <libassets/type/Material.h>
#include <libassets/util/Error.h>
#include <string>
#include <vector>

class LevelMaterialAsset
{
    public:
        enum class SoundClass : uint8_t
        {
            DEFAULT
        };

        LevelMaterialAsset() = default;

        [[nodiscard]] static Error::ErrorCode CreateFromAsset(const char *assetPath, LevelMaterialAsset &material);

        [[nodiscard]] Error::ErrorCode SaveAsAsset(const char *assetPath) const;

        static constexpr uint8_t LEVEL_MATERIAL_ASSET_VERSION = 1;

        std::string texture{};
        std::array<float, 2> baseScale = {1, 1};
        Material::MaterialShader shader = Material::MaterialShader::SHADER_SHADED;
        SoundClass soundClass = SoundClass::DEFAULT;
        bool compileInvisible = false; // skip generating visual geometry for faces with this material
        bool compileNoClip = false; // skip generating collision geometry for faces with this material
    private:
        void SaveToBuffer(std::vector<uint8_t> &buffer) const;
};
