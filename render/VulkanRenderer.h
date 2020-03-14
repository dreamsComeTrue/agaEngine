// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#pragma once

#include "platform/Platform.h"
#include "core/String.h"

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
        void SetSurfaceSize(uint32_t width, uint32_t height);

        bool BeginRender();
        bool RenderFrame();
        bool EndRender();

        bool CreateVulkanSurface();
        void DestroyVulkanSurface();

        bool CreateSwapChain();
        void DestroySwapChain();

        bool CreateSwapChainImages();
        void DestroySwapChainImages();

        bool CreateDepthStencilImage();
        void DestroyDepthStencilImage();

        bool CreateRenderPass();
        void DestroyRenderPass();

        bool CreateFrameBuffers();
        void DestroyFrameBuffers();

        bool CreateSynchronizations();
        void DestroySynchronizations();

        VkCommandPool CreateCommandPool();
        void DestroyCommandPool(VkCommandPool pool);

        VkCommandBuffer CreateCommandBuffer(VkCommandPool pool);

        const VkInstance GetVulkanInstance();
        const VkDevice GetVulkanDevice();
        const VkPhysicalDevice GetPhysicalDevice();
        const VkPhysicalDeviceMemoryProperties &GetVulkanPhysicalDeviceMemoryProperties() const;
        const VkQueue &GetVulkanQueue() const;
        const VkSemaphore GetRenderCompleteSemaphore() const;

        VkRenderPass GetRenderPass();
        VkFramebuffer GetActiveFrameBuffer();
        VkExtent2D GetSurfaceSize();

    private:
        void CheckResult(VkResult result, const String& message);
        
        void _PrepareExtensions();
        bool _InitInstance();
        void _DestroyInstance();

        bool _InitDevice();
        bool _IsPhysicalDeviceSuitable(VkPhysicalDevice device);
        void _DestroyDevice();

        bool _InitDebugging();
        bool _DestroyDebugging();

        uint32_t FindMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties *memoryProperties,
                                     const VkMemoryRequirements *memoryRequirements,
                                     const VkMemoryPropertyFlags requiredPropertyFlags);

    private:
        PlatformWindowBase *m_PlatformWindow;
        VkInstance m_VulkanInstance;
        VkDevice m_VulkanDevice;
        VkPhysicalDevice m_VulkanPhysicalDevice;
        uint32_t m_GraphicsFamilyIndex;
        VkQueue m_Queue;
        VkPhysicalDeviceMemoryProperties m_PhysicalDeviceMemoryProperties;

        VkCommandPool m_CommandPool;
        VkCommandBuffer m_CommandBuffer;

        VkSemaphore m_RenderCompleteSemaphore;

        std::vector<const char *> m_InstanceLayers;
        std::vector<const char *> m_InstanceExtensions;

        std::vector<const char *> m_DeviceLayers;
        std::vector<const char *> m_DeviceExtensions;

        VkDebugReportCallbackEXT m_DebugReport;
        VkDebugReportCallbackCreateInfoEXT m_DebugCallbackCreateInfo;

        uint32_t m_SurfaceWidth;
        uint32_t m_SurfaceHeight;
        VkSurfaceKHR m_VulkanSurface;
        VkSurfaceCapabilitiesKHR m_SurfaceCapabilities;
        VkFormat m_DepthStencilFormat;
        bool m_IsStencilAvailable;
        VkSurfaceFormatKHR m_SurfaceFormat;

        VkSwapchainKHR m_SwapChain;
        uint32_t m_SwapChainImageCount;
        uint32_t m_ActiveSwapChainImageID;

        VkFence m_SwapChainImageFence;

        VkRenderPass m_RenderPass;

        std::vector<VkImage> m_SwapChainImages;
        std::vector<VkImageView> m_SwapChainImagesViews;
        std::vector<VkFramebuffer> m_FrameBuffers;

        VkImage m_DepthStencilImage;
        VkDeviceMemory m_DepthStencilImageMemory;
        VkImageView m_DepthStencilImageView;
    };
}  // namespace aga
