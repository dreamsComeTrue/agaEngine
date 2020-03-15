// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#include "PlatformFileSystem.h"

#if defined(_WIN32)
#include "platform/windows/WindowsPlatformFileSystem.h"
#elif defined(__linux)
#include "platform/x11/X11PlatformFileSystem.h"
#else
// platform not yet supported
#error Platform not yet supported
#endif

namespace aga
{
    PlatformFileSystemBase::PlatformFileSystemBase() :
        m_Renderer(nullptr),
        m_ShouldRun(true)
    {
    }

    PlatformFileSystemBase::~PlatformFileSystemBase()
    {
    }

#if defined(_WIN32)
    PlatformFileSystemBase *PlatformFileSystem::s_PlatformFileSystemBase = new X11PlatformFileSystem();
#elif defined(__linux)
    PlatformFileSystemBase *PlatformFileSystem::s_PlatformFileSystemBase = new X11PlatformFileSystem();
#endif

    PlatformFileSystemBase *PlatformFileSystem::getInstance()
    {
        return s_PlatformFileSystemBase;
    }
}  // namespace aga