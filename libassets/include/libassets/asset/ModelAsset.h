//
// Created by droc101 on 6/26/25.
//

#pragma once

#include <cstdint>
#include <libassets/util/BoundingBox.h>
#include <libassets/util/ConvexHull.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <libassets/util/Material.h>
#include <libassets/util/ModelLod.h>
#include <string>
#include <vector>
#include "libassets/util/StaticCollisionMesh.h"

class ModelAsset final
{
    public:
        enum class CollisionModelType : uint8_t
        {
            NONE,
            STATIC_SINGLE_CONCAVE,
            DYNAMIC_MULTIPLE_CONVEX
        };

        /**
         * Please use @c ModelAsset::Create* instead.
         */
        ModelAsset() = default;

        [[nodiscard]] static Error::ErrorCode CreateFromAsset(const std::string &assetPath, ModelAsset &modelAsset);

        [[nodiscard]] static Error::ErrorCode CreateFromStandardModel(const std::string &modelPath,
                                                                      ModelAsset &model,
                                                                      const std::string &defaultTexture);

        [[nodiscard]] Error::ErrorCode SaveAsAsset(const std::string &assetPath) const;


        [[nodiscard]] ModelLod &GetLod(uint32_t index);

        void SortLODs();

        [[nodiscard]] bool AddLod(const std::string &path);

        void RemoveLod(uint32_t index);

        [[nodiscard]] uint32_t GetLodCount() const;

        [[nodiscard]] bool ValidateLodDistances();


        [[nodiscard]] std::vector<uint32_t> &GetSkin(uint32_t index);

        [[nodiscard]] uint32_t GetSkinCount() const;

        void AddSkin();

        void RemoveSkin(uint32_t index);

        [[nodiscard]] uint32_t GetMaterialsPerSkin() const;


        [[nodiscard]] Material &GetMaterial(uint32_t index);

        [[nodiscard]] uint32_t GetMaterialCount() const;

        void AddMaterial(const Material &material);

        void RemoveMaterial(uint32_t index);


        void GetVertexBuffer(uint32_t lodIndex, DataWriter &writer);

        BoundingBox &GetBoundingBox();

        CollisionModelType &GetCollisionModelType();

        [[nodiscard]] size_t GetNumHulls() const;

        ConvexHull &GetHull(size_t index);

        void AddHull(const ConvexHull &hull);

        void AddHulls(const std::string &path);

        void RemoveHull(size_t index);

        StaticCollisionMesh &GetStaticCollisionMesh();

        void SetStaticCollisionMesh(const StaticCollisionMesh &mesh);

        static constexpr uint8_t MODEL_ASSET_VERSION = 1;

    private:
        std::vector<Material> materials{};
        std::vector<std::vector<uint32_t>> skins{};
        std::vector<ModelLod> lods{};

        CollisionModelType collisionModelType = CollisionModelType::NONE;
        BoundingBox boundingBox{};
        std::vector<ConvexHull> convexHulls{};
        StaticCollisionMesh staticCollisionMesh{};

        void SaveToBuffer(std::vector<uint8_t> &buffer) const;

        static bool LODSortCompare(const ModelLod &a, const ModelLod &b);
};
