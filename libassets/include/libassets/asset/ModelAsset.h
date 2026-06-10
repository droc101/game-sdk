//
// Created by droc101 on 6/26/25.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <libassets/type/BoundingBox.h>
#include <libassets/type/ConvexHull.h>
#include <libassets/type/Material.h>
#include <libassets/type/ModelLod.h>
#include <libassets/type/StaticCollisionMesh.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <string>
#include <vector>

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

        static constexpr uint8_t MODEL_ASSET_VERSION = 1;

        /**
         * Create a ModelAsset from a GMDL file
         * @param assetPath The path to the GMDL file
         * @param modelAsset The ModelAsset to populate
         */
        [[nodiscard]] static Error::ErrorCode CreateFromAsset(const std::string &assetPath, ModelAsset &modelAsset);

        /**
         * Create a ModelAsset from a conventional model format
         * @param modelPath The path to the model file
         * @param model The ModelAsset to populate
         * @param defaultTexture The texture to use for materials
         */
        [[nodiscard]] static Error::ErrorCode CreateFromStandardModel(const std::string &modelPath,
                                                                      ModelAsset &model,
                                                                      const std::string &defaultTexture);

        /**
         * Save this ModelAsset to a GMDL file
         * @param assetPath The GMDL file to save to
         */
        [[nodiscard]] Error::ErrorCode SaveAsAsset(const std::string &assetPath) const;


        /**
         * Get a LOD by index
         */
        [[nodiscard]] ModelLod &GetLod(uint32_t index);

        /**
         * Sort LODs by distance
         */
        void SortLODs();

        /**
         * Add a LOD
         * @param path The path to a conventional model file to use as the LOD
         */
        [[nodiscard]] bool AddLod(const std::string &path);

        /**
         * Remove a LOD by index
         */
        void RemoveLod(uint32_t index);

        /**
         * Get the number of LODs
         */
        [[nodiscard]] uint32_t GetLodCount() const;

        /**
         * Validate the distances of LODs
         */
        [[nodiscard]] bool ValidateLodDistances();


        /**
         * Get a skin by index
         */
        [[nodiscard]] std::vector<uint32_t> &GetSkin(uint32_t index);

        /**
         * Get the number of skins
         */
        [[nodiscard]] uint32_t GetSkinCount() const;

        /**
         * Add a skin
         */
        void AddSkin();

        /**
         * Remove a skin by index
         */
        void RemoveSkin(uint32_t index);

        /**
         * Get the number of material slots per skin
         */
        [[nodiscard]] uint32_t GetMaterialsPerSkin() const;


        /**
         * Get a material by index
         */
        [[nodiscard]] Material &GetMaterial(uint32_t index);

        /**
         * Get the number of materials
         */
        [[nodiscard]] uint32_t GetMaterialCount() const;

        /**
         * Add a material
         */
        void AddMaterial(const Material &material);

        /**
         * Remove a material by index
         */
        void RemoveMaterial(uint32_t index);


        /**
         * Get a vertex buffer for a LOD
         * @param lodIndex The index of the LOD
         * @param writer The DataWriter to write the buffer to
         */
        void GetVertexBuffer(uint32_t lodIndex, DataWriter &writer);

        /**
         * Get the bounding box of this model
         */
        BoundingBox &GetBoundingBox();

        /**
         * Get the bounding box of this model
         */
        [[nodiscard]] const BoundingBox &GetBoundingBox() const;

        /**
         * Get the type of collision model this model has
         */
        CollisionModelType &GetCollisionModelType();

        /**
         * Get the number of convex hulls this model has
         */
        [[nodiscard]] size_t GetNumHulls() const;

        /**
         * Get a convex hull by index
         */
        ConvexHull &GetHull(size_t index);

        /**
         * Add a new convex hull
         */
        void AddHull(const ConvexHull &hull);

        /**
         * Add convex hulls from a model
         */
        [[nodiscard]] Error::ErrorCode AddHulls(const std::string &path);

        /**
         * Remove a hull by index
         */
        void RemoveHull(size_t index);

        /**
         * Get the static collision mesh
         */
        StaticCollisionMesh &GetStaticCollisionMesh();

        /**
         * Set the static collision mesh
         */
        void SetStaticCollisionMesh(const StaticCollisionMesh &mesh);

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
