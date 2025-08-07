//
// Created by droc101 on 7/23/25.
//

#pragma once

#include <cstdint>
#include <libassets/util/Error.h>
#include <vector>

class ShaderAsset final
{
    public:
        enum class ShaderPlatform : uint8_t
        {
            PLATFORM_OPENGL,
            PLATFORM_VULKAN
        };

        enum class ShaderType : uint8_t
        {
            SHADER_TYPE_FRAG,
            SHADER_TYPE_VERT
        };

        ShaderAsset() = default;

        [[nodiscard]] static Error::ErrorCode CreateFromAsset(const char *assetPath, ShaderAsset &shader);

        [[nodiscard]] static Error::ErrorCode CreateFromGlsl(const char *glslPath, ShaderAsset &shader);

        [[nodiscard]] Error::ErrorCode SaveAsGlsl(const char *glslPath) const;

        [[nodiscard]] Error::ErrorCode SaveAsAsset(const char *assetPath) const;

        [[nodiscard]] std::string &GetGLSL();

        ShaderPlatform platform = ShaderPlatform::PLATFORM_VULKAN;
        ShaderType type = ShaderType::SHADER_TYPE_FRAG;

        static constexpr uint8_t SHADER_ASSET_VERSION = 1;

    private:
        std::string glsl;

        [[nodiscard]] Error::ErrorCode SaveToBuffer(std::vector<uint8_t> &buffer) const;
};
