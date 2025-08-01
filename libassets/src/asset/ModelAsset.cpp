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
    const size_t materialCount = asset.reader.Read<size_t>();
    const size_t materialsPerSkin = asset.reader.Read<size_t>();
    const size_t skinCount = asset.reader.Read<size_t>();
    const size_t lodCount = asset.reader.Read<size_t>();
    modelAsset.collisionModelType = static_cast<CollisionModelType>(asset.reader.Read<uint8_t>());

    modelAsset.materials.reserve(materialCount);
    for (size_t i = 0; i < materialCount; i++)
    {
        modelAsset.materials.emplace_back(asset.reader);
    }

    modelAsset.skins.resize(skinCount);
    for (std::vector<size_t> &skin: modelAsset.skins)
    {
        skin.reserve(materialsPerSkin);
        for (size_t _i = 0; _i < materialsPerSkin; _i++)
        {
            skin.emplace_back(asset.reader.Read<size_t>());
        }
    }

    for (size_t _i = 0; _i < lodCount; _i++)
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
    writer.Write<size_t>(materials.size());
    writer.Write<size_t>(skins.at(0).size());
    writer.Write<size_t>(skins.size());
    writer.Write<size_t>(lods.size());
    writer.Write<uint8_t>(static_cast<uint8_t>(collisionModelType));

    for (const Material &mat: materials)
    {
        mat.Write(writer);
    }

    for (const std::vector<size_t> &skinMaterialIndices: skins)
    {
        for (const size_t &materialIndex: skinMaterialIndices)
        {
            writer.Write<size_t>(materialIndex);
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

size_t *ModelAsset::GetSkin(const size_t index)
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

size_t ModelAsset::GetMaterialsPerSkin() const
{
    return skins.at(0).size();
}

void ModelAsset::AddSkin()
{
    std::vector<size_t> skin{};
    for (size_t i = 0; i < GetMaterialsPerSkin(); i++)
    {
        skin.push_back(0);
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
        vertex.color.WriteFloats(writer);
        writer.WriteBuffer<float, 3>(vertex.normal);
    }
}

Error::ErrorCode ModelAsset::CreateFromStandardModel(const char *objPath, ModelAsset &model, const std::string& defaultTexture)
{
    model = ModelAsset();
    const ModelLod lod(objPath, 0);
    model.lods.push_back(lod);
    const size_t materialCount = lod.indexCounts.size();
    std::vector<size_t> skin{};
    for (size_t _i = 0; _i < materialCount; _i++)
    {
        skin.emplace_back(0);
    }
    model.skins.push_back(skin);
    model.materials = {
        Material(defaultTexture, -1u, Material::MaterialShader::SHADER_SHADED)
    };
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
    if (l.indexCounts.size() != GetMaterialsPerSkin()) return false;
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

Material &ModelAsset::GetMaterial(const size_t index)
{
    return materials.at(index);
}

size_t ModelAsset::GetMaterialCount() const
{
    return materials.size();
}

void ModelAsset::AddMaterial(const Material &mat)
{
    materials.push_back(mat);
}

void ModelAsset::RemoveMaterial(const size_t index)
{
    materials.erase(materials.begin() + static_cast<int64_t>(index));
}

