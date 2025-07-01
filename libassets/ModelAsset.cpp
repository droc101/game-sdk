//
// Created by droc101 on 6/26/25.
//

#include "include/libassets/ModelAsset.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include "include/libassets/AssetReader.h"
#include "include/libassets/DataWriter.h"

ModelAsset::Vertex::Vertex(DataReader &reader)
{
    static_assert(uv.size() == 2);
    for (float &pos: position)
    {
        pos = reader.ReadFloat();
    }
    uv.at(0) = reader.ReadFloat();
    uv.at(1) = reader.ReadFloat();
    for (float &norm: normal)
    {
        norm = reader.ReadFloat();
    }
}

ModelAsset::Vertex::Vertex(const aiMesh *mesh, const uint32_t vertexIndex)
{
    const aiVector3D position = mesh->mVertices[vertexIndex];
    const aiVector3D normal = mesh->HasNormals() ? mesh->mNormals[vertexIndex] : aiVector3D(0, 0, 0);
    const aiVector3D uv = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][vertexIndex] : aiVector3D(0, 0, 0);
    this->position = {position.x, position.y, position.z};
    this->normal = {normal.x, normal.y, normal.z};
    this->uv = {uv.x, uv.y};
}

ModelAsset::Material::Material(DataReader &reader)
{
    reader.ReadString(texture, 64);
    color = reader.ReadU32();
    shader = static_cast<ModelShader>(reader.ReadU32());
}

ModelAsset::Material::Material(std::string texture, const uint32_t color, const ModelShader shader):
    texture(std::move(texture)),
    color(color),
    shader(shader)
{}

ModelAsset::ModelLod::ModelLod(DataReader &reader, const uint32_t materialCount)
{
    distance = reader.ReadFloat();
    const uint32_t vertexCount = reader.ReadU32();
    for (uint32_t _i = 0; _i < vertexCount; _i++)
    {
        vertices.emplace_back(reader);
    }
    for (uint32_t _i = 0; _i < materialCount; _i++)
    {
        indexCounts.push_back(reader.ReadU32());
    }
    for (const uint32_t indexCount: indexCounts)
    {
        std::vector<uint32_t> materialIndices;
        for (uint32_t _i = 0; _i < indexCount; _i++)
        {
            materialIndices.push_back(reader.ReadU32());
        }
        indices.push_back(materialIndices);
    }
}

void ModelAsset::CreateFromAsset(const char *assetPath, ModelAsset &modelAsset)
{
    modelAsset.lods.clear();
    modelAsset.skins.clear();
    DataReader reader;
    [[maybe_unused]] const AssetReader::AssetType assetType = AssetReader::LoadFromFile(assetPath, reader);
    assert(assetType == AssetReader::AssetType::ASSET_TYPE_MODEL);
    const uint32_t materialCount = reader.ReadU32();
    const uint32_t skinCount = reader.ReadU32();
    const uint32_t lodCount = reader.ReadU32();

    modelAsset.skins.resize(skinCount);
    for (std::vector<Material> &skin: modelAsset.skins)
    {
        skin.reserve(materialCount);
        for (uint32_t _i = 0; _i < materialCount; _i++)
        {
            skin.emplace_back(reader);
        }
    }

    for (uint32_t _i = 0; _i < lodCount; _i++)
    {
        modelAsset.lods.emplace_back(reader, materialCount);
    }
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
            writer.WriteBuffer<const char>(material.texture.c_str(), 64);
            writer.Write<uint32_t>(material.color);
            writer.Write<uint32_t>(static_cast<uint32_t>(material.shader));
        }
    }

    for (const ModelLod &lod: lods)
    {
        writer.Write<float>(lod.distance);
        writer.Write<uint32_t>(static_cast<uint32_t>(lod.vertices.size()));
        for (const Vertex &vertex: lod.vertices)
        {
            writer.WriteBuffer<float, 3>(vertex.position);
            writer.WriteBuffer<float, 2>(vertex.uv);
            writer.WriteBuffer<float, 3>(vertex.normal);
        }
        writer.WriteBuffer<uint32_t>(lod.indexCounts);
        for (const std::vector<uint32_t> &lodIndices: lod.indices)
        {
            writer.WriteBuffer<uint32_t>(lodIndices);
        }
    }
    writer.CopyToVector(buffer);
}

void ModelAsset::SaveAsAsset(const char *assetPath) const
{
    std::vector<uint8_t> data;
    SaveToBuffer(data);
    AssetReader::SaveToFile(assetPath, data, AssetReader::AssetType::ASSET_TYPE_MODEL);
}

const ModelAsset::ModelLod &ModelAsset::GetLod(const size_t index) const
{
    return lods.at(index);
}

const ModelAsset::Material *ModelAsset::GetSkin(const size_t index) const
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

void ModelAsset::GetVertexBuffer(const size_t lodIndex, DataWriter &writer) const
{
    const ModelLod &lod = GetLod(lodIndex);
    for (const Vertex &vertex: lod.vertices)
    {
        writer.WriteBuffer<float, 3>(vertex.position);
        writer.WriteBuffer<float, 2>(vertex.uv);
        writer.WriteBuffer<float, 3>(vertex.normal);
    }
}

ModelAsset::ModelLod ModelAsset::CreateLodFromStandardModel(const char *filePath, const float distance)
{
    ModelLod lod;
    lod.distance = distance;

    std::unordered_map<Vertex, uint32_t> vertexToIndex;

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(filePath,
                                             aiProcess_Triangulate |
                                                     aiProcess_JoinIdenticalVertices |
                                                     aiProcess_SortByPType |
                                                     aiProcess_ValidateDataStructure |
                                                     aiProcess_CalcTangentSpace |
                                                     aiProcess_FlipUVs |
                                                     aiProcess_GenSmoothNormals);

    if (scene == nullptr || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0u || scene->mRootNode == nullptr)
    {
        printf("Assimp error: %s\n", importer.GetErrorString());
        throw std::runtime_error("assimp error, check stdout");
    }

    for (uint32_t i = 0; i < scene->mNumMeshes; i++)
    {
        const aiMesh *mesh = scene->mMeshes[i];
        const uint32_t materialIndex = mesh->mMaterialIndex;

        if (materialIndex >= lod.indices.size())
        {
            lod.indices.resize(materialIndex + 1);
        }

        std::vector<uint32_t> &indices = lod.indices[materialIndex];

        for (uint32_t j = 0; j < mesh->mNumFaces; j++)
        {
            const aiFace &face = mesh->mFaces[j];
            for (uint32_t k = 0; k < face.mNumIndices; k++)
            {
                const uint32_t vertexIndex = face.mIndices[k];
                const Vertex v(mesh, vertexIndex);
                if (vertexToIndex.contains(v))
                {
                    indices.push_back(vertexToIndex.at(v));
                } else
                {
                    indices.push_back(lod.vertices.size());
                    lod.vertices.emplace_back(mesh, vertexIndex);
                    vertexToIndex[v] = indices.back();
                }
            }
        }
    }


    for (const std::vector<uint32_t> &i: lod.indices)
    {
        lod.indexCounts.push_back(i.size());
    }

    return lod;
}

bool ModelAsset::Vertex::operator==(const Vertex &other) const
{
    return this->normal == other.normal && this->position == other.position && this->uv == other.uv;
}

std::size_t std::hash<ModelAsset::Vertex>::operator()(const ModelAsset::Vertex &vertex) const noexcept
{
    constexpr size_t goldenRatio = 0x9e3779b9;
    size_t hashValue = 0;
    for (const float position: vertex.position)
    {
        hashValue ^= std::hash<float>()(position) + goldenRatio + (hashValue << 6) + (hashValue >> 2);
    }
    for (const float normal: vertex.normal)
    {
        hashValue ^= std::hash<float>()(normal) + goldenRatio + (hashValue << 6) + (hashValue >> 2);
    }
    for (const float uv: vertex.uv)
    {
        hashValue ^= std::hash<float>()(uv) + goldenRatio + (hashValue << 6) + (hashValue >> 2);
    }
    return hashValue;
}

void ModelAsset::CreateFromStandardModel(const char *objPath, ModelAsset &model)
{
    const ModelLod lod = CreateLodFromStandardModel(objPath, 0);
    model.lods.push_back(lod);
    const size_t materialCount = lod.indexCounts.size();
    std::vector<Material> skin{};
    for (size_t _i = 0; _i < materialCount; _i++)
    {
        skin.emplace_back("texture/level_wall_test.gtex", -1u, ModelShader::SHADER_SHADED);
    }
    model.skins.push_back(skin);
}
