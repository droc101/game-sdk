//
// Created by droc101 on 7/23/25.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <libassets/asset/Asset.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <string>
#include <string_view>
#include <vector>

class FontAsset final: public Asset
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

        static constexpr std::string_view FONT_VALID_CHARS = "!\"#$%&'()*+,-./"
                                                             "0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
                                                             "abcdefghijklmnopqrstuvwxyz{|}~";
        static constexpr size_t FONT_MAX_SYMBOLS = FONT_VALID_CHARS.length();

        [[nodiscard]] Error::ErrorCode LoadFromBuffer(DataReader &reader) override;
        [[nodiscard]] Error::ErrorCode SaveToBuffer(DataWriter &writer) const override;

        [[nodiscard]] AssetType GetAssetType() const override;
        [[nodiscard]] uint8_t GetAssetTypeVersion() const override;

        /**
         * Get the list of valid characters formatted nicely for display (such as "a (0x61)")
         */
        static std::vector<std::string> GetCharListForDisplay();

    private:
        static constexpr uint8_t FONT_ASSET_VERSION = 1;
};
