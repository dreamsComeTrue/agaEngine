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
        void _DestroyInstance();
        
        bool _InitDevice();
        void _DestroyDevice();

    private:
        VkInstance m_VulkanInstance;        
        VkDevice m_VulkanDevice;
        VkPhysicalDevice m_VulkanPhysicalDevice;
    };
}  // namespace aga
