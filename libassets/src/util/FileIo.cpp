//
// Created by droc101 on 8/8/26.
//

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <istream>
#include <libassets/util/DataReader.h>
#include <libassets/util/Error.h>
#include <libassets/util/FileIo.h>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

Error FileIo::OpenFileR(const std::string &filename, std::ifstream &stream, const std::ios::openmode mode)
{
    if (!std::filesystem::exists(filename)) {
        return Errors::FILE_NOT_FOUND;
    }
    if (!std::filesystem::is_regular_file(filename)) {
        return Errors::CANT_OPEN_FILE;
    }
    stream = std::ifstream(filename, mode);
    if (!stream.is_open())
    {
        return Errors::CANT_OPEN_FILE;
    }
    return Errors::OK;
}

Error FileIo::OpenFileW(const std::string &filename, std::ofstream &stream, const std::ios::openmode mode)
{
    stream = std::ofstream(filename, mode);
    if (!stream.is_open())
    {
        return Errors::CANT_OPEN_FILE;
    }
    return Errors::OK;
}

Error FileIo::ReadFileToString(const std::string &filename, std::string &output)
{
    std::ifstream file;
    Error openError = OpenFileR(filename, file);
    if (openError.Failed())
    {
        return openError;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    output = ss.str();
    file.close();
    return Errors::OK;
}

Error FileIo::ReadFileToBuffer(const std::string &filename, std::vector<uint8_t> &output)
{
    if (!output.empty())
    {
        return Errors::INVALID_ARGUMENT;
    }
    std::ifstream file;
    Error openError = OpenFileR(filename, file, std::ios::binary);
    if (openError.Failed())
    {
        return openError;
    }
    file.seekg(0, std::ios::seekdir::_S_end);
    const std::ios::pos_type size = file.tellg();
    output.resize(size);
    file.seekg(0, std::ios::seekdir::_S_beg);
    file.read(reinterpret_cast<std::istream::char_type *>(output.data()), size);
    return Errors::OK;
}

Error FileIo::CreateFileDataReader(const std::string &filename, DataReader &reader)
{
    std::vector<uint8_t> backingBuffer{};
    Error readError = ReadFileToBuffer(filename, backingBuffer);
    if (readError.Failed())
    {
        return readError;
    }
    reader = DataReader(backingBuffer);
    return Errors::OK;
}

Error FileIo::WriteStringToFile(const std::string &filename, const std::string &contents)
{
    std::ofstream file;
    Error openError = OpenFileW(filename, file);
    if (openError.Failed())
    {
        return openError;
    }
    file << contents;
    file.close();
    return Errors::OK;
}

Error FileIo::WriteBufferToFile(const std::string &filename, const std::vector<uint8_t> &buffer)
{
    std::ofstream file;
    Error openError = OpenFileW(filename, file, std::ios::binary);
    if (openError.Failed())
    {
        return openError;
    }
    file.write(reinterpret_cast<const std::ostream::char_type *>(buffer.data()), buffer.size());
    file.close();
    return Errors::OK;
}
