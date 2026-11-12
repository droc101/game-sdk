//
// Created by droc101 on 11/8/26.
//

#include <libassets/util/Error.h>
#include <string>

Error::Error(const std::string &errorMessage)
{
    this->ok = false;
    this->errorMessage = errorMessage;
}

Error::Error(const std::string &errorMessage, const bool ok)
{
    this->ok = ok;
    this->errorMessage = errorMessage;
}

const std::string &Error::Message() const
{
    return errorMessage;
}

bool Error::Ok() const
{
    return ok;
}

bool Error::Failed() const
{
    return !ok;
}
