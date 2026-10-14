//
// Created by droc101 on 7/23/25.
//

#pragma once

#include <cstdint>
#include <libassets/asset/Asset.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <string>

class ShaderAsset final: public Asset
{
    public:

        enum class ShaderType : uint8_t
        {
            SHADER_KIND_FRAGMENT,
            SHADER_KIND_VERTEX,
            SHADER_KIND_COMPUTE,
            SHADER_KIND_GEOMETRY,
        };

        ShaderAsset();

        ShaderType type;

        static constexpr std::string SHADER_ASSET_EXTENSION = "gshd";

        void Reset() override;

        [[nodiscard]] Error::ErrorCode SaveToBuffer(DataWriter &writer) const override;
        [[nodiscard]] Error::ErrorCode SaveToBufferEx(DataWriter &writer,
                                                      bool enableOptimization,
                                                      bool debugInfo,
                                                      std::string *errorLog = nullptr,
                                                      const std::string &shaderFilename = "glsl_source") const;

        [[nodiscard]] Error::ErrorCode SaveToAssetEx(const std::string &filePath,
                                                     bool enableOptimization,
                                                     bool debugInfo,
                                                     std::string *errorLog = nullptr,
                                                     const std::string &shaderFilename = "glsl_source",
                                                     bool dumpSpvBinary = false) const;

        [[nodiscard]] Error::ErrorCode Import(const std::string &filePath) override;

        [[nodiscard]] AssetType GetAssetType() const override;
        [[nodiscard]] uint8_t GetAssetTypeVersion() const override;

    private:
        static constexpr uint8_t SHADER_ASSET_VERSION = 2;

        std::string glsl;
};
