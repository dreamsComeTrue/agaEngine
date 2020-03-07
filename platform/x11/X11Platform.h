// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#pragma once

#include "platform/Platform.h"

namespace aga
{
    class X11Platform : public PlatformBase
    {
    public:
        X11Platform();
        ~X11Platform();

        void Initialize() override;

        std::vector<const char *> GetRequiredExtensions() override;
    };
}  // namespace aga
