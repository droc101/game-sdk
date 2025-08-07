//
// Created by droc101 on 7/18/25.
//

#include <assimp/color4.h>
#include <assimp/mesh.h>
#include <assimp/vector3.h>
#include <cstdint>
#include <libassets/util/Color.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/ModelVertex.h>

ModelVertex::ModelVertex(DataReader &reader)
{
    for (float &pos: position)
    {
        pos = reader.Read<float>();
    }
    uv.at(0) = reader.Read<float>();
    uv.at(1) = reader.Read<float>();
    color = Color(reader, true);
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
    const aiColor4D color = mesh->HasVertexColors(0) ? mesh->mColors[0][vertexIndex] : aiColor4D(1, 1, 1, 1);
    this->position = {position.x, position.y, position.z};
    this->uv = {uv.x, uv.y};
    this->normal = {normal.x, normal.y, normal.z};
    this->color = Color(color.r, color.g, color.b, color.a);
}

bool ModelVertex::operator==(const ModelVertex &other) const
{
    return this->normal == other.normal &&
           this->position == other.position &&
           this->uv == other.uv &&
           this->color == other.color;
}
