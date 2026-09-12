//
// Created by droc101 on 9/2/26.
//

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <glm/ext/matrix_transform.hpp>
#include <libassets/type/Axis.h>
#include <libassets/type/BoundingBox.h>
#include <libassets/type/Brush.h>
#include <unordered_set>
#include <utility>
#include <vector>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

Brush::Brush(nlohmann::ordered_json json)
{
    editorName = json.value("editor_name", "");

    origin.x = json["origin_x"];
    origin.y = json["origin_y"];
    origin.z = json["origin_z"];

    rotation.x = json["rotation_x"];
    rotation.y = json["rotation_y"];
    rotation.z = json["rotation_z"];

    const nlohmann::ordered_json jsonVertices = json.at("vertices");
    for (const auto &item: jsonVertices.items())
    {
        vertices.push_back({item.value().value("x", 0.0f),
                            item.value().value("y", 0.0f),
                            item.value().value("z", 0.0f)});
    }

    const nlohmann::ordered_json jsonFaces = json.at("faces");
    for (const auto &item: jsonFaces.items())
    {
        faces.emplace_back(item.value());
    }
}

Brush::Brush(const std::vector<glm::vec2> &points,
             const float startDepth,
             const float endDepth,
             const Axis axis,
             const std::string &material,
             const float gridSnap)
{
    for (const glm::vec2 &glmPoint: points)
    {
        vertices.push_back(AxisHelper::Make3D(axis, glmPoint, startDepth));
    }
    for (const glm::vec2 &glmPoint: points)
    {
        vertices.push_back(AxisHelper::Make3D(axis, glmPoint, endDepth));
    }

    const bool reverse = glm::cross(glm::vec3{points.at(1) - points.at(0), 0},
                                    glm::vec3{points.at(2) - points.at(0), 0})
                                 .z < 0;

    for (size_t i = 0; i < points.size(); i++)
    {
        Face face{};
        face.material = material;
        size_t nextIndex = (i + 1) % points.size();
        face.indices.push_back(i);
        if (reverse)
        {
            face.indices.push_back(i + points.size());
            face.indices.push_back(nextIndex + points.size());
            face.indices.push_back(nextIndex);
        } else
        {
            face.indices.push_back(nextIndex);
            face.indices.push_back(nextIndex + points.size());
            face.indices.push_back(i + points.size());
        }
        faces.push_back(face);
    }

    for (uint8_t whichCap = 0; whichCap < 2; whichCap++)
    {
        const size_t indexOffset = whichCap == 1 ? points.size() : 0;

        Face face{};
        face.material = material;
        if ((reverse && whichCap == 1) || (!reverse && whichCap == 0))
        {
            face.indices.push_back(indexOffset);
            for (size_t i = points.size() - 1; i > 0; i--)
            {
                face.indices.push_back(i + indexOffset);
            }
        } else
        {
            for (size_t i = 0; i < points.size(); i++)
            {
                face.indices.push_back(i + indexOffset);
            }
        }
        faces.push_back(face);
    }

    CenterOrigin(gridSnap);
}

nlohmann::ordered_json Brush::GenerateJson() const
{
    nlohmann::ordered_json json = nlohmann::ordered_json();

    json["editor_name"] = editorName;

    json["origin_x"] = origin.x;
    json["origin_y"] = origin.y;
    json["origin_z"] = origin.z;
    json["rotation_x"] = rotation.x;
    json["rotation_y"] = rotation.y;
    json["rotation_z"] = rotation.z;

    json["vertices"] = nlohmann::ordered_json::array();
    for (const glm::vec3 &vertex: vertices)
    {
        nlohmann::ordered_json vertexJson = nlohmann::ordered_json();
        vertexJson["x"] = vertex.x;
        vertexJson["y"] = vertex.y;
        vertexJson["z"] = vertex.z;
        json["vertices"].push_back(vertexJson);
    }

    json["faces"] = nlohmann::ordered_json::array();
    for (const Face &face: faces)
    {
        json.push_back(face.GenerateJson());
    }

    return json;
}

Brush::Face::Face(nlohmann::ordered_json json)
{
    const nlohmann::ordered_json jsonIndices = json.at("indices");
    for (const auto &item: jsonIndices.items())
    {
        indices.push_back(item.value().get<uint32_t>());
    }
    material = json.value("material", "");
    textureScale.x = json.value("texture_scale_x", 1.0f);
    textureScale.y = json.value("texture_scale_y", 1.0f);
    textureOffset.x = json.value("texture_offset_x", 0.0f);
    textureOffset.y = json.value("texture_offset_y", 0.0f);
    textureRotation = json.value("texture_rotation", 0.0f);
    unitsPerLuxel = json.value("units_per_luxel", 1.0f);
}

nlohmann::ordered_json Brush::Face::GenerateJson() const
{
    nlohmann::ordered_json json = nlohmann::ordered_json();
    json["indices"] = nlohmann::ordered_json::array();
    for (const uint32_t index: indices)
    {
        json["indices"].push_back(index);
    }
    json["material"] = material;
    json["texture_scale_x"] = textureScale.x;
    json["texture_scale_y"] = textureScale.y;
    json["texture_offset_x"] = textureOffset.x;
    json["texture_offset_y"] = textureOffset.y;
    json["texture_rotation"] = textureRotation;
    json["units_per_luxel"] = unitsPerLuxel;
    return json;
}

bool Brush::IsValid() const
{
    return true; // TODO
}

BoundingBox Brush::GetAABB() const
{
    const glm::mat4 matrix = GetTransformMatrix();
    std::vector<glm::vec3> points{};
    for (const glm::vec3 &vertex: vertices)
    {
        points.emplace_back(matrix * glm::vec4(vertex, 0.0f));
    }
    return BoundingBox(points);
}

glm::mat4 Brush::GetTransformMatrix() const
{
    glm::mat4 matrix = glm::identity<glm::mat4>();
    matrix = glm::translate(matrix, origin);
    matrix = glm::rotate(matrix, glm::radians(rotation.y), glm::vec3(0, 1, 0));
    matrix = glm::rotate(matrix, glm::radians(rotation.x), glm::vec3(1, 0, 0));
    matrix = glm::rotate(matrix, -glm::radians(rotation.z), glm::vec3(0, 0, 1));
    return matrix;
}

std::unordered_set<std::pair<glm::vec3, glm::vec3>> Brush::GetUniqueEdges() const
{
    std::unordered_set<std::pair<glm::vec3, glm::vec3>> edges{};
    for (const Face &face: faces)
    {
        for (size_t i = 0; i < face.indices.size(); i++)
        {
            const size_t nextIndex = (i + 1) % face.indices.size();
            uint32_t startIndex = face.indices.at(i);
            uint32_t endIndex = face.indices.at(nextIndex);
            if (startIndex > endIndex)
            {
                // swap to have consistent ordering to not push duplicates into set via a reversed pair
                std::swap(startIndex, endIndex);
            }
            edges.emplace(vertices.at(startIndex), vertices.at(endIndex));
        }
    }

    return edges;
}

bool Brush::ContainsPoint(const Axis axis, const glm::vec2 point) const
{
    for (const Face &face: faces)
    {
        bool inside = false;
        const size_t n = face.indices.size();
        for (size_t i = 0; i < n; i++)
        {
            const size_t j = (i + n - 1) % n;
            const glm::vec2 &pointI = AxisHelper::Make2D(axis, vertices.at(face.indices.at(i)));
            const glm::vec2 &pointJ = AxisHelper::Make2D(axis, vertices.at(face.indices.at(j)));

            const bool intersect = ((pointI.y > point.y) != (pointJ.y > point.y)) &&
                                   (point.x <
                                    (pointJ.x - pointI.x) * (point.y - pointI.y) / (pointJ.y - pointI.y) + pointI.x);

            if (intersect)
            {
                inside = !inside;
            }
        }
        if (inside)
        {
            return true;
        }
    }

    return false;
}

std::vector<uint32_t> Brush::GetTriangulatedMesh() const
{
    std::vector<uint32_t> indices;
    for (const Face &face: faces)
    {
        const uint32_t sharedIndex = face.indices.at(0);

        for (uint32_t i = 1; i < face.indices.size() - 1; i++)
        {
            indices.emplace_back(sharedIndex);
            indices.emplace_back(face.indices.at(i));
            indices.emplace_back(face.indices.at(i + 1));
        }
    }
    return indices;
}

void Brush::CenterOrigin(const float gridSnap)
{
    const BoundingBox bb = BoundingBox(vertices);
    glm::vec3 newOrigin = bb.origin;

    float nf = newOrigin.x / gridSnap;
    float fs = std::round(nf);
    newOrigin.x = fs * gridSnap;
    nf = newOrigin.y / gridSnap;
    fs = std::round(nf);
    newOrigin.y = fs * gridSnap;
    nf = newOrigin.z / gridSnap;
    fs = std::round(nf);
    newOrigin.z = fs * gridSnap;

    for (glm::vec3 &vertex: vertices)
    {
        vertex = vertex - newOrigin;
    }

    origin = newOrigin;
}
