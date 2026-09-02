//
// Created by droc101 on 11/16/25.
//

#include <cstdint>
#include <libassets/asset/Asset.h>
#include <libassets/asset/LevelMaterialAsset.h>
#include <libassets/type/Material.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>

LevelMaterialAsset::LevelMaterialAsset()
{
    Reset();
}

Asset::AssetType LevelMaterialAsset::GetAssetType() const
{
    return AssetType::ASSET_TYPE_LEVEL_MATERIAL;
}

uint8_t LevelMaterialAsset::GetAssetTypeVersion() const
{
    return LEVEL_MATERIAL_ASSET_VERSION;
}

void LevelMaterialAsset::Reset()
{
    texture = "";
    baseScale = {1, 1};
    shader = Material::MaterialShader::SHADER_SHADED;
    soundClass = SoundClass::DEFAULT;
    castsShadows = true;
    compileInvisible = false;
    compileNoClip = false;
    emissive = 0;
}

Error::ErrorCode LevelMaterialAsset::LoadFromBuffer(DataReader &reader)
{
    Reset();
    reader.ReadStringWithSize(texture);
    baseScale = reader.ReadVec2();
    shader = static_cast<Material::MaterialShader>(reader.Read<uint8_t>());
    soundClass = static_cast<SoundClass>(reader.Read<uint8_t>());
    compileInvisible = reader.Read<uint8_t>() == 1;
    compileNoClip = reader.Read<uint8_t>() == 1;
    emissive = reader.Read<float>();
    castsShadows = reader.Read<bool>();
    return Error::ErrorCode::OK;
}

Error::ErrorCode LevelMaterialAsset::SaveToBuffer(DataWriter &writer) const
{
    writer.WriteString(texture);
    writer.WriteVec2(baseScale);
    writer.Write<uint8_t>(static_cast<uint8_t>(shader));
    writer.Write<uint8_t>(static_cast<uint8_t>(soundClass));
    writer.Write<bool>(castsShadows);
    writer.Write<uint8_t>(compileInvisible ? 1 : 0);
    writer.Write<uint8_t>(compileNoClip ? 1 : 0);
    writer.Write<float>(emissive);
    return Error::ErrorCode::OK;
}
