//
// Created by droc101 on 1/20/26.
//

#pragma once

#include <cstdint>
#include <libassets/type/Param.h>
#include <libassets/util/Error.h>
#include <vector>

class DataAsset
{
    public:
        DataAsset() = default;

        [[nodiscard]] static Error::ErrorCode CreateFromAsset(const char *assetPath, DataAsset &dataAsset);

        [[nodiscard]] static Error::ErrorCode CreateFromJson(const char *jsonPath, DataAsset &dataAsset);

        [[nodiscard]] Error::ErrorCode SaveAsJson(const char *jsonPath) const;

        [[nodiscard]] Error::ErrorCode SaveAsAsset(const char *assetPath) const;

        static constexpr uint8_t DATA_ASSET_VERSION = 1;

        KvList data{};

    private:
        void SaveToBuffer(std::vector<uint8_t> &buffer) const;
};
