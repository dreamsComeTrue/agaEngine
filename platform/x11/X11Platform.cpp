// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#include "X11Platform.h"

namespace aga
{
    X11Platform::X11Platform()
    {
    }

    X11Platform::~X11Platform()
    {
    }

    void X11Platform::Initialize()
    {
    }

    std::vector<const char *> X11Platform::GetRequiredExtensions()
    {
        std::vector<const char *> extensions;

        extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);

        return extensions;
    }
}  // namespace aga