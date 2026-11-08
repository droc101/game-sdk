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
        static const Error &OpenFileR(const std::string &filename,
                                      std::ifstream &stream,
                                      std::ios::openmode mode = std::ios::in);
        static const Error &ReadFileToString(const std::string &filename, std::string &output);
        static const Error &ReadFileToBuffer(const std::string &filename, std::vector<uint8_t> &output);
        static const Error &CreateFileDataReader(const std::string &filename, DataReader &reader);

        static const Error &OpenFileW(const std::string &filename,
                                      std::ofstream &stream,
                                      std::ios::openmode mode = std::ios::out);
        static const Error &WriteStringToFile(const std::string &filename, const std::string &contents);
        static const Error &WriteBufferToFile(const std::string &filename, const std::vector<uint8_t> &buffer);
};
