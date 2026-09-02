//
// Created by droc101 on 11/16/25.
//

#pragma once

#include <cstdint>
#include <glm/vec2.hpp>
#include <libassets/asset/Asset.h>
#include <libassets/type/Material.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <string>

class LevelMaterialAsset final: public Asset
{
    public:
        enum class SoundClass : uint8_t
        {
            DEFAULT,
        };

        LevelMaterialAsset();

        /// The texture this material uses
        std::string texture;
        /// The base scale of this material
        glm::vec2 baseScale;
        /// The shader this material uses
        Material::MaterialShader shader;
        /// The sound class this material uses
        SoundClass soundClass;
        /// Whether this material should cast shadows or not
        bool castsShadows;
        /// Whether to skip generating visual geometry for faces with this material
        bool compileInvisible;
        /// Whether to skip generating collision geometry for faces with this material
        bool compileNoClip;
        /// The strength of emission by this material
        /// The color of each luxel this surface maps to will be multipled by this value to determine how much light is emitted
        float emissive;

        void Reset() override;

        [[nodiscard]] Error::ErrorCode LoadFromBuffer(DataReader &reader) override;
        [[nodiscard]] Error::ErrorCode SaveToBuffer(DataWriter &writer) const override;

        [[nodiscard]] AssetType GetAssetType() const override;
        [[nodiscard]] uint8_t GetAssetTypeVersion() const override;

    private:
        static constexpr uint8_t LEVEL_MATERIAL_ASSET_VERSION = 1;
};
