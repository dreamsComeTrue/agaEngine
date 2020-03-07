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

m_ShouldRun
}  // namespace aga
