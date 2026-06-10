//
// Created by droc101 on 7/23/25.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <libassets/util/Error.h>
#include <string>
#include <string_view>
#include <vector>

class FontAsset final
{
    public:
        FontAsset() = default;

        /// The height of the texture
        uint8_t textureHeight = 1;
        /// The baseline position, starting from the top
        uint8_t baseline = 1;
        /// The spacing between characters
        uint8_t charSpacing = 1;
        /// The spacing between lines
        uint8_t lineSpacing = 1;
        /// The width of any characters not explicitly set in @c charWidths
        uint8_t charWidth = 1;
        /// The width of a space
        uint8_t spaceWidth = 1;
        /// The default size of this font
        uint8_t defaultSize = 1;
        /// Whether lowercase characters should be treated and rendered as uppercase
        bool uppercaseOnly{};
        /// The path to the texture this font uses
        std::string texture{};
        /// The characters this font contains
        std::vector<char> chars{};
        /// The widths of the characters in this font
        std::vector<uint8_t> charWidths{};

        static constexpr uint8_t FONT_ASSET_VERSION = 1;

        static constexpr std::string_view FONT_VALID_CHARS = "!\"#$%&'()*+,-./"
                                                             "0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
                                                             "abcdefghijklmnopqrstuvwxyz{|}~";
        static constexpr size_t FONT_MAX_SYMBOLS = FONT_VALID_CHARS.length();

        /**
         * Create a FontAsset from a GFON file
         * @param assetPath The path to the GFON file
         * @param font The FontAsset to populate
         */
        [[nodiscard]] static Error::ErrorCode CreateFromAsset(const char *assetPath, FontAsset &font);

        /**
         * Save this FontAsset as a GFON file
         * @param assetPath The path to the GFON file
         */
        [[nodiscard]] Error::ErrorCode SaveAsAsset(const char *assetPath) const;

        /**
         * Get the list of valid characters formatted nicely for display (such as "a (0x61)")
         */
        static std::vector<std::string> GetCharListForDisplay();

    private:
        void SaveToBuffer(std::vector<uint8_t> &buffer) const;
};
