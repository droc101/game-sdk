//
// Created by droc101 on 7/18/25.
//

#pragma once

#include <cstdint>
#include <libassets/type/ModelVertex.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <string>
#include <vector>

class ModelLod
{
        struct ModelComponent
        {
                std::vector<uint32_t> indices;
                glm::vec3 centerOffset;
                float radius;
        };

    public:
        ModelLod() = default;

        ModelLod(DataReader &reader, uint32_t componentCount);

        ModelLod(const std::string &filePath, float distance, Error::ErrorCode &status);

        float distance{};
        float unitsPerLuxel{};
        glm::uvec2 lightmapSize{1};
        std::vector<ModelVertex> vertices{};
        uint32_t totalIndexCount{};
        std::vector<ModelComponent> components{};

        void Export(const char *path) const;

        void Write(DataWriter &writer) const;

        bool CalculateLightmapUvs();

        /**
         * Flip Y axis UVs in this LOD
         */
        void FlipVerticalUVs();
};
