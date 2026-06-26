//
// Created by droc101 on 7/22/25.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <libassets/util/Error.h>
#include <list>
#include <shaderc/shaderc.h>
#include <shaderc/shaderc.hpp>
#include <string>
#include <vector>

class ShaderCompiler
{
        /**
         * An implementation of shaderc's IncluderInterface that is used to include source glsl files.
         * All include types are treated as being relative, and there is no cap on include depth.
         * Does not handle reading from shader assets, only from source files.
         */
        class SDKIncluder: public shaderc::CompileOptions::IncluderInterface
        {
            public:
                shaderc_include_result *GetInclude(const char *requestedSource,
                                                   shaderc_include_type type,
                                                   const char *requestingSource,
                                                   size_t includeDepth) override;

                void ReleaseInclude(shaderc_include_result *data) override;

            private:
                std::list<shaderc_include_result> includeResults{};
        };

    public:
        ShaderCompiler() = delete;

        ShaderCompiler(std::string glslSource, shaderc_shader_kind shaderKind, std::string shaderName, bool optimize);

        ShaderCompiler(const std::filesystem::path &path, shaderc_shader_kind shaderKind, bool optimize);

        [[nodiscard]] Error::ErrorCode Compile(std::vector<uint32_t> &outputSpirv);

        [[nodiscard]] const std::string &GetErrorMessage() const;

    private:
        shaderc::CompileOptions options{};

        shaderc_shader_kind shaderKind;

        std::string glslSource;

        std::string shaderName;

        std::string errorMessage;
};
