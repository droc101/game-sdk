//
// Created by droc101 on 6/25/25.
//

#include <cstring>
#include "libassets/TextureAsset.h"

char *get_arg(const char *arg_name, const int argc, char *argv[])
{
    for (int i = 0; i < argc - 1; i++)
    {
        if (strcmp(argv[i], arg_name) == 0)
        {
            if (argv[i + 1][0] == '-')
            {
                continue;
            }
            return argv[i + 1];
        }
    }
    return nullptr;
}

bool has_arg_standalone(const char *arg_name, const int argc, char *argv[])
{
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], arg_name) == 0)
        {
            return true;
        }
    }
    return false;
}

bool has_arg_with_value(const char *arg_name, const int argc, char *argv[])
{
    for (int i = 0; i < argc - 1; i++)
    {
        if (strcmp(argv[i], arg_name) == 0)
        {
            if (argv[i + 1][0] == '-')
            {
                continue;
            }
            return true;
        }
    }
    return false;
}

#include <cstdio>
int main(const int argc, char *argv[])
{
    TextureAsset t;
    if (has_arg_with_value("--import", argc, argv))
    {
        t = TextureAsset::CreateFromAsset(get_arg("--import", argc, argv));
    } else if (has_arg_with_value("--open", argc, argv))
    {
        t = TextureAsset::CreateFromImage(get_arg("--open", argc, argv));
    }

    if (has_arg_with_value("--export", argc, argv))
    {
        t.SaveAsImage(get_arg("--export", argc, argv), TextureAsset::ImageFormat::IMAGE_FORMAT_PNG);
    } else if (has_arg_with_value("--save", argc, argv))
    {
        t.SaveAsAsset(get_arg("--save", argc, argv));
    }

    return 0;
}
