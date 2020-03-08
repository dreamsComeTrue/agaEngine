// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#include "PlatformWindow.h"
#include "core/Logger.h"
#include "render/VulkanRenderer.h"

#if defined(_WIN32)
#include "platform/windows/WindowsPlatformWindow.h"
#elif defined(__linux)
#include "platform/x11/X11PlatformWindow.h"
#else
// platform not yet supported
#error Platform not yet supported
#endif

namespace aga
{
    PlatformWindowBase::PlatformWindowBase() :
        m_Renderer(nullptr),
        m_ShouldRun(true),
        m_VulkanSurface(VK_NULL_HANDLE),
        m_SwapChain(VK_NULL_HANDLE),
        m_SwapChainImageCount(2),
        m_RenderPass(VK_NULL_HANDLE),
        m_DepthStencilImage(VK_NULL_HANDLE),
        m_DepthStencilImageView(VK_NULL_HANDLE),
        m_DepthStencilImageMemory(VK_NULL_HANDLE),
        m_DepthStencilFormat(VK_FORMAT_UNDEFINED),
        m_IsStencilAvailable(false)
    {
    }

    PlatformWindowBase::~PlatformWindowBase()
    {
    }

    void PlatformWindowBase::Close()
    {
        m_ShouldRun = false;
    }

    void PlatformWindowBase::SetRenderer(VulkanRenderer *renderer)
    {
        m_Renderer = renderer;
    }

#if defined(_WIN32)
    PlatformWindowBase *PlatformWindow::s_PLatformWindowBase = new X11PlatformWindow();
#elif defined(__linux)
    PlatformWindowBase *PlatformWindow::s_PLatformWindowBase = new X11PlatformWindow();
#endif

    PlatformWindowBase *PlatformWindow::getInstance()
    {
        return s_PLatformWindowBase;
    }

    bool PlatformWindowBase::CreateVulkanSurface()
    {
        _CreateVulkanSurface();

        VkPhysicalDevice device = m_Renderer->GetPhysicalDevice();

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_VulkanSurface, &m_SurfaceCapabilities);

        if (m_SurfaceCapabilities.currentExtent.width < UINT32_MAX ||
            m_SurfaceCapabilities.currentExtent.height < UINT32_MAX)
        {
            m_SurfaceWidth = m_SurfaceCapabilities.currentExtent.width;
            m_SurfaceHeight = m_SurfaceCapabilities.currentExtent.height;
        }

        {
            uint32_t formatCount = 0;
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_VulkanSurface, &formatCount,
                                                 VK_NULL_HANDLE);
            std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_VulkanSurface, &formatCount,
                                                 surfaceFormats.data());

            if (formatCount == 0)
            {
                LOG_ERROR_F("X11PlatformWindow Surface format missing\n");

                return false;
            }

            if (surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
            {
                m_SurfaceFormat.format = VK_FORMAT_R8G8B8A8_UNORM;
                m_SurfaceFormat.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
            }
            else
            {
                m_SurfaceFormat = surfaceFormats[0];
            }
        }

        LOG_DEBUG_F("X11PlatformWindow Vulkan Surface created\n");

        return true;
    }

    void PlatformWindowBase::DestroyVulkanSurface()
    {
        vkDestroySurfaceKHR(m_Renderer->GetVulkanInstance(), m_VulkanSurface, VK_NULL_HANDLE);
        m_VulkanSurface = VK_NULL_HANDLE;

        LOG_DEBUG_F("X11PlatformWindow Vulkan Surface destroyed\n");
    }

    bool PlatformWindowBase::CreateSwapChain()
    {
        if (m_SwapChainImageCount < m_SurfaceCapabilities.minImageCount + 1)
        {
            m_SwapChainImageCount = m_SurfaceCapabilities.minImageCount + 1;
        }

        if (m_SurfaceCapabilities.maxImageCount > 0)
        {
            if (m_SwapChainImageCount > m_SurfaceCapabilities.maxImageCount)
            {
                m_SwapChainImageCount = m_SurfaceCapabilities.maxImageCount;
            }
        }

        VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
        {
            uint32_t presentModeCount = 0;
            if (vkGetPhysicalDeviceSurfacePresentModesKHR(m_Renderer->GetPhysicalDevice(),
                                                          m_VulkanSurface, &presentModeCount,
                                                          VK_NULL_HANDLE) != VK_SUCCESS)
            {
                LOG_ERROR_F("X11PlatformWindow CreateSwapChain failed\n");
                return false;
            }

            std::vector<VkPresentModeKHR> presentModes(presentModeCount);
            if (vkGetPhysicalDeviceSurfacePresentModesKHR(m_Renderer->GetPhysicalDevice(),
                                                          m_VulkanSurface, &presentModeCount,
                                                          presentModes.data()) != VK_SUCCESS)
            {
                LOG_ERROR_F("X11PlatformWindow CreateSwapChain failed\n");
                return false;
            }

            //  Look for V-Sync support
            for (VkPresentModeKHR mode : presentModes)
            {
                if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
                {
                    presentMode = mode;
                    break;
                }
            }
        }

        VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
        swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapChainCreateInfo.surface = m_VulkanSurface;
        swapChainCreateInfo.minImageCount = m_SwapChainImageCount;
        swapChainCreateInfo.imageFormat = m_SurfaceFormat.format;
        swapChainCreateInfo.imageColorSpace = m_SurfaceFormat.colorSpace;
        swapChainCreateInfo.imageExtent.width = m_SurfaceWidth;
        swapChainCreateInfo.imageExtent.height = m_SurfaceHeight;
        swapChainCreateInfo.imageArrayLayers = 1;
        swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapChainCreateInfo.queueFamilyIndexCount = 0;
        swapChainCreateInfo.pQueueFamilyIndices = VK_NULL_HANDLE;
        swapChainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapChainCreateInfo.presentMode = presentMode;
        swapChainCreateInfo.clipped = VK_TRUE;
        swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(m_Renderer->GetVulkanDevice(), &swapChainCreateInfo,
                                 VK_NULL_HANDLE, &m_SwapChain) != VK_SUCCESS)
        {
            LOG_ERROR_F("X11PlatformWindow CreateSwapChain failed\n");
            return false;
        }

        if (vkGetSwapchainImagesKHR(m_Renderer->GetVulkanDevice(), m_SwapChain,
                                    &m_SwapChainImageCount, VK_NULL_HANDLE) != VK_SUCCESS)
        {
            LOG_ERROR_F("X11PlatformWindow CreateSwapChain failed\n");
            return false;
        }

        LOG_DEBUG_F("X11PlatformWindow Vulkan SwapChain created\n");

        return true;
    }

    void PlatformWindowBase::DestroySwapChain()
    {
        vkDestroySwapchainKHR(m_Renderer->GetVulkanDevice(), m_SwapChain, VK_NULL_HANDLE);

        LOG_DEBUG_F("X11PlatformWindow Vulkan SwapChain destroyed\n");
    }

    bool PlatformWindowBase::CreateSwapChainImages()
    {
        m_SwapChainImages.resize(m_SwapChainImageCount);
        m_SwapChainImagesViews.resize(m_SwapChainImageCount);

        if (vkGetSwapchainImagesKHR(m_Renderer->GetVulkanDevice(), m_SwapChain,
                                    &m_SwapChainImageCount, m_SwapChainImages.data()) != VK_SUCCESS)
        {
            LOG_ERROR_F("X11PlatformWindow CreateSwapChainImages failed\n");
            return false;
        }

        for (uint32_t i = 0; i < m_SwapChainImageCount; ++i)
        {
            VkImageViewCreateInfo imageViewCreateInfo = {};
            imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewCreateInfo.image = m_SwapChainImages[i];
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewCreateInfo.format = m_SurfaceFormat.format;
            imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
            imageViewCreateInfo.subresourceRange.levelCount = 1;
            imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
            imageViewCreateInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(m_Renderer->GetVulkanDevice(), &imageViewCreateInfo, nullptr,
                                  &m_SwapChainImagesViews[i]) != VK_SUCCESS)
            {
                LOG_ERROR_F("X11PlatformWindow CreateSwapChainImages failed\n");
                return false;
            }
        }

        LOG_DEBUG_F("X11PlatformWindow Vulkan SwapChain Images created\n");

        return true;
    }

    void PlatformWindowBase::DestroySwapChainImages()
    {
        for (uint32_t i = 0; i < m_SwapChainImagesViews.size(); ++i)
        {
            vkDestroyImageView(m_Renderer->GetVulkanDevice(), m_SwapChainImagesViews[i],
                               VK_NULL_HANDLE);
        }

        LOG_DEBUG_F("X11PlatformWindow Vulkan SwapChain Images destroyed\n");
    }

    bool PlatformWindowBase::CreateDepthStencilImage()
    {
        std::vector<VkFormat> formats{VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT,
                                      VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT,
                                      VK_FORMAT_D16_UNORM};

        for (VkFormat &format : formats)
        {
            VkFormatProperties formatProperties = {};
            vkGetPhysicalDeviceFormatProperties(m_Renderer->GetPhysicalDevice(), format,
                                                &formatProperties);

            if (formatProperties.optimalTilingFeatures &
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                m_DepthStencilFormat = format;
                break;
            }
        }

        if (m_DepthStencilFormat == VK_FORMAT_UNDEFINED)
        {
            LOG_ERROR_F("X11PlatformWindow DepthStencil format not selected\n");

            return false;
        }

        if (m_DepthStencilFormat == VK_FORMAT_D32_SFLOAT_S8_UINT ||
            m_DepthStencilFormat == VK_FORMAT_D24_UNORM_S8_UINT ||
            m_DepthStencilFormat == VK_FORMAT_D16_UNORM_S8_UINT ||
            m_DepthStencilFormat == VK_FORMAT_S8_UINT)
        {
            m_IsStencilAvailable = true;
        }

        VkImageCreateInfo imageCreateInfo = {};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.flags = 0;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = m_DepthStencilFormat;
        imageCreateInfo.extent.width = m_SurfaceWidth;
        imageCreateInfo.extent.height = m_SurfaceHeight;
        imageCreateInfo.extent.depth = 1;
        imageCreateInfo.mipLevels = 1;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.queueFamilyIndexCount = VK_QUEUE_FAMILY_IGNORED;
        imageCreateInfo.pQueueFamilyIndices = VK_NULL_HANDLE;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        vkCreateImage(m_Renderer->GetVulkanDevice(), &imageCreateInfo, VK_NULL_HANDLE,
                      &m_DepthStencilImage);

        VkMemoryRequirements imageMemoryRequirements;
        vkGetImageMemoryRequirements(m_Renderer->GetVulkanDevice(), m_DepthStencilImage,
                                     &imageMemoryRequirements);

        uint32_t memoryIndex =
            FindMemoryTypeIndex(&m_Renderer->GetVulkanPhysicalDeviceMemoryProperties(),
                                &imageMemoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (memoryIndex == UINT32_MAX)
        {
            LOG_ERROR_F("X11PlatformWindow DepthStencil memory index not selected\n");

            return false;
        }

        VkMemoryAllocateInfo memoryAllocateInfo = {};
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.allocationSize = imageMemoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex = memoryIndex;

        vkAllocateMemory(m_Renderer->GetVulkanDevice(), &memoryAllocateInfo, VK_NULL_HANDLE,
                         &m_DepthStencilImageMemory);
        vkBindImageMemory(m_Renderer->GetVulkanDevice(), m_DepthStencilImage,
                          m_DepthStencilImageMemory, 0);

        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = m_DepthStencilImage;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = m_DepthStencilFormat;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask =
            VK_IMAGE_ASPECT_DEPTH_BIT | (m_IsStencilAvailable ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        vkCreateImageView(m_Renderer->GetVulkanDevice(), &imageViewCreateInfo, VK_NULL_HANDLE,
                          &m_DepthStencilImageView);

        LOG_DEBUG_F("X11PlatformWindow DepthStencil Image created\n");

        return true;
    }

    void PlatformWindowBase::DestroyDepthStencilImage()
    {
        vkDestroyImageView(m_Renderer->GetVulkanDevice(), m_DepthStencilImageView, VK_NULL_HANDLE);
        m_DepthStencilImageView = VK_NULL_HANDLE;

        vkFreeMemory(m_Renderer->GetVulkanDevice(), m_DepthStencilImageMemory, VK_NULL_HANDLE);
        m_DepthStencilImageMemory = VK_NULL_HANDLE;

        vkDestroyImage(m_Renderer->GetVulkanDevice(), m_DepthStencilImage, VK_NULL_HANDLE);
        m_DepthStencilImage = VK_NULL_HANDLE;

        LOG_DEBUG_F("X11PlatformWindow DepthStencil Image destroyed\n");
    }

    bool PlatformWindowBase::CreateRenderPass()
    {
        std::array<VkAttachmentDescription, 2> attachments = {};
        attachments[0].flags = 0;
        attachments[0].format = m_DepthStencilFormat;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        attachments[1].flags = 0;
        attachments[1].format = m_SurfaceFormat.format;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        std::array<VkAttachmentReference, 1> subPass0ColorAttachments = {};
        subPass0ColorAttachments[0].attachment = 1;
        subPass0ColorAttachments[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference subPass0DepthStencilAttachment = {};
        subPass0DepthStencilAttachment.attachment = 0;
        subPass0DepthStencilAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        std::array<VkSubpassDescription, 1> subPasses = {};
        subPasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subPasses[0].colorAttachmentCount = subPass0ColorAttachments.size();
        subPasses[0].pColorAttachments = subPass0ColorAttachments.data();
        subPasses[0].pDepthStencilAttachment = &subPass0DepthStencilAttachment;

        VkRenderPassCreateInfo renderPassCreateInfo = {};
        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.attachmentCount = attachments.size();
        renderPassCreateInfo.pAttachments = attachments.data();
        renderPassCreateInfo.subpassCount = subPasses.size();
        renderPassCreateInfo.pSubpasses = subPasses.data();

        if (vkCreateRenderPass(m_Renderer->GetVulkanDevice(), &renderPassCreateInfo, VK_NULL_HANDLE,
                               &m_RenderPass) != VK_SUCCESS)
        {
            LOG_ERROR_F("X11PlatformWindow Error while creating RenderPass\n");

            return false;
        }

        LOG_DEBUG_F("X11PlatformWindow RenderPass created\n");

        return true;
    }

    void PlatformWindowBase::DestroyRenderPass()
    {
        vkDestroyRenderPass(m_Renderer->GetVulkanDevice(), m_RenderPass, VK_NULL_HANDLE);

        LOG_DEBUG_F("X11PlatformWindow RenderPass destroyed\n");
    }

    uint32_t PlatformWindowBase::FindMemoryTypeIndex(
        const VkPhysicalDeviceMemoryProperties *memoryProperties,
        const VkMemoryRequirements *memoryRequirements,
        const VkMemoryPropertyFlags requiredPropertyFlags)
    {
        for (uint32_t i = 0; i < memoryProperties->memoryTypeCount; ++i)
        {
            if (memoryRequirements->memoryTypeBits & (1 << i))
            {
                if ((memoryProperties->memoryTypes[i].propertyFlags & requiredPropertyFlags) ==
                    requiredPropertyFlags)
                {
                    return i;
                }
            }
        }

        return UINT32_MAX;
    }

}  // namespace aga