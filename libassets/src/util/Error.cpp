//
// Created by droc101 on 7/18/25.
//

#include <libassets/util/Error.h>

std::string Error::ErrorString(ErrorCode e)
{
    switch (e)
    {
        case ErrorCode::E_OK:
            return "OK";
        case ErrorCode::E_INVALID_HEADER:
            return "Invalid asset header";
        case ErrorCode::E_COMPRESSION_ERROR:
            return "Compression error";
        case ErrorCode::E_INVALID_BODY:
            return "Invalid asset body";
        case ErrorCode::E_INVALID_ARGUMENT:
            return "Invalid argument";
        case ErrorCode::E_FILE_NOT_FOUND:
            return "File not found";
        case ErrorCode::E_CANT_OPEN_FILE:
            return "Can't open file";
        case ErrorCode::E_INCORRECT_FORMAT:
            return "Incorrect File Format";
        case ErrorCode::E_SHADER_PARSE_ERROR:
            return "Shader Parse Error";
        case ErrorCode::E_SHADER_LINK_ERROR:
            return "Shader Link Error";
        case ErrorCode::E_INCORRECT_VERSION:
            return "Incorrect Version";
        case ErrorCode::E_UNKNOWN:
        default:
            return "Unknown Error";
    }
}
