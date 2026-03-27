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
    public:
        ModelLod() = default;

        ModelLod(DataReader &reader, uint32_t materialsPerSkin);

        ModelLod(const std::string &filePath, float distance, Error::ErrorCode &status);

        void Export(const char *path) const;

        void Write(DataWriter &writer) const;

        void FlipVerticalUVs();

        float distance{};

        std::vector<ModelVertex> vertices{};

        std::vector<uint32_t> indexCounts{};

        std::vector<std::vector<uint32_t>> materialIndices{};
};
