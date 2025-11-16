//
// Created by droc101 on 11/16/25.
//

#include <cstdint>
#include <libassets/asset/LevelMaterialAsset.h>
#include <libassets/type/Asset.h>
#include <libassets/type/Material.h>
#include <libassets/util/AssetReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <vector>

Error::ErrorCode LevelMaterialAsset::CreateFromAsset(const char *assetPath, LevelMaterialAsset &material)
{
    Asset asset;
    const Error::ErrorCode error = AssetReader::LoadFromFile(assetPath, asset);
    if (error != Error::ErrorCode::OK)
    {
        return error;
    }
    if (asset.type != Asset::AssetType::ASSET_TYPE_LEVEL_MATERIAL)
    {
        return Error::ErrorCode::INCORRECT_FORMAT;
    }
    if (asset.typeVersion != LEVEL_MATERIAL_ASSET_VERSION)
    {
        return Error::ErrorCode::INCORRECT_VERSION;
    }
    material = LevelMaterialAsset();
    asset.reader.ReadStringWithSize(material.texture);
    material.baseScale[0] = asset.reader.Read<float>();
    material.baseScale[1] = asset.reader.Read<float>();
    material.shader = static_cast<Material::MaterialShader>(asset.reader.Read<uint8_t>());
    material.soundClass = static_cast<SoundClass>(asset.reader.Read<uint8_t>());
    material.compileInvisible = asset.reader.Read<uint8_t>() == 1;
    material.compileNoClip = asset.reader.Read<uint8_t>() == 1;
    return Error::ErrorCode::OK;
}

void LevelMaterialAsset::SaveToBuffer(std::vector<uint8_t> &buffer) const
{
    DataWriter writer{};
    writer.WriteString(texture);
    writer.WriteBuffer<float>(baseScale);
    writer.Write<uint8_t>(static_cast<uint8_t>(shader));
    writer.Write<uint8_t>(static_cast<uint8_t>(soundClass));
    writer.Write<uint8_t>(compileInvisible ? 1 : 0);
    writer.Write<uint8_t>(compileInvisible ? 1 : 0);
    writer.CopyToVector(buffer);
}

Error::ErrorCode LevelMaterialAsset::SaveAsAsset(const char *assetPath) const
{
    std::vector<uint8_t> buffer;
    SaveToBuffer(buffer);
    return AssetReader::SaveToFile(assetPath,
                                   buffer,
                                   Asset::AssetType::ASSET_TYPE_LEVEL_MATERIAL,
                                   LEVEL_MATERIAL_ASSET_VERSION);
}
