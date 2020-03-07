// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#include "Platform.h"

#if defined(_WIN32)
#include "windows/WindowsPlatform.h"
#elif defined(__linux)
#include "x11/X11Platform.h"
#else
// platform not yet supported
#error Platform not yet supported
#endif

namespace aga
{
#if defined(_WIN32)
    PlatformBase *Platform::s_PlatformBase = new X11Platform();
#elif defined(__linux)
    PlatformBase *Platform::s_PlatformBase = new X11Platform();
#endif

    PlatformBase *Platform::getInstance()
    {
        return s_PlatformBase;
    }
}  // namespace aga