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
        enum class CollisionModelType: uint8_t
        {
            NONE,
            STATIC_SINGLE_CONCAVE, /// NOT YET IMPLEMENTED! DO NOT USE!
            DYNAMIC_MULTIPLE_CONVEX /// NOT YET IMPLEMENTED! DO NOT USE!
        };

        /**
         * Please use @c ModelAsset::Create* instead.
         */
        ModelAsset() = default;

        [[nodiscard]] static Error::ErrorCode CreateFromAsset(const char *assetPath, ModelAsset &modelAsset);

        [[nodiscard]] static Error::ErrorCode CreateFromStandardModel(const char *objPath, ModelAsset &model, const std::string& defaultTexture);

        [[nodiscard]] Error::ErrorCode SaveAsAsset(const char *assetPath) const;


        [[nodiscard]] ModelLod &GetLod(size_t index);

        void SortLODs();

        [[nodiscard]] bool AddLod(const std::string &path);

        void RemoveLod(size_t index);

        [[nodiscard]] size_t GetLodCount() const;

        [[nodiscard]] bool ValidateLodDistances();


        [[nodiscard]] size_t *GetSkin(size_t index);

        [[nodiscard]] size_t GetSkinCount() const;

        void AddSkin();

        void RemoveSkin(size_t index);

        [[nodiscard]] size_t GetMaterialsPerSkin() const;



        [[nodiscard]] Material &GetMaterial(size_t index);

        [[nodiscard]] size_t GetMaterialCount() const;

        void AddMaterial(const Material &mat);

        void RemoveMaterial(size_t index);


        void GetVertexBuffer(size_t lodIndex, DataWriter &writer);

        static constexpr uint8_t MODEL_ASSET_VERSION = 1;

    private:
        std::vector<Material> materials{};
        std::vector<std::vector<size_t>> skins{};
        std::vector<ModelLod> lods{};
        CollisionModelType collisionModelType = CollisionModelType::NONE;

        void SaveToBuffer(std::vector<uint8_t> &buffer) const;

        static bool LODSortCompare(const ModelLod &a, const ModelLod &b);
};
