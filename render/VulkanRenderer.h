// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#pragma once

#include "core/String.h"
#include "core/math/Rect2D.h"
#include "platform/Platform.h"

namespace aga
{
    class PlatformWindowBase;

    struct QueueFamilyIndices
    {
        static const int GRAPHICS_BIT = 1 << 0;
        static const int PRESENT_BIT = 1 << 1;

        uint32_t GraphicsIndex;
        uint32_t PresentIndex;

        int valid_bit = 0;

        bool IsValid()
        {
            return (valid_bit & GRAPHICS_BIT) && (valid_bit & PRESENT_BIT);
        }
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR SurfaceCapabilities;
        std::vector<VkSurfaceFormatKHR> Formats;
        std::vector<VkPresentModeKHR> PresentModes;
    };

    class VulkanRenderer
    {
    public:
        VulkanRenderer();
        ~VulkanRenderer();

        bool Initialize();
        void Destroy();

        void SetPlatformWindow(PlatformWindowBase *window);

        bool BeginRender();
        bool RenderFrame();
        bool EndRender();

        bool CreateSwapChain();
        void DestroySwapChain();
        void RecreateSwapChain();

        bool CreateSwapChainImages();
        void DestroySwapChainImages();

        bool CreateDepthStencilImage();
        void DestroyDepthStencilImage();

        bool CreateRenderPass();
        void DestroyRenderPass();

        bool CreateGraphicsPipeline();
        void DestroyGraphicsPipeline();

        bool CreateFrameBuffers();
        void DestroyFrameBuffers();

        bool CreateSynchronizations();
        void DestroySynchronizations();

        bool CreateCommandPool();
        void DestroyCommandPool();

        bool CreateVertexBuffer();
        void DestroyVertexBuffer();

        bool CreateCommandBuffers();

        const VkInstance GetVulkanInstance();
        const VkDevice GetVulkanDevice();
        const VkPhysicalDevice GetPhysicalDevice();
        const VkPhysicalDeviceMemoryProperties &GetVulkanPhysicalDeviceMemoryProperties() const;
        const VkQueue &GetVulkanQueue() const;

        VkRenderPass GetRenderPass();
        VkFramebuffer GetActiveFrameBuffer();
        Rect2D GetSurfaceSize();

        void SetFrameBufferResized(bool resized);

        static void CheckResult(VkResult result, const String &message);

    private:
        void _PrepareExtensions();
        bool _InitInstance();
        void _DestroyInstance();

        bool _InitPhysicalDevice();
        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
        SwapChainSupportDetails FindSwapChainDetails(VkPhysicalDevice device);

        void _CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                           VkBuffer &buffer, VkDeviceMemory &bufferMemory);
        void _CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

        VkSurfaceFormatKHR _ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
        VkPresentModeKHR _ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
        Rect2D _ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

        bool _IsPhysicalDeviceSuitable(VkPhysicalDevice device);
        bool _InitLogicalDevice();
        void _DestroyLogicalDevice();

        bool _InitDebugging();
        bool _DestroyDebugging();

        VkShaderModule _CreateShaderModule(const String &data);

        uint32_t FindMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties *memoryProperties,
                                     const VkMemoryRequirements *memoryRequirements,
                                     const VkMemoryPropertyFlags requiredPropertyFlags);
        uint32_t _FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    private:
        PlatformWindowBase *m_PlatformWindow;
        VkInstance m_VulkanInstance;
        VkDevice m_VulkanDevice;
        VkPhysicalDevice m_VulkanPhysicalDevice;
        uint32_t m_GraphicsFamilyIndex;
        uint32_t m_PresentFamilyIndex;
        VkQueue m_GraphicsQueue;
        VkQueue m_PresentQueue;
        VkPhysicalDeviceMemoryProperties m_PhysicalDeviceMemoryProperties;

        VkCommandPool m_CommandPool;
        std::vector<VkCommandBuffer> m_CommandBuffers;

        std::vector<const char *> m_InstanceLayers;
        std::vector<const char *> m_InstanceExtensions;

        std::vector<const char *> m_DeviceLayers;
        std::vector<const char *> m_DeviceExtensions;

        VkDebugReportCallbackEXT m_DebugReport;
        VkDebugReportCallbackCreateInfoEXT m_DebugCallbackCreateInfo;

        uint32_t m_SurfaceWidth;
        uint32_t m_SurfaceHeight;
        VkPresentModeKHR m_PresentMode;
        VkSurfaceKHR m_VulkanSurface;
        VkSurfaceCapabilitiesKHR m_SurfaceCapabilities;
        VkFormat m_DepthStencilFormat;
        bool m_IsStencilAvailable;
        VkSurfaceFormatKHR m_SurfaceFormat;

        VkSwapchainKHR m_SwapChain;
        uint32_t m_SwapChainImageCount;
        uint32_t m_ActiveSwapChainImageID;

        VkPipelineLayout m_PipelineLayout;
        VkPipeline m_GraphicsPipeline;

        std::vector<VkSemaphore> m_ImageAvailableSemaphores;
        std::vector<VkSemaphore> m_RenderFinishedSemaphores;
        std::vector<VkFence> m_SyncFences;
        std::vector<VkFence> m_ImagesInProcess;

        VkRenderPass m_RenderPass;

        size_t m_CurrentFrame;
        bool m_FramebufferResized;

        std::vector<VkImage> m_SwapChainImages;
        std::vector<VkImageView> m_SwapChainImagesViews;
        std::vector<VkFramebuffer> m_FrameBuffers;

        VkBuffer m_VertexBuffer;
        VkDeviceMemory m_VertexBufferMemory;

        VkImage m_DepthStencilImage;
        VkDeviceMemory m_DepthStencilImageMemory;
        VkImageView m_DepthStencilImageView;
    };
}  // namespace aga
