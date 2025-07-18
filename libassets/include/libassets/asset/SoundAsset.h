//
// Created by droc101 on 7/12/25.
//

#ifndef MUSICASSET_H
#define MUSICASSET_H
#include <cstdint>
#include <vector>


class SoundAsset {
    public:
        SoundAsset() = default;

        [[nodiscard]] static SoundAsset CreateFromAsset(const char *assetPath);

        [[nodiscard]] static SoundAsset CreateFromWAV(const char *wavPath);

        void SaveAsWAV(const char *wavPath) const;

        void SaveAsAsset(const char *assetPath) const;

        [[nodiscard]] const std::vector<uint8_t> &GetData() const;
        [[nodiscard]] size_t GetDataSize() const;

    private:
        std::vector<uint8_t> wavData;

        void SaveToBuffer(std::vector<uint8_t> &buffer) const;
};



#endif //MUSICASSET_H
