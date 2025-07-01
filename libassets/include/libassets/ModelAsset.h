//
// Created by droc101 on 6/26/25.
//

#pragma once

#include <array>
#include <assimp/mesh.h>
#include <cstdint>
#include <string>
#include <vector>
#include "DataReader.h"
#include "DataWriter.h"

class ModelAsset
{
    public:
        enum class ModelShader : uint32_t // NOLINT(*-enum-size)
        {
            SHADER_SKY,
            SHADER_UNSHADED,
            SHADER_SHADED
        };

        class Vertex
        {
            public:
                explicit Vertex(DataReader &reader);
                explicit Vertex(const aiMesh *mesh, uint32_t vertexIndex);

                bool operator==(const Vertex &other) const;

                std::array<float, 3> position{};
                std::array<float, 3> normal{};
                std::array<float, 2> uv{};
        };

        class Material
        {
            public:
                explicit Material(DataReader &reader);
                Material(std::string texture, uint32_t color, ModelShader shader);

                std::string texture{};
                uint32_t color{};
                ModelShader shader{};
        };

        class ModelLod
        {
            public:
                ModelLod() = default;
                explicit ModelLod(DataReader &reader, uint32_t materialCount);

                float distance{};
                std::vector<Vertex> vertices{};
                std::vector<uint32_t> indexCounts{};
                std::vector<std::vector<uint32_t>> indices{};
        };

        /**
         * Please use @c ModelAsset::Create* instead.
         */
        ModelAsset() = default;

        static void CreateFromAsset(const char *assetPath, ModelAsset &modelAsset);

        static void CreateFromStandardModel(const char *objPath, ModelAsset &model);

        [[nodiscard]] static ModelLod CreateLodFromStandardModel(const char *filePath, float distance);

        [[nodiscard]] const ModelLod &GetLod(size_t index) const;

        [[nodiscard]] const Material *GetSkin(size_t index) const;

        [[nodiscard]] size_t GetSkinCount() const;

        [[nodiscard]] size_t GetLodCount() const;

        [[nodiscard]] size_t GetMaterialCount() const;

        void GetVertexBuffer(size_t lodIndex, DataWriter &writer) const;

        void SaveAsAsset(const char *assetPath) const;

        // void SaveAsObj(const char *objPath) const;

    private:
        std::vector<ModelLod> lods{};
        std::vector<std::vector<Material>> skins{};

        void SaveToBuffer(std::vector<uint8_t> &buffer) const;
};

template<> struct std::hash<ModelAsset::Vertex>
{
        size_t operator()(const ModelAsset::Vertex &vertex) const noexcept;
};
