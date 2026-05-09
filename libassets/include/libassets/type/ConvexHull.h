//
// Created by droc101 on 8/30/25.
//

#pragma once

#include <assimp/mesh.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <string>
#include <vector>

class ConvexHull
{
    public:
        ConvexHull(const std::string &objPath, Error::ErrorCode &status);
        explicit ConvexHull(DataReader &reader);
        explicit ConvexHull(const aiMesh *mesh);

        void Write(DataWriter &writer) const;

        std::vector<glm::vec3> &GetPoints();

        [[nodiscard]] std::vector<float> GetPointsForRender() const;

        [[nodiscard]] static Error::ErrorCode ImportMultiple(const std::string &path, std::vector<ConvexHull> &output);

    private:
        glm::vec3 offset{};
        std::vector<glm::vec3> points{};

        void CalculateOffset();
};
