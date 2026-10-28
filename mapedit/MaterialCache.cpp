//
// Created by droc101 on 10/28/26.
//

#include "MaterialCache.h"
#include <game_sdk/SharedMgr.h>
#include <libassets/asset/LevelMaterialAsset.h>
#include <libassets/util/Error.h>
#include <string>

LevelMaterialAsset &MaterialCache::GetMaterial(const std::string &material)
{
    if (cache.contains(material))
    {
        return cache.at(material);
    }

    const std::string materialPath = SharedMgr::Get().pathManager.GetAssetPath(material);
    LevelMaterialAsset materialAsset{};
    const Error::ErrorCode error = materialAsset.LoadFromAsset(materialPath);
    if (error != Error::ErrorCode::OK)
    {
        materialAsset.Reset();
    }

    cache[material] = materialAsset;
    return cache.at(material);
}
