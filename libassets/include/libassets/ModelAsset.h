//
// Created by droc101 on 6/26/25.
//

#ifndef MODELASSET_H
#define MODELASSET_H
#include <array>
#include <cstdint>
#include <vector>

class ModelAsset
{

    public:
        enum ModelShader: uint32_t // NOLINT(*-enum-size)
        {
            SHADER_SKY,
            SHADER_UNSHADED,
            SHADER_SHADED
        };

        struct Vertex
        {
            std::array<float, 3> position = std::array<float, 3>();
            std::array<float, 3> normal = std::array<float, 3>();
            std::array<float, 2> uv = std::array<float, 2>();

            bool operator==(const Vertex &o) const;
        };

        struct Material
        {
            char *texture;
            uint32_t color;
            ModelShader shader;
        };

        struct ModelLod
        {
            float distance;
            std::vector<Vertex> vertices = std::vector<Vertex>();
            std::vector<uint32_t> indexCounts = std::vector<uint32_t>();
            std::vector<std::vector<uint32_t>> indices = std::vector<std::vector<uint32_t>>();
        };

        /**
         * Please use @c ModelAsset::Create* instead.
         */
        ModelAsset() = default;

        ~ModelAsset() = default;

        [[nodiscard]] static ModelAsset CreateFromAsset(const char *assetPath);

        [[nodiscard]] static ModelAsset CreateFromStandardModel(const char *objPath);

        [[nodiscard]] static ModelLod CreateLodFromStandardModel(const char *filePath, float distance);

        [[nodiscard]] ModelLod GetLod(size_t index) const;

        [[nodiscard]] const Material *GetSkin(size_t index) const;

        [[nodiscard]] size_t GetSkinCount() const;

        [[nodiscard]] size_t GetLodCount() const;

        [[nodiscard]] size_t GetMaterialCount() const;

        [[nodiscard]] const uint8_t *GetVertexBuffer(size_t lodIndex, size_t *size) const;

        void SaveAsAsset(const char *assetPath) const;

        // void SaveAsObj(const char *objPath) const;

    private:
        std::vector<ModelLod> lods = std::vector<ModelLod>();
        std::vector<std::vector<Material>> skins = std::vector<std::vector<Material>>();

        uint8_t *SaveToBuffer(size_t *outSize) const;
};

template<> struct std::hash<ModelAsset::Vertex>
{
    size_t operator()(const ModelAsset::Vertex &v) const noexcept;
};

#endif //MODELASSET_H
