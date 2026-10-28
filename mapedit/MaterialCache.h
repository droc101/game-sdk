//
// Created by droc101 on 10/28/26.
//

#pragma once

#include <libassets/asset/LevelMaterialAsset.h>
#include <string>
#include <unordered_map>

class MaterialCache
{
    public:
        MaterialCache() = delete;

        [[nodiscard]] static LevelMaterialAsset &GetMaterial(const std::string &material);

    private:
        static inline std::unordered_map<std::string, LevelMaterialAsset> cache{};
};
