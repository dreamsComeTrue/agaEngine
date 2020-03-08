// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#pragma once

#include "core/String.h"
#include "core/Typedefs.h"
#include "platform/Platform.h"

namespace aga
{
    class VulkanRenderer;

    class PlatformWindowBase
    {
    public:
        PlatformWindowBase();
        virtual ~PlatformWindowBase();

        virtual bool Initialize(const char *title, uint32_t width = 1024,
                                uint32_t height = 768) = 0;
        virtual void Destroy() = 0;

        virtual bool Update() = 0;

        void Close();

        void SetRenderer(VulkanRenderer *renderer);

        virtual bool CreateVulkanSurface() = 0;
        virtual void DestroyVulkanSurface() = 0;

        virtual bool CreateSwapChain() = 0;
        virtual void DestroySwapChain() = 0;

    protected:
        VulkanRenderer *m_Renderer;
        String m_Name;
        uint32_t m_Width;
        uint32_t m_Height;
        uint32_t m_SurfaceWidth;
        uint32_t m_SurfaceHeight;
        bool m_ShouldRun;

        VkSurfaceKHR m_VulkanSurface;
        VkSurfaceCapabilitiesKHR m_SurfaceCapabilities;
        VkSurfaceFormatKHR m_SurfaceFormat;
        
        VkSwapchainKHR m_SwapChain;
        uint32_t m_SwapChainImageCount;
    };

    class PlatformWindow
    {
    public:
        static PlatformWindowBase *getInstance();

    private:
        PlatformWindow()
        {
        }

    public:
        PlatformWindow(PlatformWindow const &) = delete;
        void operator=(PlatformWindow const &) = delete;

    private:
        static PlatformWindowBase *s_PLatformWindowBase;
    };
}  // namespace aga
