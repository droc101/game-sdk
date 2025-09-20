//
// Created by droc101 on 7/18/25.
//

#pragma once

#include <algorithm>
#include <cstdint>
#include <format>
#include <libassets/libassets.h>
#include <string>
#include <utility>

class Error
{
    public:
        enum class ErrorCode : uint8_t
        {
            OK,
            INVALID_HEADER,
            COMPRESSION_ERROR,
            INVALID_BODY,
            INVALID_ARGUMENT,
            FILE_NOT_FOUND,
            CANT_OPEN_FILE,
            UNKNOWN,
            INCORRECT_FORMAT,
            SHADER_PARSE_ERROR,
            SHADER_LINK_ERROR,
            INCORRECT_VERSION,
            INVALID_DIRECTORY
        };

        Error() = delete;

        static constexpr std::string ErrorString(const ErrorCode error)
        {
            switch (error)
            {
                case ErrorCode::OK:
                    return "OK";
                case ErrorCode::INVALID_HEADER:
                    return "Invalid asset header";
                case ErrorCode::COMPRESSION_ERROR:
                    return "Compression error";
                case ErrorCode::INVALID_BODY:
                    return "Invalid asset body";
                case ErrorCode::INVALID_ARGUMENT:
                    return "Invalid argument";
                case ErrorCode::FILE_NOT_FOUND:
                    return "File not found";
                case ErrorCode::CANT_OPEN_FILE:
                    return "Can't open file";
                case ErrorCode::INCORRECT_FORMAT:
                    return "Incorrect File Format";
                case ErrorCode::SHADER_PARSE_ERROR:
                    return "Shader Parse Error";
                case ErrorCode::SHADER_LINK_ERROR:
                    return "Shader Link Error";
                case ErrorCode::INCORRECT_VERSION:
                    return "Incorrect Version";
                case ErrorCode::INVALID_DIRECTORY:
                    return "Invalid Directory Path";
                case ErrorCode::UNKNOWN:
                default:
                    return "Unknown Error";
            }
        }
};

template<> struct std::formatter<Error::ErrorCode, char>
{
        template<class ParseContext> constexpr ParseContext::iterator parse(ParseContext &context)
        {
            return context.begin();
        }

        template<class FormatContext> FormatContext::iterator format(const Error::ErrorCode errorCode,
                                                                     FormatContext &context) const
        {
            return std::ranges::copy(std::move(Error::ErrorString(errorCode)), context.out()).out;
        }
};
