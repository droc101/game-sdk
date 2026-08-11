//
// Created by droc101 on 6/26/25.
//

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <libassets/asset/Asset.h>
#include <libassets/asset/ModelAsset.h>
#include <libassets/type/BoundingBox.h>
#include <libassets/type/Color.h>
#include <libassets/type/ConvexHull.h>
#include <libassets/type/Material.h>
#include <libassets/type/ModelLod.h>
#include <libassets/type/ModelVertex.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <string>
#include <vector>

ModelAsset::ModelAsset()
{
    Reset();
}

Asset::AssetType ModelAsset::GetAssetType() const
{
    return AssetType::ASSET_TYPE_MODEL;
}

uint8_t ModelAsset::GetAssetTypeVersion() const
{
    return MODEL_ASSET_VERSION;
}

void ModelAsset::Reset()
{
    materials = {};
    skins = {};
    lods = {};

    collisionModelType = CollisionModelType::NONE;
    boundingBox = BoundingBox();
    convexHulls = {};
    staticCollisionMesh = StaticCollisionMesh();
}

Error::ErrorCode ModelAsset::LoadFromBuffer(DataReader &reader)
{
    Reset();
    const uint32_t materialCount = reader.Read<uint32_t>();
    const uint32_t materialsPerSkin = reader.Read<uint32_t>();
    const uint32_t skinCount = reader.Read<uint32_t>();
    const uint32_t lodCount = reader.Read<uint32_t>();
    collisionModelType = static_cast<CollisionModelType>(reader.Read<uint8_t>());

    materials.reserve(materialCount);
    for (uint32_t i = 0; i < materialCount; i++)
    {
        materials.emplace_back(reader);
    }

    skins.resize(skinCount);
    for (std::vector<uint32_t> &skin: skins)
    {
        skin.reserve(materialsPerSkin);
        for (uint32_t _i = 0; _i < materialsPerSkin; _i++)
        {
            skin.emplace_back(reader.Read<uint32_t>());
        }
    }

    for (uint32_t _i = 0; _i < lodCount; _i++)
    {
        lods.emplace_back(reader, materialsPerSkin);
    }

    boundingBox = BoundingBox(reader);

    if (collisionModelType == CollisionModelType::DYNAMIC_MULTIPLE_CONVEX)
    {
        const size_t hullCount = reader.Read<size_t>();
        for (size_t i = 0; i < hullCount; i++)
        {
            const ConvexHull hull = ConvexHull(reader);
            convexHulls.push_back(hull);
        }
    } else if (collisionModelType == CollisionModelType::STATIC_SINGLE_CONCAVE)
    {
        staticCollisionMesh = StaticCollisionMesh(reader);
    }

    return Error::ErrorCode::OK;
}

Error::ErrorCode ModelAsset::SaveToBuffer(DataWriter &writer) const
{
    assert(!skins.empty()); // you gotta have a skin
    assert(!skins.at(0).empty()); // you gotta have materials
    assert(!lods.empty()); // you gotta have a lod
    assert(lods.at(0).distance == 0); // lod 0 must be distance 0
    assert(lods.at(0).vertices.size() >= 3); // triangle required

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

    boundingBox.Write(writer);

    if (collisionModelType == CollisionModelType::DYNAMIC_MULTIPLE_CONVEX)
    {
        writer.Write<size_t>(convexHulls.size());
        for (const ConvexHull &hull: convexHulls)
        {
            hull.Write(writer);
        }
    } else if (collisionModelType == CollisionModelType::STATIC_SINGLE_CONCAVE)
    {
        staticCollisionMesh.Write(writer);
    }

    return Error::ErrorCode::OK;
}

Error::ErrorCode ModelAsset::Import(const std::string &filePath)
{
    Reset();
    Error::ErrorCode lodCode = Error::ErrorCode::UNKNOWN;
    lods.emplace_back(filePath, 0, lodCode);
    if (lodCode != Error::ErrorCode::OK)
    {
        return lodCode;
    }
    const ModelLod &lod = lods.back();
    const uint32_t materialCount = lod.indexCounts.size();
    skins.emplace_back(materialCount);
    materials = {Material("", -1u, Material::MaterialShader::SHADER_SHADED)};
    return Error::ErrorCode::OK;
}

ModelLod &ModelAsset::GetLod(const uint32_t index)
{
    return lods.at(index);
}

std::vector<uint32_t> &ModelAsset::GetSkin(const uint32_t index)
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
        writer.WriteVec3(vertex.position);
        writer.WriteVec2(vertex.uv);
        vertex.color.WriteFloats(writer);
        writer.WriteVec3(vertex.normal);
    }
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
    const float dist = lods.back().distance + 5;
    Error::ErrorCode status = Error::ErrorCode::UNKNOWN;
    const ModelLod lod(path, dist, status);
    if (status != Error::ErrorCode::OK)
    {
        return false;
    }
    if (lod.indexCounts.size() != GetMaterialsPerSkin())
    {
        return false;
    }
    lods.push_back(lod);
    return true;
}

void ModelAsset::RemoveLod(const uint32_t index)
{
    lods.erase(lods.begin() + index);
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

BoundingBox &ModelAsset::GetBoundingBox()
{
    return boundingBox;
}

const BoundingBox &ModelAsset::GetBoundingBox() const
{
    return boundingBox;
}

ModelAsset::CollisionModelType &ModelAsset::GetCollisionModelType()
{
    return collisionModelType;
}

size_t ModelAsset::GetNumHulls() const
{
    return convexHulls.size();
}

ConvexHull &ModelAsset::GetHull(const size_t index)
{
    return convexHulls.at(index);
}

void ModelAsset::AddHull(const ConvexHull &hull)
{
    convexHulls.push_back(hull);
}

Error::ErrorCode ModelAsset::AddHulls(const std::string &path)
{
    return ConvexHull::ImportMultiple(path, convexHulls);
}

void ModelAsset::RemoveHull(const size_t index)
{
    convexHulls.erase(convexHulls.begin() + static_cast<int64_t>(index));
}

StaticCollisionMesh &ModelAsset::GetStaticCollisionMesh()
{
    return staticCollisionMesh;
}

void ModelAsset::SetStaticCollisionMesh(const StaticCollisionMesh &mesh)
{
    staticCollisionMesh = mesh;
}
