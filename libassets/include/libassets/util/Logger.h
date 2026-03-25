//
// Created by droc101 on 3/25/26.
//

#ifndef GAME_SDK_LOGGER_H
#define GAME_SDK_LOGGER_H

#include <format>
#include <iostream>
#include <string>

class Logger
{
    public:
        Logger() = delete;

        template<typename... Args> static void Verbose(std::format_string<Args...> fmt, Args &&...args)
        {
            if (verbose)
            {
                LogInternal(LOG_LEVEL_VERBOSE, fmt, std::forward<Args>(args)...);
            }
        }

        template<typename... Args> static void Info(std::format_string<Args...> fmt, Args &&...args)
        {
            LogInternal(LOG_LEVEL_INFO, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args> static void Warning(std::format_string<Args...> fmt, Args &&...args)
        {
            LogInternal(LOG_LEVEL_WARN, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args> static void Error(std::format_string<Args...> fmt, Args &&...args)
        {
            LogInternal(LOG_LEVEL_ERROR, fmt, std::forward<Args>(args)...);
        }

        static inline bool ansi = true;
        static inline bool verbose = false;

    private:
        struct LogLevel
        {
                const char *prefix;
                const char *ansiPrefix;
                bool verboseOnly;
        };

        static constexpr LogLevel LOG_LEVEL_VERBOSE = {
            .prefix = "[VERBOSE] ",
            .ansiPrefix = "\x1b[37m[VERBOSE] ",
            .verboseOnly = true,
        };

        static constexpr LogLevel LOG_LEVEL_INFO = {
            .prefix = "[INFO] ",
            .ansiPrefix = "\x1b[37m[INFO] ",
            .verboseOnly = true,
        };

        static constexpr LogLevel LOG_LEVEL_WARN = {
            .prefix = "[WARN] ",
            .ansiPrefix = "\x1b[33m[WARN] ",
            .verboseOnly = true,
        };

        static constexpr LogLevel LOG_LEVEL_ERROR = {
            .prefix = "[ERROR] ",
            .ansiPrefix = "\x1b[31m[ERROR] ",
            .verboseOnly = true,
        };

        template<typename... Args> static void LogInternal(const LogLevel &level,
                                                           std::format_string<Args...> fmt,
                                                           Args &&...args)
        {
            const std::string formatted = std::format(fmt, std::forward<Args>(args)...);
            std::cout << (ansi ? level.ansiPrefix : level.prefix) << formatted << std::endl;
        }
};


#endif //GAME_SDK_LOGGER_H
