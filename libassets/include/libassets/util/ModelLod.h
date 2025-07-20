//
// Created by droc101 on 7/18/25.
//

#ifndef MODELLOD_H
#define MODELLOD_H

#include <libassets/util/DataReader.h>
#include <libassets/util/ModelVertex.h>
#include <libassets/util/DataWriter.h>

class ModelLod
{
    public:
        ModelLod() = default;
        explicit ModelLod(DataReader &reader, uint32_t materialCount);
        explicit ModelLod(const char *filePath, float distance);

        float distance{};
        std::vector<ModelVertex> vertices{};
        std::vector<uint32_t> indexCounts{};
        std::vector<std::vector<uint32_t>> indices{};

        void Export(const char *path) const;

        void Write(DataWriter &writer) const;
};


#endif //MODELLOD_H
