// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#pragma once

#include "platform/Platform.h"
#include "platform/PlatformFileSystem.h"

namespace aga
{
    class X11PlatformFileSystem : public PlatformFileSystemBase
    {
    public:
        X11PlatformFileSystem();
        ~X11PlatformFileSystem();

    public:
        String ReadEntireFileTextMode(const String &path) override;
        String ReadEntireFileBinaryMode(const String &path) override;
    };
}  // namespace aga
