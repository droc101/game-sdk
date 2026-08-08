//
// Created by droc101 on 7/12/25.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <libassets/asset/Asset.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <string>
#include <vector>

class SoundAsset final: public Asset
{
    public:
        SoundAsset();

        void Reset() override;

        [[nodiscard]] Error::ErrorCode LoadFromBuffer(DataReader &reader) override;
        [[nodiscard]] Error::ErrorCode SaveToBuffer(DataWriter &writer) const override;

        [[nodiscard]] Error::ErrorCode Import(const std::string &filePath) override;
        [[nodiscard]] Error::ErrorCode Export(const std::string &filePath) const override;

        [[nodiscard]] AssetType GetAssetType() const override;
        [[nodiscard]] uint8_t GetAssetTypeVersion() const override;

        /**
         * Get the WAV file data
         */
        [[nodiscard]] const std::vector<uint8_t> &GetData() const;
        /**
         * Get the size of the WAV file data
         */
        [[nodiscard]] size_t GetDataSize() const;

    private:
        static constexpr uint8_t SOUND_ASSET_VERSION = 1;

        std::vector<uint8_t> wavData;
};
