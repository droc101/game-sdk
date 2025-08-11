//
// Created by droc101 on 6/26/25.
//

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <libassets/asset/ModelAsset.h>
#include <libassets/util/Asset.h>
#include <libassets/util/AssetReader.h>
#include <libassets/util/Color.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <libassets/util/Material.h>
#include <libassets/util/ModelLod.h>
#include <libassets/util/ModelVertex.h>
#include <string>
#include <vector>

Error::ErrorCode ModelAsset::CreateFromAsset(const std::string &assetPath, ModelAsset &modelAsset)
{
    modelAsset.lods.clear();
    modelAsset.skins.clear();
    Asset asset;
    const Error::ErrorCode e = AssetReader::LoadFromFile(assetPath.c_str(), asset);
    if (e != Error::ErrorCode::OK)
    {
        return e;
    }
    if (asset.type != Asset::AssetType::ASSET_TYPE_MODEL)
    {
        return Error::ErrorCode::INCORRECT_FORMAT;
    }
    if (asset.typeVersion != MODEL_ASSET_VERSION)
    {
        return Error::ErrorCode::INCORRECT_VERSION;
    }
    modelAsset = ModelAsset();
    const uint32_t materialCount = asset.reader.Read<uint32_t>();
    const uint32_t materialsPerSkin = asset.reader.Read<uint32_t>();
    const uint32_t skinCount = asset.reader.Read<uint32_t>();
    const uint32_t lodCount = asset.reader.Read<uint32_t>();
    modelAsset.collisionModelType = static_cast<CollisionModelType>(asset.reader.Read<uint8_t>());

    modelAsset.materials.reserve(materialCount);
    for (uint32_t i = 0; i < materialCount; i++)
    {
        modelAsset.materials.emplace_back(asset.reader);
    }

    modelAsset.skins.resize(skinCount);
    for (std::vector<uint32_t> &skin: modelAsset.skins)
    {
        skin.reserve(materialsPerSkin);
        for (uint32_t _i = 0; _i < materialsPerSkin; _i++)
        {
            skin.emplace_back(asset.reader.Read<uint32_t>());
        }
    }

    for (uint32_t _i = 0; _i < lodCount; _i++)
    {
        modelAsset.lods.emplace_back(asset.reader, materialsPerSkin);
    }
    return Error::ErrorCode::OK;
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
    writer.Write<uint32_t>(materials.size());
    writer.Write<uint32_t>(skins.at(0).size());
    writer.Write<uint32_t>(skins.size());
    writer.Write<uint32_t>(lods.size());
    writer.Write<uint8_t>(static_cast<uint8_t>(collisionModelType));

    for (const Material &material: materials)
    {
        material.Write(writer);
    }

    for (const std::vector<uint32_t> &skinMaterialIndices: skins)
    {
        for (const uint32_t &materialIndex: skinMaterialIndices)
        {
            writer.Write<uint32_t>(materialIndex);
        }
    }

    for (const ModelLod &lod: lods)
    {
        lod.Write(writer);
    }
    writer.CopyToVector(buffer);
}

Error::ErrorCode ModelAsset::SaveAsAsset(const std::string &assetPath) const
{
    std::vector<uint8_t> data;
    SaveToBuffer(data);
    return AssetReader::SaveToFile(assetPath.c_str(), data, Asset::AssetType::ASSET_TYPE_MODEL, MODEL_ASSET_VERSION);
}

ModelLod &ModelAsset::GetLod(uint32_t index)
{
    return lods.at(index);
}

std::vector<uint32_t> &ModelAsset::GetSkin(uint32_t index)
{
    return skins.at(index);
}

uint32_t ModelAsset::GetLodCount() const
{
    return lods.size();
}

uint32_t ModelAsset::GetSkinCount() const
{
    return skins.size();
}

uint32_t ModelAsset::GetMaterialsPerSkin() const
{
    return skins.at(0).size();
}

void ModelAsset::AddSkin()
{
    std::vector<uint32_t> skin;
    for (uint32_t i = 0; i < GetMaterialsPerSkin(); i++)
    {
        skin.push_back(0);
    }
    skins.push_back(skin);
}

void ModelAsset::RemoveSkin(const uint32_t index)
{
    skins.erase(skins.begin() + index);
}

void ModelAsset::GetVertexBuffer(const uint32_t lodIndex, DataWriter &writer)
{
    const ModelLod &lod = GetLod(lodIndex);
    for (const ModelVertex &vertex: lod.vertices)
    {
        writer.WriteBuffer<float, 3>(vertex.position);
        writer.WriteBuffer<float, 2>(vertex.uv);
        vertex.color.WriteFloats(writer);
        writer.WriteBuffer<float, 3>(vertex.normal);
    }
}

Error::ErrorCode ModelAsset::CreateFromStandardModel(const std::string &modelPath,
                                                     ModelAsset &model,
                                                     const std::string &defaultTexture)
{
    model = ModelAsset();
    model.lods.emplace_back(modelPath, 0);
    const ModelLod &lod = model.lods.back();
    const uint32_t materialCount = lod.indexCounts.size();
    model.skins.emplace_back(materialCount);
    model.materials = {Material(defaultTexture, -1u, Material::MaterialShader::SHADER_SHADED)};
    return Error::ErrorCode::OK;
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
    const ModelLod lod(path, dist);
    if (lod.indexCounts.size() != GetMaterialsPerSkin())
    {
        return false;
    }
    lods.push_back(lod);
    return true;
}

void ModelAsset::RemoveLod(const uint32_t index)
{
    lods.erase(lods.begin() + static_cast<int64_t>(index));
}

bool ModelAsset::ValidateLodDistances()
{
    SortLODs();
    if (lods.at(0).distance != 0.0f)
    {
        return false; // First LOD must have a distance of 0.
    }
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

Material &ModelAsset::GetMaterial(const uint32_t index)
{
    return materials.at(index);
}

uint32_t ModelAsset::GetMaterialCount() const
{
    return materials.size();
}

void ModelAsset::AddMaterial(const Material &material)
{
    materials.push_back(material);
}

void ModelAsset::RemoveMaterial(const uint32_t index)
{
    materials.erase(materials.begin() + index);
    for (std::vector<uint32_t> &skin: skins)
    {
        for (uint32_t &material: skin)
        {
            if (material > GetMaterialCount() - 1)
            {
                material = GetMaterialCount() - 1;
            }
        }
    }
}
