// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#pragma once

#include <vulkan/vulkan.h>

namespace aga
{
    class VulkanRenderer
    {
    public:
        VulkanRenderer();
        ~VulkanRenderer();

        void RenderFrame();

    private:
        void _Initialize();
        void _Destroy();

        bool _InitInstance();

    private:
        VkInstance m_VulkanInstance;
    };
}  // namespace aga
