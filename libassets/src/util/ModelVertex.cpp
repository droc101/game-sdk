//
// Created by droc101 on 7/18/25.
//

#include <libassets/util/ModelVertex.h>

ModelVertex::ModelVertex(DataReader &reader)
{
    for (float &pos: position)
    {
        pos = reader.Read<float>();
    }
    uv.at(0) = reader.Read<float>();
    uv.at(1) = reader.Read<float>();
    for (float &norm: normal)
    {
        norm = reader.Read<float>();
    }
}

ModelVertex::ModelVertex(const aiMesh *mesh, const uint32_t vertexIndex)
{
    const aiVector3D position = mesh->mVertices[vertexIndex];
    const aiVector3D normal = mesh->HasNormals() ? mesh->mNormals[vertexIndex] : aiVector3D(0, 0, 0);
    const aiVector3D uv = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][vertexIndex] : aiVector3D(0, 0, 0);
    this->position = {position.x, position.y, position.z};
    this->normal = {normal.x, normal.y, normal.z};
    this->uv = {uv.x, uv.y};
}

bool ModelVertex::operator==(const ModelVertex &other) const
{
    return this->normal == other.normal && this->position == other.position && this->uv == other.uv;
}

std::size_t std::hash<ModelVertex>::operator()(const ModelVertex &vertex) const noexcept
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
