//
// Created by droc101 on 7/18/25.
//

#ifndef ERROR_H
#define ERROR_H
#include <cstdint>
#include <string>


class Error
{
    public:
        enum class ErrorCode : uint8_t
        {
            E_OK,
            E_INVALID_HEADER,
            E_COMPRESSION_ERROR,
            E_INVALID_BODY,
            E_INVALID_ARGUMENT,
            E_FILE_NOT_FOUND,
            E_CANT_OPEN_FILE,
            E_UNKNOWN
        };

        Error() = delete;

        static std::string ErrorString(ErrorCode e);
};


#endif //ERROR_H
