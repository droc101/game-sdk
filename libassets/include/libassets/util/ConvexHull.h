//
// Created by droc101 on 8/30/25.
//

#pragma once

#include <array>
#include <assimp/mesh.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <string>
#include <vector>

class ConvexHull
{
    public:
        explicit ConvexHull(const std::string &objPath);
        explicit ConvexHull(DataReader &reader);
        explicit ConvexHull(const aiMesh *mesh);

        void Write(DataWriter &writer) const;

        std::vector<std::array<float, 3>> &GetPoints();

        [[nodiscard]] std::vector<float> GetPointsForRender() const;

        static void ImportMultiple(const std::string &path, std::vector<ConvexHull> &output);

    private:
        std::array<float, 3> offset{};
        std::vector<std::array<float, 3>> points{};

        void CalculateOffset();
};
