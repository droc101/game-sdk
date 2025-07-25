//
// Created by droc101 on 7/23/25.
//

#ifndef FONTASSET_H
#define FONTASSET_H

#include <cstring>
#include <vector>
#include "libassets/util/Error.h"


class FontAsset {
    public:
        FontAsset() = default;

        [[nodiscard]] static Error::ErrorCode CreateFromAsset(const char *assetPath, FontAsset &font);

        [[nodiscard]] Error::ErrorCode SaveAsAsset(const char *assetPath) const;

        static constexpr uint8_t FONT_ASSET_VERSION = 1;
        static constexpr const char* FONT_VALID_CHARS = "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
        static constexpr size_t FONT_MAX_SYMBOLS = strlen(FONT_VALID_CHARS);

        uint8_t textureHeight = 1;
        uint8_t baseline = 1;
        uint8_t charSpacing = 1;
        uint8_t lineSpacing = 1;
        uint8_t charWidth = 1;
        uint8_t spaceWidth = 1;
        uint8_t defaultSize = 1;
        bool uppercaseOnly{};
        std::string texture{};
        std::vector<char> chars{};
        std::vector<uint8_t> char_widths{};

        static std::vector<std::string> GetCharListForDisplay();

    private:
        void SaveToBuffer(std::vector<uint8_t> &buffer) const;
};



#endif //FONTASSET_H
