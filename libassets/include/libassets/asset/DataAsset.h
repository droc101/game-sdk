//
// Created by droc101 on 1/20/26.
//

#pragma once

#include <cstdint>
#include <libassets/asset/Asset.h>
#include <libassets/type/Param.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <string>

class DataAsset final: public Asset
{
    public:
        DataAsset() = default;

        KvList data{};

        [[nodiscard]] Error::ErrorCode LoadFromBuffer(DataReader &reader) override;
        [[nodiscard]] Error::ErrorCode SaveToBuffer(DataWriter &writer) const override;

        [[nodiscard]] Error::ErrorCode Import(const std::string &filePath) override;
        [[nodiscard]] Error::ErrorCode Export(const std::string &filePath) const override;

        [[nodiscard]] AssetType GetAssetType() const override;
        [[nodiscard]] uint8_t GetAssetTypeVersion() const override;

        /**
         * Create a DataAsset from a KVL file
         * @param kvlPath The path to the KVL file
         */
        [[nodiscard]] Error::ErrorCode CreateFromKvlFile(const std::string &kvlPath);

        /**
         * Save this DataAsset as a KVL file
         * @param kvlFile The path to the KVL file
         */
        [[nodiscard]] Error::ErrorCode SaveAsKvlFile(const std::string &kvlFile) const;

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

        static constexpr uint8_t DATA_ASSET_VERSION = 1;

        // "KVLF" in ASCII
        static constexpr uint32_t KVL_MAGIC = 0x464c564b;
        static constexpr uint16_t KVL_VERSION = 1;
};
