//
// Created by droc101 on 7/12/25.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <libassets/util/AssetReader.h>
#include <libassets/util/Error.h>
#include <vector>

class SoundAsset final
{
    public:
        SoundAsset() = default;

        static constexpr uint8_t SOUND_ASSET_VERSION = 1;

        /**
         * Create a SoundAsset from a GSND file
         * @param assetPath The path to the GSND file
         * @param sound The SoundAsset to populate
         */
        [[nodiscard]] static Error::ErrorCode CreateFromAsset(const char *assetPath, SoundAsset &sound);

        /**
         * Create a SoundAsset from a WAV file
         * @param wavPath The path to the WAV file
         * @param sound The SoundAsset to populate
         */
        [[nodiscard]] static Error::ErrorCode CreateFromWAV(const char *wavPath, SoundAsset &sound);

        /**
         * Save this SoundAsset as a WAV file
         * @param wavPath The path to the WAV file
         */
        [[nodiscard]] Error::ErrorCode SaveAsWAV(const char *wavPath) const;

        /**
         * Save this SoundAsset as a GSND file
         * @param assetPath The path to the GSND file
         */
        [[nodiscard]] Error::ErrorCode SaveAsAsset(const char *assetPath) const;

        /**
         * Get the WAV file data
         */
        [[nodiscard]] const std::vector<uint8_t> &GetData() const;
        /**
         * Get the size of the WAV file data
         */
        [[nodiscard]] size_t GetDataSize() const;

    private:
        std::vector<uint8_t> wavData;

        void SaveToBuffer(std::vector<uint8_t> &buffer) const;
};
