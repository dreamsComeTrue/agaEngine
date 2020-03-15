// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#include "X11PlatformFileSystem.h"
#include "core/Logger.h"
#include "core/Macros.h"
#include "platform/Platform.h"

#include <fstream>

namespace aga
{
    String X11PlatformFileSystem::ReadEntireFileTextMode(const String &path)
    {
        return "";
    }

    String X11PlatformFileSystem::ReadEntireFileBinaryMode(const String &path)
    {
        std::ifstream file(path, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            LOG_ERROR_F("Failed to open file: " + path);

            return "";
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        String result(buffer);

        return result;
    }

}  // namespace aga