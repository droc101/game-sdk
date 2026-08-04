//
// Created by droc101 on 7/23/25.
//

#pragma once

#include <cstdint>
#include <libassets/asset/Asset.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <string>

class ShaderAsset final: public Asset
{
    public:

        enum class ShaderKind : uint8_t
        {
            SHADER_KIND_FRAGMENT,
            SHADER_KIND_VERTEX,
            SHADER_KIND_COMPUTE,
            SHADER_KIND_GEOMETRY,
        };

        ShaderAsset() = default;

        ShaderKind kind = ShaderKind::SHADER_KIND_FRAGMENT;

        static constexpr std::string SHADER_ASSET_EXTENSION = "gshd";

        [[nodiscard]] Error::ErrorCode LoadFromBuffer(DataReader &reader) override;
        [[nodiscard]] Error::ErrorCode SaveToBuffer(DataWriter &writer) const override;
        [[nodiscard]] Error::ErrorCode SaveToBufferEx(DataWriter &writer,
                                                      bool enableOptimization,
                                                      std::string *errorLog = nullptr,
                                                      const std::string &shaderFilename = "glsl_source") const;

        [[nodiscard]] Error::ErrorCode SaveToAssetEx(const std::string &filePath,
                                                     bool enableOptimization,
                                                     std::string *errorLog = nullptr,
                                                     const std::string &shaderFilename = "glsl_source") const;

        [[nodiscard]] Error::ErrorCode Import(const std::string &filePath) override;
        [[nodiscard]] Error::ErrorCode Export(const std::string &filePath) const override;

        [[nodiscard]] AssetType GetAssetType() const override;
        [[nodiscard]] uint8_t GetAssetTypeVersion() const override;

        /**
         * Get the GLSL in this ShaderAsset
         */
        [[nodiscard]] std::string &GetGLSL();

    private:
        static constexpr uint8_t SHADER_ASSET_VERSION = 1;

        std::string glsl;
};
