//
// Created by droc101 on 7/18/25.
//

#pragma once

class Error
{
        friend class Errors;

    public:
        Error() = delete;
        explicit Error(const std::string &errorMessage);
        template<typename... Args> explicit Error(std::format_string<Args...> fmt, Args &&...args)
        {
            this->ok = false;
            this->errorMessage = std::format(fmt, std::forward<Args>(args)...);
        }

        const std::string &Message() const;
        bool Ok() const;
        bool Failed() const;

    private:
        Error(const std::string &errorMessage, bool ok);

        std::string errorMessage;
        bool ok;
};

class Errors
{
    public:
        Errors() = delete;

        static inline const Error OK = Error("OK", true);
        static inline const Error INVALID_HEADER = Error("Invalid header data");
        static inline const Error COMPRESSION_ERROR = Error("Compression error");
        static inline const Error INVALID_BODY = Error("Invalid body data");
        static inline const Error INVALID_ARGUMENT = Error("Invalid argument");
        static inline const Error FILE_NOT_FOUND = Error("File not found");
        static inline const Error CANT_OPEN_FILE = Error("Can't open file");
        static inline const Error INCORRECT_FORMAT = Error("Incorrect file format");
        static inline const Error INCORRECT_VERSION = Error("Incorrect file version");
        static inline const Error INVALID_DIRECTORY = Error("Invalid directory path");
        static inline const Error NOT_FOUND = Error("Not found");
        static inline const Error NOT_IMPLEMENTED = Error("Not implemented");
        static inline const Error PERMISSION_DENIED = Error("Permission denied");


        /// @warning Avoid returning this whenever possible
        static inline const Error UNKNOWN = Error("Unknown error");
};

template<> struct std::formatter<Error, char>
{
        template<class ParseContext> constexpr ParseContext::iterator parse(ParseContext &context)
        {
            return context.begin();
        }

        template<class FormatContext> FormatContext::iterator format(const Error error, FormatContext &context) const
        {
            return std::ranges::copy(std::move(error.Message()), context.out()).out;
        }
};
