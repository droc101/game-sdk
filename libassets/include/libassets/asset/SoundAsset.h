//
// Created by droc101 on 7/12/25.
//

#ifndef MUSICASSET_H
#define MUSICASSET_H

#include <cstdint>
#include <vector>
#include <libassets/util/AssetReader.h>


class SoundAsset
{
    public:
        SoundAsset() = default;

        [[nodiscard]] static Error::ErrorCode CreateFromAsset(const char *assetPath, SoundAsset &sound);

        [[nodiscard]] static Error::ErrorCode CreateFromWAV(const char *wavPath, SoundAsset &sound);

        [[nodiscard]] Error::ErrorCode SaveAsWAV(const char *wavPath) const;

        [[nodiscard]] Error::ErrorCode SaveAsAsset(const char *assetPath) const;

        [[nodiscard]] const std::vector<uint8_t> &GetData() const;
        [[nodiscard]] size_t GetDataSize() const;

        static constexpr uint8_t SOUND_ASSET_VERSION = 1;

    private:
        std::vector<uint8_t> wavData;

        void SaveToBuffer(std::vector<uint8_t> &buffer) const;
};


#endif //MUSICASSET_H
