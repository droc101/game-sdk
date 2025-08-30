//
// Created by droc101 on 8/30/25.
//

#pragma once

#include <array>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <string>
#include <vector>

class ConvexHull
{
    public:
        explicit ConvexHull(const std::string &objPath);
        explicit ConvexHull(DataReader &reader);

        void Write(DataWriter &writer) const;

        std::vector<std::array<float, 3>> &GetPoints();

        std::vector<float> GetPointsForRender() const;

    private:
        std::vector<std::array<float, 3>> points{};
};
