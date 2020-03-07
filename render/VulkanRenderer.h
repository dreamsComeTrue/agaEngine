// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#pragma once

#include "core/Common.h"

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

        bool _InitDebugging();
        bool _DestroyDebugging();

        void _CreateCommandPool();

    private:
        VkInstance m_VulkanInstance;
        VkDevice m_VulkanDevice;
        VkPhysicalDevice m_VulkanPhysicalDevice;
        uint32_t m_GraphicsFamilyIndex;
        VkQueue m_Queue;

        std::vector<const char *> m_InstanceLayers;
        std::vector<const char *> m_InstanceExtensions;

        std::vector<const char *> m_DeviceLayers;
        std::vector<const char *> m_DeviceExtensions;

        VkDebugReportCallbackEXT m_DebugReport;
        VkDebugReportCallbackCreateInfoEXT m_DebugCallbackCreateInfo;
    };
}  // namespace aga
