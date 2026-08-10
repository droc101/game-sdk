//
// Created by droc101 on 8/8/26.
//

#include <cstdint>
#include <fstream>
#include <ios>
#include <istream>
#include <libassets/util/Error.h>
#include <libassets/util/FileIo.h>
#include <ostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

Error::ErrorCode FileIo::OpenFileR(const std::string &filename, std::ifstream &stream, const std::ios::openmode mode)
{
    if (access(filename.c_str(), F_OK))
    {
        return Error::ErrorCode::FILE_NOT_FOUND;
    }
    if (access(filename.c_str(), R_OK))
    {
        return Error::ErrorCode::PERMISSION_DENIED;
    }
    stream = std::ifstream(filename, mode);
    if (!stream.is_open())
    {
        return Error::ErrorCode::CANT_OPEN_FILE;
    }
    return Error::ErrorCode::OK;
}

Error::ErrorCode FileIo::OpenFileW(const std::string &filename, std::ofstream &stream, const std::ios::openmode mode)
{
    if (access(filename.c_str(), F_OK))
    {
        return Error::ErrorCode::FILE_NOT_FOUND;
    }
    if (access(filename.c_str(), W_OK))
    {
        return Error::ErrorCode::PERMISSION_DENIED;
    }
    stream = std::ofstream(filename, mode);
    if (!stream.is_open())
    {
        return Error::ErrorCode::CANT_OPEN_FILE;
    }
    return Error::ErrorCode::OK;
}

Error::ErrorCode FileIo::ReadFileToString(const std::string &filename, std::string &output)
{
    std::ifstream file;
    const Error::ErrorCode openError = OpenFileR(filename, file);
    if (openError != Error::ErrorCode::OK)
    {
        return openError;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    output = ss.str();
    file.close();
    return Error::ErrorCode::OK;
}

Error::ErrorCode FileIo::ReadFileToBuffer(const std::string &filename, std::vector<uint8_t> &output)
{
    if (!output.empty())
    {
        return Error::ErrorCode::INVALID_ARGUMENT;
    }
    std::ifstream file;
    const Error::ErrorCode openError = OpenFileR(filename, file, std::ios::binary);
    if (openError != Error::ErrorCode::OK)
    {
        return openError;
    }
    file.seekg(0, std::ios::seekdir::_S_end);
    const std::ios::pos_type size = file.tellg();
    output.resize(size);
    file.seekg(0, std::ios::seekdir::_S_beg);
    file.read(reinterpret_cast<std::istream::char_type *>(output.data()), size);
    return Error::ErrorCode::OK;
}

Error::ErrorCode FileIo::CreateFileDataReader(const std::string &filename, DataReader &reader)
{
    std::vector<uint8_t> backingBuffer{};
    const Error::ErrorCode readError = ReadFileToBuffer(filename, backingBuffer);
    if (readError != Error::ErrorCode::OK)
    {
        return readError;
    }
    reader = DataReader(backingBuffer);
    return Error::ErrorCode::OK;
}

Error::ErrorCode FileIo::WriteStringToFile(const std::string &filename, const std::string &contents)
{
    std::ofstream file;
    const Error::ErrorCode openError = OpenFileW(filename, file);
    if (openError != Error::ErrorCode::OK)
    {
        return openError;
    }
    file << contents;
    file.close();
    return Error::ErrorCode::OK;
}

Error::ErrorCode FileIo::WriteBufferToFile(const std::string &filename, const std::vector<uint8_t> &buffer)
{
    std::ofstream file;
    const Error::ErrorCode openError = OpenFileW(filename, file, std::ios::binary);
    if (openError != Error::ErrorCode::OK)
    {
        return openError;
    }
    file.write(reinterpret_cast<const std::ostream::char_type *>(buffer.data()), buffer.size());
    file.close();
    return Error::ErrorCode::OK;
}
