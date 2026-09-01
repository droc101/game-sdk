//
// Created by droc101 on 7/22/25.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <glslang/Public/ShaderLang.h>
#include <libassets/util/Error.h>
#include <list>
#include <string>
#include <vector>

class ShaderCompiler
{
        /**
         * An implementation of glslang's Includer interface that is used to include source glsl files.
         * All include types are treated as being relative, and there is no cap on include depth.
         * Does not handle reading from shader assets, only from source files.
         */
        class SDKIncluder: public glslang::TShader::Includer
        {
            public:
                static SDKIncluder &Get();

                IncludeResult *includeLocal(const char *requestedSource,
                                            const char *requestingSource,
                                            size_t includeDepth) override;

                void releaseInclude(IncludeResult *data) override;

            private:
                std::list<IncludeResult> includeResults{};
        };

    public:
        ShaderCompiler() = delete;

        ShaderCompiler(std::string glslSource, EShLanguage shaderType, std::string shaderName, bool optimize);

        ShaderCompiler(const std::filesystem::path &path, EShLanguage shaderKind, bool optimize);

        [[nodiscard]] Error::ErrorCode Compile(std::vector<uint32_t> &outputSpirv);

        [[nodiscard]] const std::string &GetErrorMessage() const;

    private:
        EShLanguage shaderType;

        std::string glslSource;

        std::string shaderPath;

        bool optimize;

        std::string compileLog;
};
