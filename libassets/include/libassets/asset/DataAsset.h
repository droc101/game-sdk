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

        KvList data{};

        static constexpr uint8_t DATA_ASSET_VERSION = 1;

        // "KVLF" in ASCII
        static constexpr uint32_t KVL_MAGIC = 0x464c564b;
        static constexpr uint16_t KVL_VERSION = 1;

        /**
         * Create a DataAsset from a GKVL file
         * @param assetPath The path to the GKVL file
         * @param dataAsset The DataAsset to populate
         */
        [[nodiscard]] static Error::ErrorCode CreateFromAsset(const char *assetPath, DataAsset &dataAsset);

        /**
         * Create a DataAsset from a KVL file
         * @param kvlPath The path to the KVL file
         * @param dataAsset The DataAsset to populate
         */
        [[nodiscard]] static Error::ErrorCode CreateFromKvlFile(const char *kvlPath, DataAsset &dataAsset);

        /**
         * Create a DataAsset from a JSON file
         * @param jsonPath The path to the JSON file
         * @param dataAsset The DataAsset to populate
         */
        [[nodiscard]] static Error::ErrorCode CreateFromJson(const char *jsonPath, DataAsset &dataAsset);

        /**
         * Save this DataAsset to a JSON file
         * @param jsonPath The path to the JSON file
         */
        [[nodiscard]] Error::ErrorCode SaveAsJson(const char *jsonPath) const;

        /**
         * Save this DataAsset as a KVL file
         * @param kvlFile The path to the KVL file
         */
        [[nodiscard]] Error::ErrorCode SaveAsKvlFile(const char *kvlFile) const;

        /**
         * Save this DataAsset as a GKVL file
         * @param assetPath The path to the GKVL file
         */
        [[nodiscard]] Error::ErrorCode SaveAsAsset(const char *assetPath) const;

    private:
        struct KvlFileHeader
        {
            /// Magic bytes, should match @c KVL_MAGIC
            uint32_t magic;
            /// KVL format version, should match @c KVL_VERSION
            uint16_t version;
            /// Checksum of the content (the file data, minus the header)
            uint16_t checksum;
        };
        static_assert(sizeof(KvlFileHeader) == 8);

        void SaveToBuffer(std::vector<uint8_t> &buffer) const;
};
