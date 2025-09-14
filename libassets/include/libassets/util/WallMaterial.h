//
// Created by droc101 on 9/5/25.
//

#pragma once
#include <array>
#include <string>
#include "DataWriter.h"


class WallMaterial
{
    public:
        WallMaterial() = default;
        explicit WallMaterial(const std::string &texture);

        std::string texture;
        std::array<float, 2> uvOffset{};
        std::array<float, 2> uvScale{};
};
