// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#pragma once

#include "platform/Platform.h"

namespace aga
{
    class PlatformWindowBase;

    class VulkanRenderer
    {
    public:
        VulkanRenderer();
        ~VulkanRenderer();

        bool Initialize();
        void Destroy();
        
        void SetPlatformWindow(PlatformWindowBase *window);

        bool RenderFrame();

        void CreateCommandPool();
        
        const VkInstance GetVulkanInstance();

    private:
        bool _InitInstance();
        void _DestroyInstance();

        bool _InitDevice();
        void _DestroyDevice();

        bool _InitDebugging();
        bool _DestroyDebugging();

    private:
        PlatformWindowBase *m_PlatformWindow;
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
