//
// Created by droc101 on 7/23/25.
//

#pragma once

#include <cstdint>
#include <libassets/util/Error.h>
#include <string>
#include <vector>

class ShaderAsset final
{
    public:

        enum class ShaderKind : uint8_t
        {
            SHADER_KIND_FRAGMENT,
            SHADER_KIND_VERTEX,
            SHADER_KIND_COMPUTE
        };

        ShaderAsset() = default;

        ShaderKind kind = ShaderKind::SHADER_KIND_FRAGMENT;

        static constexpr uint8_t SHADER_ASSET_VERSION = 1;
        static constexpr std::string SHADER_ASSET_EXTENSION = "gshd";

        /**
         * Create a ShaderAsset from a GSHD file
         * @param assetPath The path to the GSHD file
         * @param shader The ShaderAsset to populate
         */
        [[nodiscard]] static Error::ErrorCode CreateFromAsset(const char *assetPath, ShaderAsset &shader);

        /**
         * Create a ShaderAsset from a GLSL file
         * @param glslPath The path to the GLSL file
         * @param shader The ShaderAsset to populate
         */
        [[nodiscard]] static Error::ErrorCode CreateFromGlsl(const char *glslPath, ShaderAsset &shader);

        /**
         * Save this ShaderAsset as a GLSL file
         * @param glslPath The path to the GLSL file
         */
        [[nodiscard]] Error::ErrorCode SaveAsGlsl(const char *glslPath) const;

        /**
         * Save this ShaderAsset as a GSHD file
         * @param assetPath The path to the GSHD file
         * @param enableOptimization
         * @param errorLog An optional pointer to a std::string which will be filled with the compile logs
         * @return
         */
        [[nodiscard]] Error::ErrorCode SaveAsAsset(const char *assetPath,
                                                   bool enableOptimization,
                                                   std::string *errorLog = nullptr) const;

        /**
         * Get the GLSL in this ShaderAsset
         */
        [[nodiscard]] std::string &GetGLSL();

    private:
        std::string glsl;

        [[nodiscard]] Error::ErrorCode SaveToBuffer(const char *assetPath,
                                                    bool enableOptimization,
                                                    std::vector<uint8_t> &buffer,
                                                    std::string *errorLog) const;
};
