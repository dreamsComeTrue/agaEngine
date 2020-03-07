// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#pragma once

#include "core/Common.h"

#include <vulkan/vulkan.h>

namespace aga
{
    class PlatformBase
    {
    public:
        PlatformBase()
        {
        }

        virtual ~PlatformBase()
        {
        }

        virtual void Initialize() = 0;

        virtual std::vector<const char *> GetRequiredExtensions() = 0;
    };

    class Platform
    {
    public:
        static PlatformBase *getInstance();

    private:
        Platform()
        {
        }

    public:
        Platform(Platform const &) = delete;
        void operator=(Platform const &) = delete;

    private:
        static PlatformBase *s_PlatformBase;
    };
}  // namespace aga
