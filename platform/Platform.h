// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#pragma once

#if defined(_WIN32)
#include "windows/WindowsPlatform.h"
#elif defined(__linux)
#include "x11/X11Platform.h"
#else
// platform not yet supported
#error Platform not yet supported
#endif

#include <vulkan/vulkan.h>

namespace aga
{
}  // namespace aga
