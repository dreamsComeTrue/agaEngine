// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#include "VulkanRenderer.h"
#include "core/BuildConfig.h"
#include "core/Common.h"
#include "core/Logger.h"
#include "core/Typedefs.h"
#include "platform/Platform.h"
#include "platform/PlatformWindow.h"

namespace aga
{
    const std::vector<const char *> g_ValidationLayers = {"VK_LAYER_LUNARG_standard_validation"};

    VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugReportFlagsEXT flags,
                                                       VkDebugReportObjectTypeEXT objectType,
                                                       uint64_t sourceObject, size_t location,
                                                       int32_t code, const char *layerPrefix,
                                                       const char *message, void *userData)
    {
        String layerPart = String(" Layer[") + layerPrefix + "]: ";

        if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
        {
            LOG_INFO(layerPart + String(message) + "\n");
        }
        else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
        {
            LOG_WARNING(layerPart + String(message) + "\n");
        }
        else if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
        {
            LOG_ERROR(layerPart + String(message) + "\n");
        }
        else if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
        {
            LOG_DEBUG(layerPart + String(message) + "\n");
        }
        else
        {
            LOG_INFO(layerPart + String(message) + "\n");
        }

        return VK_FALSE;
    }

    PFN_vkCreateDebugReportCallbackEXT createDebugReportCallbackEXTFunc = VK_NULL_HANDLE;
    PFN_vkDestroyDebugReportCallbackEXT destroDebugReportCallbackEXTFunc = VK_NULL_HANDLE;

    VulkanRenderer::VulkanRenderer() :
        m_PlatformWindow(nullptr),
        m_VulkanInstance(VK_NULL_HANDLE),
        m_VulkanDevice(VK_NULL_HANDLE),
        m_VulkanPhysicalDevice(VK_NULL_HANDLE),
        m_GraphicsFamilyIndex(0),
        m_Queue(VK_NULL_HANDLE),
        m_DebugReport(VK_NULL_HANDLE),
        m_RenderCompleteSemaphore(VK_NULL_HANDLE),
        m_CommandPool(VK_NULL_HANDLE),
        m_CommandBuffer(VK_NULL_HANDLE),
        m_VulkanSurface(VK_NULL_HANDLE),
        m_SwapChain(VK_NULL_HANDLE),
        m_SwapChainImageCount(2),
        m_ActiveSwapChainImageID(0),
        m_SwapChainImageFence(VK_NULL_HANDLE),
        m_RenderPass(VK_NULL_HANDLE),
        m_DepthStencilImage(VK_NULL_HANDLE),
        m_DepthStencilImageView(VK_NULL_HANDLE),
        m_DepthStencilImageMemory(VK_NULL_HANDLE),
        m_DepthStencilFormat(VK_FORMAT_UNDEFINED),
        m_IsStencilAvailable(false)
    {
    }

    VulkanRenderer::~VulkanRenderer()
    {
    }

    bool VulkanRenderer::BeginRender()
    {
        vkAcquireNextImageKHR(m_VulkanDevice, m_SwapChain, UINT64_MAX, VK_NULL_HANDLE,
                              m_SwapChainImageFence, &m_ActiveSwapChainImageID);
        if (vkWaitForFences(m_VulkanDevice, 1, &m_SwapChainImageFence, VK_TRUE, UINT64_MAX) !=
            VK_SUCCESS)
        {
            LOG_ERROR_F("PlatformWindowBase Wait For Fences error\n");

            return false;
        }

        if (vkResetFences(m_VulkanDevice, 1, &m_SwapChainImageFence) != VK_SUCCESS)
        {
            LOG_ERROR_F("PlatformWindowBase Reset Fences error\n");

            return false;
        }

        if (vkQueueWaitIdle(m_Queue) != VK_SUCCESS)
        {
            LOG_ERROR_F("PlatformWindowBase Queue Wait Idle error\n");

            return false;
        }

        return true;
    }

    bool VulkanRenderer::EndRender()
    {
        std::vector<VkSemaphore> waitSemaphores = {m_RenderCompleteSemaphore};
        VkResult presentResult = VkResult::VK_RESULT_MAX_ENUM;

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = waitSemaphores.size();
        presentInfo.pWaitSemaphores = waitSemaphores.data();
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_SwapChain;
        presentInfo.pImageIndices = &m_ActiveSwapChainImageID;
        presentInfo.pResults = &presentResult;

        if (vkQueuePresentKHR(m_Queue, &presentInfo) != VK_SUCCESS)
        {
            LOG_ERROR_F("PlatformWindowBase Queue present error\n");

            return false;
        }

        if (presentResult != VK_SUCCESS)
        {
            LOG_ERROR_F("PlatformWindowBase Present Result error\n");

            return false;
        }

        return true;
    }

    VkRenderPass VulkanRenderer::GetRenderPass()
    {
        return m_RenderPass;
    }

    VkFramebuffer VulkanRenderer::GetActiveFrameBuffer()
    {
        return m_FrameBuffers[m_ActiveSwapChainImageID];
    }

    VkExtent2D VulkanRenderer::GetSurfaceSize()
    {
        return {m_SurfaceWidth, m_SurfaceHeight};
    }

    bool VulkanRenderer::RenderFrame()
    {
        VkCommandBufferBeginInfo commandBufferBeginInfo = {};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VkRect2D renderArea = {};
        renderArea.offset.x = 0;
        renderArea.offset.y = 0;
        renderArea.extent = GetSurfaceSize();

        vkBeginCommandBuffer(m_CommandBuffer, &commandBufferBeginInfo);
        {
            std::array<VkClearValue, 2> clearValues = {};
            clearValues[0].depthStencil.depth = 0.0f;
            clearValues[0].depthStencil.stencil = 0;

            clearValues[1].color.float32[0] = 0.0f;
            clearValues[1].color.float32[1] = 0.0f;
            clearValues[1].color.float32[2] = 1.0f;
            clearValues[1].color.float32[3] = 0.0f;

            VkRenderPassBeginInfo renderPassBeginInfo = {};
            renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassBeginInfo.renderPass = m_RenderPass;
            renderPassBeginInfo.framebuffer = GetActiveFrameBuffer();
            renderPassBeginInfo.renderArea = renderArea;
            renderPassBeginInfo.clearValueCount = clearValues.size();
            renderPassBeginInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(m_CommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdEndRenderPass(m_CommandBuffer);
        }
        vkEndCommandBuffer(m_CommandBuffer);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_CommandBuffer;
        submitInfo.waitSemaphoreCount = 0;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &m_RenderCompleteSemaphore;
        submitInfo.pWaitSemaphores = VK_NULL_HANDLE;
        submitInfo.pWaitDstStageMask = VK_NULL_HANDLE;

        vkQueueSubmit(m_Queue, 1, &submitInfo, VK_NULL_HANDLE);

        return true;
    }

    bool VulkanRenderer::CreateVulkanSurface()
    {
        m_VulkanSurface = m_PlatformWindow->CreateVulkanSurface();

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_VulkanPhysicalDevice, m_VulkanSurface,
                                                  &m_SurfaceCapabilities);

        if (m_SurfaceCapabilities.currentExtent.width < UINT32_MAX ||
            m_SurfaceCapabilities.currentExtent.height < UINT32_MAX)
        {
            m_SurfaceWidth = m_SurfaceCapabilities.currentExtent.width;
            m_SurfaceHeight = m_SurfaceCapabilities.currentExtent.height;
        }

        {
            uint32_t formatCount = 0;
            vkGetPhysicalDeviceSurfaceFormatsKHR(m_VulkanPhysicalDevice, m_VulkanSurface,
                                                 &formatCount, VK_NULL_HANDLE);
            std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(m_VulkanPhysicalDevice, m_VulkanSurface,
                                                 &formatCount, surfaceFormats.data());

            if (formatCount == 0)
            {
                LOG_ERROR_F("VulkanRenderer Surface format missing\n");

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

        LOG_DEBUG_F("VulkanRenderer Vulkan Surface created\n");

        return true;
    }

    void VulkanRenderer::DestroyVulkanSurface()
    {
        vkDestroySurfaceKHR(m_VulkanInstance, m_VulkanSurface, VK_NULL_HANDLE);
        m_VulkanSurface = VK_NULL_HANDLE;

        LOG_DEBUG_F("VulkanRenderer Vulkan Surface destroyed\n");
    }

    bool VulkanRenderer::CreateSwapChain()
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
            if (vkGetPhysicalDeviceSurfacePresentModesKHR(m_VulkanPhysicalDevice, m_VulkanSurface,
                                                          &presentModeCount,
                                                          VK_NULL_HANDLE) != VK_SUCCESS)
            {
                LOG_ERROR_F("VulkanRenderer CreateSwapChain failed\n");
                return false;
            }

            std::vector<VkPresentModeKHR> presentModes(presentModeCount);
            if (vkGetPhysicalDeviceSurfacePresentModesKHR(m_VulkanPhysicalDevice, m_VulkanSurface,
                                                          &presentModeCount,
                                                          presentModes.data()) != VK_SUCCESS)
            {
                LOG_ERROR_F("VulkanRenderer CreateSwapChain failed\n");
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

        if (vkCreateSwapchainKHR(m_VulkanDevice, &swapChainCreateInfo, VK_NULL_HANDLE,
                                 &m_SwapChain) != VK_SUCCESS)
        {
            LOG_ERROR_F("VulkanRenderer CreateSwapChain failed\n");
            return false;
        }

        if (vkGetSwapchainImagesKHR(m_VulkanDevice, m_SwapChain, &m_SwapChainImageCount,
                                    VK_NULL_HANDLE) != VK_SUCCESS)
        {
            LOG_ERROR_F("VulkanRenderer CreateSwapChain failed\n");
            return false;
        }

        LOG_DEBUG_F("VulkanRenderer Vulkan SwapChain created\n");

        return true;
    }

    void VulkanRenderer::DestroySwapChain()
    {
        vkDestroySwapchainKHR(m_VulkanDevice, m_SwapChain, VK_NULL_HANDLE);

        LOG_DEBUG_F("VulkanRenderer Vulkan SwapChain destroyed\n");
    }

    bool VulkanRenderer::CreateSwapChainImages()
    {
        m_SwapChainImages.resize(m_SwapChainImageCount);
        m_SwapChainImagesViews.resize(m_SwapChainImageCount);

        if (vkGetSwapchainImagesKHR(m_VulkanDevice, m_SwapChain, &m_SwapChainImageCount,
                                    m_SwapChainImages.data()) != VK_SUCCESS)
        {
            LOG_ERROR_F("VulkanRenderer CreateSwapChainImages failed\n");
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

            if (vkCreateImageView(m_VulkanDevice, &imageViewCreateInfo, nullptr,
                                  &m_SwapChainImagesViews[i]) != VK_SUCCESS)
            {
                LOG_ERROR_F("VulkanRenderer CreateSwapChainImages failed\n");
                return false;
            }
        }

        LOG_DEBUG_F("VulkanRenderer Vulkan SwapChain Images created\n");

        return true;
    }

    void VulkanRenderer::DestroySwapChainImages()
    {
        for (uint32_t i = 0; i < m_SwapChainImagesViews.size(); ++i)
        {
            vkDestroyImageView(m_VulkanDevice, m_SwapChainImagesViews[i], VK_NULL_HANDLE);
        }

        LOG_DEBUG_F("VulkanRenderer Vulkan SwapChain Images destroyed\n");
    }

    bool VulkanRenderer::CreateDepthStencilImage()
    {
        std::vector<VkFormat> formats{VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT,
                                      VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT,
                                      VK_FORMAT_D16_UNORM};

        for (VkFormat &format : formats)
        {
            VkFormatProperties formatProperties = {};
            vkGetPhysicalDeviceFormatProperties(m_VulkanPhysicalDevice, format, &formatProperties);

            if (formatProperties.optimalTilingFeatures &
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                m_DepthStencilFormat = format;
                break;
            }
        }

        if (m_DepthStencilFormat == VK_FORMAT_UNDEFINED)
        {
            LOG_ERROR_F("VulkanRenderer DepthStencil format not selected\n");

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

        vkCreateImage(m_VulkanDevice, &imageCreateInfo, VK_NULL_HANDLE, &m_DepthStencilImage);

        VkMemoryRequirements imageMemoryRequirements;
        vkGetImageMemoryRequirements(m_VulkanDevice, m_DepthStencilImage, &imageMemoryRequirements);

        uint32_t memoryIndex =
            FindMemoryTypeIndex(&GetVulkanPhysicalDeviceMemoryProperties(),
                                &imageMemoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (memoryIndex == UINT32_MAX)
        {
            LOG_ERROR_F("VulkanRenderer DepthStencil memory index not selected\n");

            return false;
        }

        VkMemoryAllocateInfo memoryAllocateInfo = {};
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.allocationSize = imageMemoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex = memoryIndex;

        vkAllocateMemory(m_VulkanDevice, &memoryAllocateInfo, VK_NULL_HANDLE,
                         &m_DepthStencilImageMemory);
        vkBindImageMemory(m_VulkanDevice, m_DepthStencilImage, m_DepthStencilImageMemory, 0);

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

        vkCreateImageView(m_VulkanDevice, &imageViewCreateInfo, VK_NULL_HANDLE,
                          &m_DepthStencilImageView);

        LOG_DEBUG_F("VulkanRenderer DepthStencil Image created\n");

        return true;
    }

    void VulkanRenderer::DestroyDepthStencilImage()
    {
        vkDestroyImageView(m_VulkanDevice, m_DepthStencilImageView, VK_NULL_HANDLE);
        m_DepthStencilImageView = VK_NULL_HANDLE;

        vkFreeMemory(m_VulkanDevice, m_DepthStencilImageMemory, VK_NULL_HANDLE);
        m_DepthStencilImageMemory = VK_NULL_HANDLE;

        vkDestroyImage(m_VulkanDevice, m_DepthStencilImage, VK_NULL_HANDLE);
        m_DepthStencilImage = VK_NULL_HANDLE;

        LOG_DEBUG_F("VulkanRenderer DepthStencil Image destroyed\n");
    }

    bool VulkanRenderer::CreateRenderPass()
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

        if (vkCreateRenderPass(m_VulkanDevice, &renderPassCreateInfo, VK_NULL_HANDLE,
                               &m_RenderPass) != VK_SUCCESS)
        {
            LOG_ERROR_F("VulkanRenderer Error while creating RenderPass\n");

            return false;
        }

        LOG_DEBUG_F("VulkanRenderer RenderPass created\n");

        return true;
    }

    void VulkanRenderer::DestroyRenderPass()
    {
        vkDestroyRenderPass(m_VulkanDevice, m_RenderPass, VK_NULL_HANDLE);

        LOG_DEBUG_F("VulkanRenderer RenderPass destroyed\n");
    }

    bool VulkanRenderer::CreateFrameBuffers()
    {
        m_FrameBuffers.resize(m_SwapChainImageCount);

        for (uint32_t i = 0; i < m_SwapChainImageCount; ++i)
        {
            std::array<VkImageView, 2> attachments = {};
            attachments[0] = m_DepthStencilImageView;
            attachments[1] = m_SwapChainImagesViews[i];

            VkFramebufferCreateInfo frameBufferCreateInfo = {};
            frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            frameBufferCreateInfo.renderPass = m_RenderPass;
            frameBufferCreateInfo.attachmentCount = attachments.size();
            frameBufferCreateInfo.pAttachments = attachments.data();
            frameBufferCreateInfo.width = m_SurfaceWidth;
            frameBufferCreateInfo.height = m_SurfaceHeight;
            frameBufferCreateInfo.layers = 1;

            if (vkCreateFramebuffer(m_VulkanDevice, &frameBufferCreateInfo, VK_NULL_HANDLE,
                                    &m_FrameBuffers[i]) != VK_SUCCESS)
            {
                LOG_ERROR_F("VulkanRenderer Error while creating FrameBuffers\n");

                return false;
            }
        }

        LOG_DEBUG_F("VulkanRenderer FrameBuffer created\n");

        return true;
    }

    void VulkanRenderer::DestroyFrameBuffers()
    {
        for (uint32_t i = 0; i < m_SwapChainImageCount; ++i)
        {
            vkDestroyFramebuffer(m_VulkanDevice, m_FrameBuffers[i], VK_NULL_HANDLE);
        }

        LOG_DEBUG_F("VulkanRenderer FrameBuffer destroyed\n");
    }

    bool VulkanRenderer::CreateSynchronizations()
    {
        VkFenceCreateInfo fenceCreateInfo = {};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        vkCreateFence(m_VulkanDevice, &fenceCreateInfo, VK_NULL_HANDLE, &m_SwapChainImageFence);
        LOG_DEBUG_F("VulkanRenderer Synchronizations created\n");

        return true;
    }

    void VulkanRenderer::DestroySynchronizations()
    {
        vkDestroyFence(m_VulkanDevice, m_SwapChainImageFence, VK_NULL_HANDLE);

        LOG_DEBUG_F("VulkanRenderer Synchronizations destroyed\n");
    }

    bool VulkanRenderer::Initialize()
    {
        if (!_InitInstance())
        {
            return false;
        }

        if (!_InitDevice())
        {
            return false;
        }

#if BUILD_ENABLE_VULKAN_DEBUG
        _InitDebugging();
#endif

        m_CommandPool = CreateCommandPool();
        m_CommandBuffer = CreateCommandBuffer(m_CommandPool);

        return true;
    }

    void VulkanRenderer::Destroy()
    {
        DestroyCommandPool(m_CommandPool);

        _DestroyDevice();

#if BUILD_ENABLE_VULKAN_DEBUG
        _DestroyDebugging();
#endif
        _DestroyInstance();
    }

    void VulkanRenderer::SetPlatformWindow(PlatformWindowBase *window)
    {
        m_PlatformWindow = window;
    }

    void VulkanRenderer::SetSurfaceSize(uint32_t width, uint32_t height)
    {
        m_SurfaceWidth = width;
        m_SurfaceHeight = height;
    }

    bool VulkanRenderer::_InitInstance()
    {
#if BUILD_ENABLE_VULKAN_DEBUG
        m_DebugCallbackCreateInfo = {};
        m_DebugCallbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        m_DebugCallbackCreateInfo.pfnCallback = VulkanDebugCallback;
        m_DebugCallbackCreateInfo.flags =
            VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
            VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT |
            VK_DEBUG_REPORT_DEBUG_BIT_EXT;
#endif

        for (const char *layer : g_ValidationLayers)
        {
            m_InstanceLayers.push_back(layer);
            m_DeviceLayers.push_back(layer);
        }

        m_InstanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        m_InstanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

        std::vector<const char *> requiredExtensions =
            Platform::getInstance()->GetRequiredExtensions();

        for (const char *ext : requiredExtensions)
        {
            m_InstanceExtensions.push_back(ext);
        }

        VkApplicationInfo applicationInfo = {};
        applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        applicationInfo.apiVersion = VK_API_VERSION_1_2;
        applicationInfo.engineVersion =
            VK_MAKE_VERSION(ENGINE_VERSION_MAJOR, ENGINE_VERSION_MINOR, ENGINE_VERSION_PATCH);
        applicationInfo.applicationVersion =
            VK_MAKE_VERSION(ENGINE_VERSION_MAJOR, ENGINE_VERSION_MINOR, ENGINE_VERSION_PATCH);
        applicationInfo.pEngineName = ENGINE_NAME;
        applicationInfo.pApplicationName = ENGINE_NAME;

        VkInstanceCreateInfo instanceCreateInfo = {};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pApplicationInfo = &applicationInfo;
        instanceCreateInfo.enabledLayerCount = m_InstanceLayers.size();
        instanceCreateInfo.ppEnabledLayerNames = m_InstanceLayers.data();
        instanceCreateInfo.enabledExtensionCount = m_InstanceExtensions.size();
        instanceCreateInfo.ppEnabledExtensionNames = m_InstanceExtensions.data();
#if BUILD_ENABLE_VULKAN_DEBUG
        instanceCreateInfo.pNext = &m_DebugCallbackCreateInfo;
#endif

        VkResult result = vkCreateInstance(&instanceCreateInfo, VK_NULL_HANDLE, &m_VulkanInstance);

        if (result != VK_SUCCESS)
        {
            LOG_ERROR_F("vkCreateInstance failed!\n");

            return false;
        }

        LOG_DEBUG_F("vkCreateInstance succeeded\n");

        return true;
    }

    void VulkanRenderer::_DestroyInstance()
    {
        if (m_VulkanInstance)
        {
            vkDestroyInstance(m_VulkanInstance, VK_NULL_HANDLE);
            m_VulkanInstance = VK_NULL_HANDLE;

            LOG_DEBUG_F("vkDestroyInstance destroyed\n");
        }
    }

    bool VulkanRenderer::_InitDevice()
    {
        m_DeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        uint32_t physicalDevicesCount = 0;
        vkEnumeratePhysicalDevices(m_VulkanInstance, &physicalDevicesCount, VK_NULL_HANDLE);
        std::vector<VkPhysicalDevice> devices(physicalDevicesCount);
        vkEnumeratePhysicalDevices(m_VulkanInstance, &physicalDevicesCount, devices.data());

        LOG_DEBUG("Number of Physical Devices found: " + String(physicalDevicesCount) + "\n");

        if (physicalDevicesCount < 1)
        {
            LOG_ERROR("Can't find any Physical Device!\n");
            return false;
        }

        //  TODO: find correct GPU!
        m_VulkanPhysicalDevice = devices[0];

        VkPhysicalDeviceProperties physicalDeviceProperties = {};
        vkGetPhysicalDeviceProperties(m_VulkanPhysicalDevice, &physicalDeviceProperties);
        vkGetPhysicalDeviceMemoryProperties(m_VulkanPhysicalDevice,
                                            &m_PhysicalDeviceMemoryProperties);

        LOG_DEBUG(String("Physical Device name: ") + physicalDeviceProperties.deviceName + "\n");

        String deviceType;

        switch (physicalDeviceProperties.deviceType)
        {
            case VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                deviceType = "Discrete GPU";
                break;

            case VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                deviceType = "Integrated GPU";
                break;
        }

        LOG_DEBUG(String("Physical Device type: ") + deviceType + "\n");

        uint32_t queueFamilyProperyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_VulkanPhysicalDevice, &queueFamilyProperyCount,
                                                 VK_NULL_HANDLE);
        std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyProperyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_VulkanPhysicalDevice, &queueFamilyProperyCount,
                                                 queueFamilyProperties.data());

        bool foundGraphicsBit = false;
        for (uint32_t i = 0; i < queueFamilyProperyCount; ++i)
        {
            if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                foundGraphicsBit = true;
                m_GraphicsFamilyIndex = i;
                break;
            }
        }

        if (!foundGraphicsBit)
        {
            LOG_ERROR_F("Can not find queue family supporting graphics!\n");

            return false;
        }

        uint32_t instanceLayersCount = 0;
        vkEnumerateInstanceLayerProperties(&instanceLayersCount, VK_NULL_HANDLE);
        std::vector<VkLayerProperties> instanceLayerProperties(instanceLayersCount);
        vkEnumerateInstanceLayerProperties(&instanceLayersCount, instanceLayerProperties.data());

        LOG_DEBUG("Instance layers:\n");
        for (const VkLayerProperties &layer : instanceLayerProperties)
        {
            LOG_DEBUG(String("\t") + layer.layerName + " -> " + layer.description + "\n");
        }

        uint32_t deviceLayersCount = 0;
        vkEnumerateDeviceLayerProperties(m_VulkanPhysicalDevice, &deviceLayersCount,
                                         VK_NULL_HANDLE);
        std::vector<VkLayerProperties> deviceLayerProperties(deviceLayersCount);
        vkEnumerateDeviceLayerProperties(m_VulkanPhysicalDevice, &deviceLayersCount,
                                         deviceLayerProperties.data());

        LOG_DEBUG("Device layers:\n");
        for (const VkLayerProperties &layer : deviceLayerProperties)
        {
            LOG_DEBUG(String("\t") + layer.layerName + " -> " + layer.description + "\n");
        }

        float queuePriorities[] = {1.0f};
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.queueFamilyIndex = m_GraphicsFamilyIndex;
        queueCreateInfo.pQueuePriorities = queuePriorities;

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
        deviceCreateInfo.enabledLayerCount = m_DeviceLayers.size();
        deviceCreateInfo.ppEnabledLayerNames = m_DeviceLayers.data();
        deviceCreateInfo.enabledExtensionCount = m_DeviceExtensions.size();
        deviceCreateInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();

        VkResult result = vkCreateDevice(m_VulkanPhysicalDevice, &deviceCreateInfo, VK_NULL_HANDLE,
                                         &m_VulkanDevice);

        if (result != VK_SUCCESS)
        {
            LOG_ERROR_F("vkCreateDevice failed!\n");

            return false;
        }

        vkGetDeviceQueue(m_VulkanDevice, m_GraphicsFamilyIndex, 0, &m_Queue);

        VkSemaphoreCreateInfo semaphoreCreateInfo = {};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        vkCreateSemaphore(m_VulkanDevice, &semaphoreCreateInfo, VK_NULL_HANDLE,
                          &m_RenderCompleteSemaphore);

        LOG_DEBUG_F("vkCreateDevice succeeded\n");

        return true;
    }

    void VulkanRenderer::_DestroyDevice()
    {
        if (m_RenderCompleteSemaphore)
        {
            vkDestroySemaphore(m_VulkanDevice, m_RenderCompleteSemaphore, VK_NULL_HANDLE);
        }

        if (m_VulkanDevice)
        {
            vkDestroyDevice(m_VulkanDevice, VK_NULL_HANDLE);
            m_VulkanDevice = VK_NULL_HANDLE;

            LOG_DEBUG_F("m_VulkanDevice destroyed\n");
        }
    }

    bool VulkanRenderer::_InitDebugging()
    {
#if BUILD_ENABLE_VULKAN_DEBUG
        createDebugReportCallbackEXTFunc =
            (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
                m_VulkanInstance, "vkCreateDebugReportCallbackEXT");
        destroDebugReportCallbackEXTFunc =
            (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(
                m_VulkanInstance, "vkDestroyDebugReportCallbackEXT");

        if (createDebugReportCallbackEXTFunc == VK_NULL_HANDLE ||
            destroDebugReportCallbackEXTFunc == VK_NULL_HANDLE)
        {
            LOG_ERROR_F("Can not acquire 'vkCreateDebugReportCallbackEXT' or "
                        "'vkDestroyDebugReportCallbackEXT' functions!\n");

            return false;
        }

        createDebugReportCallbackEXTFunc(m_VulkanInstance, &m_DebugCallbackCreateInfo,
                                         VK_NULL_HANDLE, &m_DebugReport);

        LOG_DEBUG_F("Vulkan debugging enabled\n");
#endif

        return true;
    }

    bool VulkanRenderer::_DestroyDebugging()
    {
#if BUILD_ENABLE_VULKAN_DEBUG
        destroDebugReportCallbackEXTFunc(m_VulkanInstance, m_DebugReport, VK_NULL_HANDLE);
        m_DebugReport = VK_NULL_HANDLE;

        LOG_DEBUG_F("Vulkan debugging destroyed\n");
#endif

        return true;
    }

    VkCommandPool VulkanRenderer::CreateCommandPool()
    {
        VkCommandPoolCreateInfo poolCreateInfo = {};
        poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolCreateInfo.queueFamilyIndex = m_GraphicsFamilyIndex;
        poolCreateInfo.flags =
            VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VkCommandPool commandPool;
        vkCreateCommandPool(m_VulkanDevice, &poolCreateInfo, VK_NULL_HANDLE, &commandPool);

        return commandPool;
    }

    VkCommandBuffer VulkanRenderer::CreateCommandBuffer(VkCommandPool pool)
    {
        VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = pool;
        commandBufferAllocateInfo.commandBufferCount = 1;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(m_VulkanDevice, &commandBufferAllocateInfo, &commandBuffer);

        return commandBuffer;
    }

    void VulkanRenderer::DestroyCommandPool(VkCommandPool pool)
    {
        vkFreeCommandBuffers(m_VulkanDevice, pool, 1, &m_CommandBuffer);
        vkDestroyCommandPool(m_VulkanDevice, pool, VK_NULL_HANDLE);
    }

    const VkInstance VulkanRenderer::GetVulkanInstance()
    {
        return m_VulkanInstance;
    }

    const VkDevice VulkanRenderer::GetVulkanDevice()
    {
        return m_VulkanDevice;
    }

    const VkPhysicalDevice VulkanRenderer::GetPhysicalDevice()
    {
        return m_VulkanPhysicalDevice;
    }

    const VkPhysicalDeviceMemoryProperties &
    VulkanRenderer::GetVulkanPhysicalDeviceMemoryProperties() const
    {
        return m_PhysicalDeviceMemoryProperties;
    }

    const VkQueue &VulkanRenderer::GetVulkanQueue() const
    {
        return m_Queue;
    }

    const VkSemaphore VulkanRenderer::GetRenderCompleteSemaphore() const
    {
        return m_RenderCompleteSemaphore;
    }

    uint32_t
    VulkanRenderer::FindMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties *memoryProperties,
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