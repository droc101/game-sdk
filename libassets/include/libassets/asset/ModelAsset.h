//
// Created by droc101 on 6/26/25.
//

#pragma once

#include <string>
#include <vector>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/AssetReader.h>
#include <libassets/util/Material.h>
#include <libassets/util/ModelLod.h>

class ModelAsset
{
    public:
        /**
         * Please use @c ModelAsset::Create* instead.
         */
        ModelAsset() = default;

        [[nodiscard]] static Error::ErrorCode CreateFromAsset(const char *assetPath, ModelAsset &modelAsset);

        [[nodiscard]] static Error::ErrorCode CreateFromStandardModel(const char *objPath, ModelAsset &model, const std::string& defaultTexture);

        [[nodiscard]] Error::ErrorCode SaveAsAsset(const char *assetPath) const;

        [[nodiscard]] ModelLod &GetLod(size_t index);

        [[nodiscard]] Material *GetSkin(size_t index);

        [[nodiscard]] size_t GetSkinCount() const;

        [[nodiscard]] size_t GetLodCount() const;

        [[nodiscard]] size_t GetMaterialCount() const;

        void AddSkin(const std::string &defaultTexture);

        void RemoveSkin(size_t index);

        void SortLODs();

        [[nodiscard]] bool AddLod(const std::string &path);

        void RemoveLod(size_t index);

        void GetVertexBuffer(size_t lodIndex, DataWriter &writer);

        bool ValidateLodDistances();

        static constexpr uint8_t MODEL_ASSET_VERSION = 1;

    private:
        std::vector<ModelLod> lods{};
        std::vector<std::vector<Material>> skins{};

        void SaveToBuffer(std::vector<uint8_t> &buffer) const;

        static bool LODSortCompare(const ModelLod &a, const ModelLod &b);
};
