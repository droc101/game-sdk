//
// Created by droc101 on 8/8/26.
//

#pragma once

#include <cstdint>
#include <ios>
#include <libassets/util/DataReader.h>
#include <libassets/util/Error.h>
#include <string>
#include <vector>

class FileIo
{
    public:
        static Error::ErrorCode OpenFileR(const std::string &filename,
                                          std::ifstream &stream,
                                          std::ios::openmode mode = std::ios::in);
        static Error::ErrorCode ReadFileToString(const std::string &filename, std::string &output);
        static Error::ErrorCode ReadFileToBuffer(const std::string &filename, std::vector<uint8_t> &output);
        static Error::ErrorCode CreateFileDataReader(const std::string &filename, DataReader &reader);

        static Error::ErrorCode OpenFileW(const std::string &filename,
                                          std::ofstream &stream,
                                          std::ios::openmode mode = std::ios::out);
        static Error::ErrorCode WriteStringToFile(const std::string &filename, const std::string &contents);
        static Error::ErrorCode WriteBufferToFile(const std::string &filename, const std::vector<uint8_t> &buffer);
};
