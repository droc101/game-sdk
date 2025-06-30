//
// Created by droc101 on 6/26/25.
//

#include "include/libassets/ModelAsset.h"
#include <cassert>
#include <iostream>
#include <unordered_map>
#include "include/libassets/AssetReader.h"
#include "include/libassets/DataReader.h"
#include "include/libassets/DataWriter.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

ModelAsset ModelAsset::CreateFromAsset(const char *assetPath)
{
    size_t aSz;
    AssetReader::AssetType aTp;
    const uint8_t *data = AssetReader::LoadFromFile(assetPath, &aSz, &aTp);
    assert(aSz >= sizeof(uint32_t) * 4);
    assert(aTp == AssetReader::ASSET_TYPE_MODEL);
    DataReader r = DataReader(data, aSz);
    ModelAsset m = ModelAsset();
    const uint32_t materialCount = r.ReadU32();
    const uint32_t skinCount = r.ReadU32();
    const uint32_t lodCount = r.ReadU32();

    for (int si = 0; si < skinCount; si++)
    {
        std::vector<Material> skin = std::vector<Material>();
        for (int mi = 0; mi < materialCount; mi++)
        {
            Material mat = Material();
            mat.texture = r.ReadString(64);
            mat.color = r.ReadU32();
            mat.shader = static_cast<ModelShader>(r.ReadU32());
            skin.push_back(mat);
        }
        m.skins.push_back(skin);
    }

    for (int li = 0; li < lodCount; li++)
    {
        ModelLod l = ModelLod();
        l.distance = r.ReadFloat();
        const uint32_t vertexCount = r.ReadU32();
        for (int vi = 0; vi < vertexCount; vi++)
        {
            Vertex v = Vertex();
            for (int i = 0; i < 3; i++)
            {
                v.position.at(i) = r.ReadFloat();
            }
            for (int i = 0; i < 3; i++)
            {
                v.normal.at(i) = r.ReadFloat();
            }
            for (int i = 0; i < 2; i++)
            {
                v.uv.at(i) = r.ReadFloat();
            }
            l.vertices.push_back(v);
        }
        for (int ici = 0; ici < materialCount; ici++)
        {
            l.indexCounts.push_back(r.ReadU32());
        }
        for (int ii = 0; ii < materialCount; ii++)
        {
            std::vector<uint32_t> indices = std::vector<uint32_t>();
            for (int iii = 0; iii < l.indexCounts.at(ii); iii++)
            {
                indices.push_back(r.ReadU32());
            }
            l.indices.push_back(indices);
        }
        m.lods.push_back(l);
    }

    return m;
}

uint8_t *ModelAsset::SaveToBuffer(size_t *outSize) const
{
    assert(!skins.empty()); // you gotta have a skin
    assert(!skins.at(0).empty()); // you gotta have materials
    assert(!lods.empty()); // you gotta have a lod
    assert(lods.at(0).distance == 0); // lod 0 must be distance 0
    assert(lods.at(0).vertices.size() >= 3); // triangle required
    DataWriter w;
    w.Write<uint32_t>(static_cast<uint32_t>(skins.at(0).size()));
    w.Write<uint32_t>(static_cast<uint32_t>(skins.size()));
    w.Write<uint32_t>(static_cast<uint32_t>(lods.size()));
    for (int si = 0; si < skins.size(); si++)
    {
        for (int mi = 0; mi < skins.at(0).size(); mi++)
        {
            const Material mat = skins.at(si).at(mi);
            w.WriteBuffer<char>(mat.texture, 64);
            w.Write<uint32_t>(mat.color);
            w.Write<uint32_t>(static_cast<uint32_t>(mat.shader));
        }
    }

    for (ModelLod l: lods)
    {
        w.Write<float>(l.distance);
        w.Write<uint32_t>(static_cast<uint32_t>(l.vertices.size()));
        for (Vertex v: l.vertices)
        {
            w.WriteBuffer<float>(v.position.data(), 3);
            w.WriteBuffer<float>(v.normal.data(), 3);
            w.WriteBuffer<float>(v.uv.data(), 2);
        }
        w.WriteBuffer<uint32_t>(l.indexCounts.data(), l.indexCounts.size());
        for (int ii = 0; ii < skins.at(0).size(); ii++)
        {
            std::vector<uint32_t> indices = l.indices.at(ii);
            w.WriteBuffer<uint32_t>(indices.data(), indices.size());
        }
    }

    *outSize = w.GetBufferSize();
    return w.GetBuffer();
}

void ModelAsset::SaveAsAsset(const char *assetPath) const
{
    size_t size;
    uint8_t *buffer = SaveToBuffer(&size);
    AssetReader::SaveToFile(assetPath, buffer, size, AssetReader::ASSET_TYPE_MODEL);
    delete[] buffer;
}

ModelAsset::ModelLod ModelAsset::GetLod(size_t index) const
{
    return lods.at(index);
}

const ModelAsset::Material *ModelAsset::GetSkin(size_t index) const
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

const uint8_t *ModelAsset::GetVertexBuffer(size_t lodIndex, size_t *size) const
{
    const ModelLod lod = GetLod(lodIndex);
    DataWriter w = DataWriter();
    for (const Vertex v: lod.vertices)
    {
        w.WriteBuffer<const float>(v.position.data(), 3);
        w.WriteBuffer<const float>(v.normal.data(), 3);
        w.WriteBuffer<const float>(v.uv.data(), 2);
    }
    *size = w.GetBufferSize();
    return w.GetBuffer();
}

ModelAsset::ModelLod ModelAsset::CreateLodFromStandardModel(const char *filePath, float distance)
{
    ModelLod lod{};
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
                                             aiProcess_GenSmoothNormals |
                                             aiProcess_FixInfacingNormals);

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

        for (uint32_t f = 0; f < mesh->mNumFaces; ++f)
        {
            const aiFace face = mesh->mFaces[f];
            for (uint32_t j = 0; j < face.mNumIndices; ++j)
            {
                const uint32_t vi = face.mIndices[j];

                Vertex v;
                const aiVector3D pos = mesh->mVertices[vi];
                const aiVector3D uv = mesh->HasNormals() ? mesh->mNormals[vi] : aiVector3D(0, 0, 0);
                const aiVector3D nrm = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][vi] : aiVector3D(0, 0, 0);
                v.position = {pos.x, pos.y, pos.z};
                v.normal = {nrm.x, nrm.y, nrm.z};
                v.uv = {uv.x, uv.y};

                auto it = vertexToIndex.find(v);
                if (it != vertexToIndex.end())
                {
                    indices.push_back(it->second);
                } else
                {
                    const uint32_t newIndex = lod.vertices.size();
                    lod.vertices.push_back(v);
                    vertexToIndex[v] = newIndex;
                    indices.push_back(newIndex);
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

bool ModelAsset::Vertex::operator==(const Vertex &o) const
{
    return this->normal == o.normal && this->position == o.position && this->uv == o.uv;
}

std::size_t std::hash<ModelAsset::Vertex>::operator()(const ModelAsset::Vertex &v) const noexcept
{
    size_t h = 0;
    for (const float f : v.position) {
        h ^= std::hash<float>()(f) + 0x9e3779b9 + (h << 6) + (h >> 2);
    }
    for (const float f : v.normal) {
        h ^= std::hash<float>()(f) + 0x9e3779b9 + (h << 6) + (h >> 2);
    }
    for (const float f : v.uv) {
        h ^= std::hash<float>()(f) + 0x9e3779b9 + (h << 6) + (h >> 2);
    }
    return h;
}

ModelAsset ModelAsset::CreateFromStandardModel(const char *objPath)
{
    ModelAsset model{};
    const ModelLod l = CreateLodFromStandardModel(objPath, 0);
    model.lods.push_back(l);
    const size_t materialCount = l.indexCounts.size();
    std::vector<Material> skin{};
    for (size_t m = 0; m < materialCount; m++)
    {
        Material mat{};
        mat.color = -1u;
        mat.shader = ModelShader::SHADER_SHADED;
        mat.texture = (char *)"texture/level_wall_test.gtex";
        skin.emplace_back(mat);
    }
    model.skins.push_back(skin);
    return model;
}
