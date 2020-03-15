// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#pragma once

#include "core/String.h"
#include "core/Typedefs.h"
#include "platform/Platform.h"

namespace aga
{
    class PlatformFileSystemBase
    {
    public:
        PlatformFileSystemBase();
        virtual ~PlatformFileSystemBase();

    public:
        virtual String ReadEntireFileTextMode(const String &path) = 0;
        virtual String ReadEntireFileBinaryMode(const String &path) = 0;
    };

    class PlatformFileSystem
    {
    public:
        static PlatformFileSystemBase *getInstance();

    private:
        PlatformFileSystem()
        {
        }

    public:
        PlatformFileSystem(PlatformFileSystem const &) = delete;
        void operator=(PlatformFileSystem const &) = delete;

    private:
        static PlatformFileSystemBase *s_PlatformFileSystemBase;
    };
}  // namespace aga
