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

        bool CreateVulkanSurface();
        void DestroyVulkanSurface();

        bool CreateSwapChain();
        void DestroySwapChain();

        bool CreateSwapChainImages();
        void DestroySwapChainImages();

        bool CreateDepthStencilImage();
        void DestroyDepthStencilImage();

        uint32_t FindMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties *memoryProperties,
                                     const VkMemoryRequirements *memoryRequirements,
                                     const VkMemoryPropertyFlags requiredPropertyFlags);

    protected:
        virtual VkSurfaceKHR _CreateVulkanSurface() = 0;

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
        VkFormat m_DepthStencilFormat;
        bool m_IsStencilAvailable;
        VkSurfaceFormatKHR m_SurfaceFormat;

        VkSwapchainKHR m_SwapChain;
        uint32_t m_SwapChainImageCount;

        std::vector<VkImage> m_SwapChainImages;
        std::vector<VkImageView> m_SwapChainImagesViews;

        VkImage m_DepthStencilImage;
        VkDeviceMemory m_DepthStencilImageMemory;
        VkImageView m_DepthStencilImageView;
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
