//
// Created by droc101 on 6/26/25.
//

#include <libassets/asset/ModelAsset.h>
#include <assimp/postprocess.h>
#include <cassert>
#include <filesystem>
#include <format>
#include <string>
#include <libassets/util/AssetReader.h>
#include <libassets/util/Color.h>
#include <libassets/util/Material.h>
#include <libassets/util/Asset.h>
#include <libassets/util/ModelLod.h>

Error::ErrorCode ModelAsset::CreateFromAsset(const char *assetPath, ModelAsset &modelAsset)
{
    modelAsset.lods.clear();
    modelAsset.skins.clear();
    Asset asset;
    const Error::ErrorCode e = AssetReader::LoadFromFile(assetPath, asset);
    if (e != Error::ErrorCode::E_OK) return e;
    if (asset.type != Asset::AssetType::ASSET_TYPE_MODEL) return Error::ErrorCode::E_INCORRECT_FORMAT;
    if (asset.typeVersion != MODEL_ASSET_VERSION) return Error::ErrorCode::E_INCORRECT_VERSION;
    modelAsset = ModelAsset();
    const uint32_t materialCount = asset.reader.Read<uint32_t>();
    const uint32_t skinCount = asset.reader.Read<uint32_t>();
    const uint32_t lodCount = asset.reader.Read<uint32_t>();

    modelAsset.skins.resize(skinCount);
    for (std::vector<Material> &skin: modelAsset.skins)
    {
        skin.reserve(materialCount);
        for (uint32_t _i = 0; _i < materialCount; _i++)
        {
            skin.emplace_back(asset.reader);
        }
    }

    for (uint32_t _i = 0; _i < lodCount; _i++)
    {
        modelAsset.lods.emplace_back(asset.reader, materialCount);
    }
    return Error::ErrorCode::E_OK;
}

void ModelAsset::SaveToBuffer(std::vector<uint8_t> &buffer) const
{
    assert(buffer.empty());
    assert(!skins.empty()); // you gotta have a skin
    assert(!skins.at(0).empty()); // you gotta have materials
    assert(!lods.empty()); // you gotta have a lod
    assert(lods.at(0).distance == 0); // lod 0 must be distance 0
    assert(lods.at(0).vertices.size() >= 3); // triangle required

    DataWriter writer;
    writer.Write<uint32_t>(static_cast<uint32_t>(skins.at(0).size()));
    writer.Write<uint32_t>(static_cast<uint32_t>(skins.size()));
    writer.Write<uint32_t>(static_cast<uint32_t>(lods.size()));

    for (const std::vector<Material> &skinMaterials: skins)
    {
        for (const Material &material: skinMaterials)
        {
            material.Write(writer);
        }
    }

    for (const ModelLod &lod: lods)
    {
        lod.Write(writer);
    }
    writer.CopyToVector(buffer);
}

Error::ErrorCode ModelAsset::SaveAsAsset(const char *assetPath) const
{
    std::vector<uint8_t> data;
    SaveToBuffer(data);
    return AssetReader::SaveToFile(assetPath, data, Asset::AssetType::ASSET_TYPE_MODEL, MODEL_ASSET_VERSION);
}

ModelLod &ModelAsset::GetLod(const size_t index)
{
    return lods.at(index);
}

Material *ModelAsset::GetSkin(const size_t index)
{
    return skins.at(index).data();
}

size_t ModelAsset::GetLodCount() const
{
    return lods.size();
}

size_t ModelAsset::GetSkinCount() const
{
    return skins.size();
}

size_t ModelAsset::GetMaterialCount() const
{
    return skins.at(0).size();
}

void ModelAsset::AddSkin(const std::string &defaultTexture)
{
    std::vector<Material> skin{};
    for (size_t i = 0; i < GetMaterialCount(); i++)
    {
        Material m{};
        m.texture = defaultTexture;
        m.color = Color({1.0f, 1.0f, 1.0f, 1.0f});
        m.shader = Material::MaterialShader::SHADER_SHADED;
        skin.push_back(m);
    }
    skins.push_back(skin);
}

void ModelAsset::RemoveSkin(const size_t index)
{
    skins.erase(skins.begin() + static_cast<int64_t>(index));
}

void ModelAsset::GetVertexBuffer(const size_t lodIndex, DataWriter &writer)
{
    const ModelLod &lod = GetLod(lodIndex);
    for (const ModelVertex &vertex: lod.vertices)
    {
        writer.WriteBuffer<float, 3>(vertex.position);
        writer.WriteBuffer<float, 2>(vertex.uv);
        writer.WriteBuffer<float, 3>(vertex.normal);
    }
}

Error::ErrorCode ModelAsset::CreateFromStandardModel(const char *objPath, ModelAsset &model, const std::string& defaultTexture)
{
    model = ModelAsset();
    const ModelLod lod(objPath, 0);
    model.lods.push_back(lod);
    const size_t materialCount = lod.indexCounts.size();
    std::vector<Material> skin{};
    for (size_t _i = 0; _i < materialCount; _i++)
    {
        skin.emplace_back(defaultTexture, -1u, Material::MaterialShader::SHADER_SHADED);
    }
    model.skins.push_back(skin);
    return Error::ErrorCode::E_OK;
}

bool ModelAsset::LODSortCompare(const ModelLod &a, const ModelLod &b)
{
    return a.distance < b.distance;
}

void ModelAsset::SortLODs()
{
    std::ranges::sort(lods, LODSortCompare);
}

bool ModelAsset::AddLod(const std::string &path)
{
    const float dist = lods.end()->distance + 5;
    const ModelLod l(path.c_str(), dist);
    if (l.indexCounts.size() != GetMaterialCount()) return false;
    lods.push_back(l);
    return true;
}

void ModelAsset::RemoveLod(const size_t index)
{
    lods.erase(lods.begin() + static_cast<int64_t>(index));
}

bool ModelAsset::ValidateLodDistances()
{
    SortLODs();
    if (lods.at(0).distance != 0.0f) return false; // First LOD must have a distance of 0.
    std::vector<float> distances{};
    for (const ModelLod &l: lods)
    {
        if (std::ranges::find(distances, l.distance) == distances.end())
        {
            distances.push_back(l.distance);
        } else
        {
            return false; // A distance is used by more than 1 LOD.
        }
    }
    return true;
}
