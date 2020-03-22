// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#include "VulkanRenderer.h"
#include "core/BuildConfig.h"
#include "core/Common.h"
#include "core/Logger.h"
#include "core/Typedefs.h"
#include "core/math/Vector2.h"
#include "core/math/Vector3.h"
#include "platform/Platform.h"
#include "platform/PlatformFileSystem.h"
#include "platform/PlatformWindow.h"

const int MAX_FRAMES_IN_PROCESS = 2;

namespace aga
{
    const std::vector<const char *> g_ValidationLayers = {"VK_LAYER_KHRONOS_validation"};

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

    struct Vertex
    {
        Vector2 Position;
        Vector3 Color;

        static VkVertexInputBindingDescription getBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription = {};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
        {
            std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, Position);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, Color);

            return attributeDescriptions;
        }
    };

    const std::vector<Vertex> vertices = {
        {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},  // 1
        {{0.4f, 0.0f}, {0.0f, 1.0f, 0.0f}},   // 2
        {{-0.4f, 0.0f}, {0.0f, 0.0f, 1.0f}},  // 3
        //---
        {{0.4f, 0.0f}, {0.0f, 1.0f, 0.0f}},  // 4
        {{0.0f, 0.5f}, {1.0f, 0.0f, 0.0f}},  // 5
        {{-0.4f, 0.0f}, {0.0f, 0.0f, 1.0f}}  // 6
    };

    VulkanRenderer::VulkanRenderer() :
        m_PlatformWindow(nullptr),
        m_VulkanInstance(VK_NULL_HANDLE),
        m_VulkanDevice(VK_NULL_HANDLE),
        m_VulkanPhysicalDevice(VK_NULL_HANDLE),
        m_GraphicsFamilyIndex(-1),
        m_PresentFamilyIndex(-1),
        m_GraphicsQueue(VK_NULL_HANDLE),
        m_DebugReport(VK_NULL_HANDLE),
        m_CommandPool(VK_NULL_HANDLE),
        m_VulkanSurface(VK_NULL_HANDLE),
        m_PresentMode(VK_PRESENT_MODE_MAX_ENUM_KHR),
        m_SwapChain(VK_NULL_HANDLE),
        m_SwapChainImageCount(2),
        m_ActiveSwapChainImageID(0),
        m_PipelineLayout(VK_NULL_HANDLE),
        m_GraphicsPipeline(VK_NULL_HANDLE),
        m_RenderPass(VK_NULL_HANDLE),
        m_CurrentFrame(0),
        m_FramebufferResized(false),
        m_VertexBuffer(VK_NULL_HANDLE),
        m_VertexBufferMemory(VK_NULL_HANDLE),
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
        CheckResult(
            vkWaitForFences(m_VulkanDevice, 1, &m_SyncFences[m_CurrentFrame], VK_TRUE, UINT64_MAX),
            "Wait For Fences error\n");

        VkResult result = vkAcquireNextImageKHR(m_VulkanDevice, m_SwapChain, UINT64_MAX,
                                                m_ImageAvailableSemaphores[m_CurrentFrame],
                                                VK_NULL_HANDLE, &m_ActiveSwapChainImageID);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            RecreateSwapChain();
            return true;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            LOG_ERROR_F("Failed to acquire swap chain image!");

            return false;
        }

        if (m_ImagesInProcess[m_ActiveSwapChainImageID] != VK_NULL_HANDLE)
        {
            vkWaitForFences(m_VulkanDevice, 1, &m_ImagesInProcess[m_ActiveSwapChainImageID],
                            VK_TRUE, UINT64_MAX);
        }

        m_ImagesInProcess[m_ActiveSwapChainImageID] = m_SyncFences[m_CurrentFrame];

        return true;
    }

    bool VulkanRenderer::EndRender()
    {
        VkSemaphore signalSemaphores[] = {m_RenderFinishedSemaphores[m_CurrentFrame]};

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_SwapChain;
        presentInfo.pImageIndices = &m_ActiveSwapChainImageID;

        VkResult result = vkQueuePresentKHR(m_GraphicsQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
            m_FramebufferResized)
        {
            m_FramebufferResized = false;

            RecreateSwapChain();
        }
        else if (result != VK_SUCCESS)
        {
            LOG_ERROR_F("Failed to present swap chain image!");

            return false;
        }

        m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_PROCESS;

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

    Rect2D VulkanRenderer::GetSurfaceSize()
    {
        return {0.0f, 0.0f, (float)m_SurfaceWidth, (float)m_SurfaceHeight};
    }

    bool VulkanRenderer::RenderFrame()
    {
        VkSemaphore waitSemaphores[] = {m_ImageAvailableSemaphores[m_CurrentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSemaphore signalSemaphores[] = {m_RenderFinishedSemaphores[m_CurrentFrame]};

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_CommandBuffers[m_ActiveSwapChainImageID];
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        CheckResult(vkResetFences(m_VulkanDevice, 1, &m_SyncFences[m_CurrentFrame]),
                    "Reset Fences error\n");

        CheckResult(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_SyncFences[m_CurrentFrame]),
                    "Failed to submit draw command buffer!");

        return true;
    }

    bool VulkanRenderer::CreateSwapChain()
    {
        SwapChainSupportDetails swapChainSupport = FindSwapChainDetails(m_VulkanPhysicalDevice);

        m_SurfaceFormat = _ChooseSwapSurfaceFormat(swapChainSupport.Formats);
        m_PresentMode = _ChooseSwapPresentMode(swapChainSupport.PresentModes);

        Rect2D extent = _ChooseSwapExtent(swapChainSupport.SurfaceCapabilities);
        m_SurfaceWidth = extent.Size.Width;
        m_SurfaceHeight = extent.Size.Height;

        m_SwapChainImageCount = swapChainSupport.SurfaceCapabilities.minImageCount + 1;

        if (swapChainSupport.SurfaceCapabilities.maxImageCount > 0 &&
            m_SwapChainImageCount > swapChainSupport.SurfaceCapabilities.maxImageCount)
        {
            m_SwapChainImageCount = swapChainSupport.SurfaceCapabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
        swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapChainCreateInfo.surface = m_VulkanSurface;
        swapChainCreateInfo.minImageCount = m_SwapChainImageCount;
        swapChainCreateInfo.imageFormat = m_SurfaceFormat.format;
        swapChainCreateInfo.imageColorSpace = m_SurfaceFormat.colorSpace;
        swapChainCreateInfo.imageExtent.width = extent.Size.Width;
        swapChainCreateInfo.imageExtent.height = extent.Size.Height;
        swapChainCreateInfo.imageArrayLayers = 1;
        swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapChainCreateInfo.preTransform = swapChainSupport.SurfaceCapabilities.currentTransform;
        swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapChainCreateInfo.presentMode = m_PresentMode;
        swapChainCreateInfo.clipped = VK_TRUE;
        swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

        if (m_GraphicsFamilyIndex != m_PresentFamilyIndex)
        {
            uint32_t queueFamilyIndices[] = {m_GraphicsFamilyIndex, m_PresentFamilyIndex};

            swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapChainCreateInfo.queueFamilyIndexCount = 2;
            swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            swapChainCreateInfo.queueFamilyIndexCount = 0;      // Optional
            swapChainCreateInfo.pQueueFamilyIndices = nullptr;  // Optional
        }

        CheckResult(vkCreateSwapchainKHR(m_VulkanDevice, &swapChainCreateInfo, VK_NULL_HANDLE,
                                         &m_SwapChain),
                    "VulkanRenderer CreateSwapChain failed\n");

        CheckResult(vkGetSwapchainImagesKHR(m_VulkanDevice, m_SwapChain, &m_SwapChainImageCount,
                                            VK_NULL_HANDLE),
                    "VulkanRenderer GetSwapchainImages failed\n");

        m_SwapChainImages.resize(m_SwapChainImageCount);

        CheckResult(vkGetSwapchainImagesKHR(m_VulkanDevice, m_SwapChain, &m_SwapChainImageCount,
                                            m_SwapChainImages.data()),
                    "VulkanRenderer GetSwapchainImages failed\n");

        LOG_DEBUG_F("VulkanRenderer Vulkan SwapChain created\n");

        return true;
    }

    bool VulkanRenderer::CreateSwapChainImages()
    {
        m_SwapChainImagesViews.resize(m_SwapChainImageCount);

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

            CheckResult(vkCreateImageView(m_VulkanDevice, &imageViewCreateInfo, nullptr,
                                          &m_SwapChainImagesViews[i]),
                        "VulkanRenderer CreateSwapChainImages failed\n");
        }

        LOG_DEBUG_F("VulkanRenderer Vulkan SwapChain Images created\n");

        return true;
    }

    void VulkanRenderer::DestroySwapChainImages()
    {
        for (int i = 0; i < m_SwapChainImagesViews.size(); ++i)
        {
            vkDestroyImageView(m_VulkanDevice, m_SwapChainImagesViews[i], VK_NULL_HANDLE);
            m_SwapChainImagesViews[i] = VK_NULL_HANDLE;
        }

        m_SwapChainImagesViews.clear();

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

    bool VulkanRenderer::CreateGraphicsPipeline()
    {
        String vertShaderCode = PlatformFileSystem::getInstance()->ReadEntireFileBinaryMode(
            "data/shaders/shader_base.vert.spv");
        String fragShaderCode = PlatformFileSystem::getInstance()->ReadEntireFileBinaryMode(
            "data/shaders/shader_base.frag.spv");

        VkShaderModule vertShaderModule = _CreateShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = _CreateShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount =
            static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)m_SurfaceWidth;
        viewport.height = (float)m_SurfaceHeight;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {};
        scissor.offset = {0, 0};
        scissor.extent.width = GetSurfaceSize().Size.Width;
        scissor.extent.height = GetSurfaceSize().Size.Height;

        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer = {};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling = {};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
        // depthStencilStateCreateInfo.sType =
        //     VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

        VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending = {};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pushConstantRangeCount = 0;

        CheckResult(vkCreatePipelineLayout(m_VulkanDevice, &pipelineLayoutInfo, VK_NULL_HANDLE,
                                           &m_PipelineLayout),
                    "Failed to create pipeline layout!");

        VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.stageCount = 2;
        pipelineCreateInfo.pStages = shaderStages;
        pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
        pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
        pipelineCreateInfo.pViewportState = &viewportState;
        pipelineCreateInfo.pRasterizationState = &rasterizer;
        pipelineCreateInfo.pMultisampleState = &multisampling;
        // pipelineCreateInfo.pDepthStencilState = VK_NULL_HANDLE;
        pipelineCreateInfo.pColorBlendState = &colorBlending;
        pipelineCreateInfo.layout = m_PipelineLayout;
        pipelineCreateInfo.renderPass = m_RenderPass;
        pipelineCreateInfo.subpass = 0;
        pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

        CheckResult(vkCreateGraphicsPipelines(m_VulkanDevice, VK_NULL_HANDLE, 1,
                                              &pipelineCreateInfo, nullptr, &m_GraphicsPipeline),
                    "Failed to create graphics pipeline!");

        vkDestroyShaderModule(m_VulkanDevice, fragShaderModule, VK_NULL_HANDLE);
        vkDestroyShaderModule(m_VulkanDevice, vertShaderModule, VK_NULL_HANDLE);

        LOG_DEBUG_F("VulkanRenderer Graphics Pipeline created\n");

        return true;
    }

    void VulkanRenderer::DestroyGraphicsPipeline()
    {
        vkDestroyPipeline(m_VulkanDevice, m_GraphicsPipeline, VK_NULL_HANDLE);
        vkDestroyPipelineLayout(m_VulkanDevice, m_PipelineLayout, VK_NULL_HANDLE);

        LOG_DEBUG_F("VulkanRenderer Graphics Pipeline destoryed\n");
    }

    VkShaderModule VulkanRenderer::_CreateShaderModule(const String &shaderCodeData)
    {
        VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
        shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCreateInfo.codeSize = shaderCodeData.Length();
        shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t *>(shaderCodeData.GetData());

        VkShaderModule shaderModule;
        CheckResult(vkCreateShaderModule(m_VulkanDevice, &shaderModuleCreateInfo, VK_NULL_HANDLE,
                                         &shaderModule),
                    "Failed to create shader module!");

        return shaderModule;
    }

    bool VulkanRenderer::CreateRenderPass()
    {
        std::array<VkAttachmentDescription, 1> attachments = {};
        // attachments[0].flags = 0;
        // attachments[0].format = m_DepthStencilFormat;
        // attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        // attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        // attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        // attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        // attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        // attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        // attachments[0].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        attachments[0].format = m_SurfaceFormat.format;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // VkAttachmentReference subPass0DepthStencilAttachment = {};
        // subPass0DepthStencilAttachment.attachment = 0;
        // subPass0DepthStencilAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        std::array<VkAttachmentReference, 1> subPass0ColorAttachments = {};
        subPass0ColorAttachments[0].attachment = 0;
        subPass0ColorAttachments[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        std::array<VkSubpassDescription, 1> subPasses = {};
        subPasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subPasses[0].colorAttachmentCount = subPass0ColorAttachments.size();
        subPasses[0].pColorAttachments = subPass0ColorAttachments.data();
        //  subPasses[0].pDepthStencilAttachment = &subPass0DepthStencilAttachment;

        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassCreateInfo = {};
        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.attachmentCount = attachments.size();
        renderPassCreateInfo.pAttachments = attachments.data();
        renderPassCreateInfo.subpassCount = subPasses.size();
        renderPassCreateInfo.pSubpasses = subPasses.data();
        renderPassCreateInfo.dependencyCount = 1;
        renderPassCreateInfo.pDependencies = &dependency;

        CheckResult(vkCreateRenderPass(m_VulkanDevice, &renderPassCreateInfo, VK_NULL_HANDLE,
                                       &m_RenderPass),
                    "VulkanRenderer Error while creating RenderPass\n");

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
        m_FrameBuffers.resize(m_SwapChainImagesViews.size());

        for (uint32_t i = 0; i < m_SwapChainImagesViews.size(); ++i)
        {
            std::array<VkImageView, 1> attachments = {};
            //      attachments[0] = m_DepthStencilImageView;
            attachments[0] = m_SwapChainImagesViews[i];

            VkFramebufferCreateInfo frameBufferCreateInfo = {};
            frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            frameBufferCreateInfo.renderPass = m_RenderPass;
            frameBufferCreateInfo.attachmentCount = attachments.size();
            frameBufferCreateInfo.pAttachments = attachments.data();
            frameBufferCreateInfo.width = m_SurfaceWidth;
            frameBufferCreateInfo.height = m_SurfaceHeight;
            frameBufferCreateInfo.layers = 1;

            CheckResult(vkCreateFramebuffer(m_VulkanDevice, &frameBufferCreateInfo, VK_NULL_HANDLE,
                                            &m_FrameBuffers[i]),
                        "VulkanRenderer Error while creating FrameBuffers\n");
        }

        LOG_DEBUG_F("VulkanRenderer FrameBuffer created\n");

        return true;
    }

    void VulkanRenderer::DestroyFrameBuffers()
    {
        for (VkFramebuffer &frameBuffer : m_FrameBuffers)
        {
            vkDestroyFramebuffer(m_VulkanDevice, frameBuffer, VK_NULL_HANDLE);
        }

        LOG_DEBUG_F("VulkanRenderer FrameBuffer destroyed\n");
    }

    bool VulkanRenderer::CreateSynchronizations()
    {
        m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_PROCESS);
        m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_PROCESS);
        m_SyncFences.resize(MAX_FRAMES_IN_PROCESS);
        m_ImagesInProcess.resize(m_SwapChainImages.size(), VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_PROCESS; ++i)
        {
            CheckResult(vkCreateSemaphore(m_VulkanDevice, &semaphoreInfo, VK_NULL_HANDLE,
                                          &m_ImageAvailableSemaphores[i]),
                        "Error while creating ImageAvailable semaphore");

            CheckResult(vkCreateSemaphore(m_VulkanDevice, &semaphoreInfo, VK_NULL_HANDLE,
                                          &m_RenderFinishedSemaphores[i]),
                        "Error while creating RenderFinished semaphore");

            CheckResult(vkCreateFence(m_VulkanDevice, &fenceInfo, VK_NULL_HANDLE, &m_SyncFences[i]),
                        "Error while creating Sync fence");
        }

        LOG_DEBUG_F("VulkanRenderer Synchronizations created\n");

        return true;
    }

    void VulkanRenderer::DestroySynchronizations()
    {
        for (size_t i = 0; i < MAX_FRAMES_IN_PROCESS; ++i)
        {
            vkDestroySemaphore(m_VulkanDevice, m_RenderFinishedSemaphores[i], VK_NULL_HANDLE);
            vkDestroySemaphore(m_VulkanDevice, m_ImageAvailableSemaphores[i], VK_NULL_HANDLE);
            vkDestroyFence(m_VulkanDevice, m_SyncFences[i], VK_NULL_HANDLE);
        }

        LOG_DEBUG_F("VulkanRenderer Synchronizations destroyed\n");
    }

    bool VulkanRenderer::Initialize()
    {
        _PrepareExtensions();

        if (!_InitInstance())
        {
            return false;
        }

#if BUILD_ENABLE_VULKAN_DEBUG
        _InitDebugging();
#endif

        if (!_InitPhysicalDevice())
        {
            return false;
        }

        if (!_InitLogicalDevice())
        {
            return false;
        }

        if (!CreateSwapChain())
        {
            return false;
        }

        if (!CreateSwapChainImages())
        {
            return false;
        }

        // if (!CreateDepthStencilImage())
        // {
        //     return false;
        // }

        if (!CreateRenderPass())
        {
            return false;
        }

        if (!CreateGraphicsPipeline())
        {
            return false;
        }

        if (!CreateFrameBuffers())
        {
            return false;
        }

        if (!CreateCommandPool())
        {
            return false;
        }

        if (!CreateVertexBuffer())
        {
            return false;
        }

        if (!CreateCommandBuffers())
        {
            return false;
        }

        if (!CreateSynchronizations())
        {
            return false;
        }

        return true;
    }

    void VulkanRenderer::Destroy()
    {
        DestroySwapChain();
        DestroyVertexBuffer();
        DestroySynchronizations();
        DestroyCommandPool();

        // DestroyDepthStencilImage();

        _DestroyLogicalDevice();

#if BUILD_ENABLE_VULKAN_DEBUG
        _DestroyDebugging();
#endif

        _DestroyInstance();
    }

    void VulkanRenderer::SetPlatformWindow(PlatformWindowBase *window)
    {
        m_PlatformWindow = window;
    }

    void VulkanRenderer::_PrepareExtensions()
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

        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        LOG_INFO("Available extensions:\n");

        for (const VkExtensionProperties &extension : extensions)
        {
            LOG_INFO(String("\t") + extension.extensionName + "\n");
        }

        m_InstanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        m_InstanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

        std::vector<const char *> requiredExtensions =
            Platform::getInstance()->GetRequiredExtensions();

        for (const char *ext : requiredExtensions)
        {
            m_InstanceExtensions.push_back(ext);
        }

        // Prepare validation layers as well
        for (const char *layer : g_ValidationLayers)
        {
            m_InstanceLayers.push_back(layer);
            m_DeviceLayers.push_back(layer);
        }
    }

    bool VulkanRenderer::_InitInstance()
    {
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

        CheckResult(vkCreateInstance(&instanceCreateInfo, VK_NULL_HANDLE, &m_VulkanInstance),
                    "vkCreateInstance failed!\n");

        LOG_DEBUG_F("vkCreateInstance succeeded\n");

        m_VulkanSurface = m_PlatformWindow->CreateVulkanSurface();

        return true;
    }

    void VulkanRenderer::_DestroyInstance()
    {
        if (m_VulkanSurface)
        {
            vkDestroySurfaceKHR(m_VulkanInstance, m_VulkanSurface, VK_NULL_HANDLE);
            m_VulkanSurface = VK_NULL_HANDLE;

            LOG_DEBUG_F("VulkanRenderer Vulkan Surface destroyed\n");
        }

        if (m_VulkanInstance)
        {
            vkDestroyInstance(m_VulkanInstance, VK_NULL_HANDLE);
            m_VulkanInstance = VK_NULL_HANDLE;

            LOG_DEBUG_F("vkDestroyInstance destroyed\n");
        }
    }

    bool VulkanRenderer::_InitPhysicalDevice()
    {
        m_DeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        uint32_t physicalDevicesCount = 0;
        vkEnumeratePhysicalDevices(m_VulkanInstance, &physicalDevicesCount, VK_NULL_HANDLE);
        std::vector<VkPhysicalDevice> devices(physicalDevicesCount);
        vkEnumeratePhysicalDevices(m_VulkanInstance, &physicalDevicesCount, devices.data());

        LOG_DEBUG("Number of Physical Devices found: " + String(physicalDevicesCount) + "\n");

        if (physicalDevicesCount < 1)
        {
            LOG_ERROR_F("Can't find any Physical Device!\n");
            return false;
        }

        for (const VkPhysicalDevice &device : devices)
        {
            if (_IsPhysicalDeviceSuitable(device))
            {
                m_VulkanPhysicalDevice = device;
                break;
            }
        }

        if (m_VulkanPhysicalDevice == VK_NULL_HANDLE)
        {
            LOG_ERROR_F("failed to find a suitable GPU!");
            return false;
        }

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

        uint32_t instanceLayersCount = 0;
        CheckResult(vkEnumerateInstanceLayerProperties(&instanceLayersCount, VK_NULL_HANDLE), "");
        std::vector<VkLayerProperties> instanceLayerProperties(instanceLayersCount);
        CheckResult(vkEnumerateInstanceLayerProperties(&instanceLayersCount,
                                                       instanceLayerProperties.data()),
                    "");

        LOG_DEBUG("Instance layers:\n");
        for (const VkLayerProperties &layer : instanceLayerProperties)
        {
            LOG_DEBUG(String("\t") + layer.layerName + " -> " + layer.description + "\n");
        }

        uint32_t deviceLayersCount = 0;
        CheckResult(vkEnumerateDeviceLayerProperties(m_VulkanPhysicalDevice, &deviceLayersCount,
                                                     VK_NULL_HANDLE),
                    "");
        std::vector<VkLayerProperties> deviceLayerProperties(deviceLayersCount);
        CheckResult(vkEnumerateDeviceLayerProperties(m_VulkanPhysicalDevice, &deviceLayersCount,
                                                     deviceLayerProperties.data()),
                    "");

        LOG_DEBUG("Device layers:\n");
        for (const VkLayerProperties &layer : deviceLayerProperties)
        {
            LOG_DEBUG(String("\t") + layer.layerName + " -> " + layer.description + "\n");
        }

        LOG_DEBUG_F("Create Physical Device succeeded\n");

        return true;
    }

    bool VulkanRenderer::_InitLogicalDevice()
    {
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {m_GraphicsFamilyIndex, m_PresentFamilyIndex};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures = {};

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.enabledLayerCount = m_DeviceLayers.size();
        deviceCreateInfo.ppEnabledLayerNames = m_DeviceLayers.data();
        deviceCreateInfo.enabledExtensionCount = m_DeviceExtensions.size();
        deviceCreateInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();
        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

        CheckResult(vkCreateDevice(m_VulkanPhysicalDevice, &deviceCreateInfo, VK_NULL_HANDLE,
                                   &m_VulkanDevice),
                    "Create Logical Device failed!\n");

        vkGetDeviceQueue(m_VulkanDevice, m_GraphicsFamilyIndex, 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_VulkanDevice, m_PresentFamilyIndex, 0, &m_PresentQueue);

        LOG_DEBUG_F("Create Logical Device succeeded\n");

        return true;
    }

    QueueFamilyIndices VulkanRenderer::FindQueueFamilies(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices;

        uint32_t queueFamilyProperyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyProperyCount, VK_NULL_HANDLE);
        std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyProperyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyProperyCount,
                                                 queueFamilyProperties.data());

        for (uint32_t i = 0; i < queueFamilyProperyCount; ++i)
        {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_VulkanSurface, &presentSupport);

            if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.GraphicsIndex = i;
                indices.valid_bit |= QueueFamilyIndices::GRAPHICS_BIT;
            }

            if (presentSupport)
            {
                indices.PresentIndex = i;
                indices.valid_bit |= QueueFamilyIndices::PRESENT_BIT;
            }

            if (indices.IsValid())
            {
                break;
            }
        }

        return indices;
    }

    bool VulkanRenderer::_IsPhysicalDeviceSuitable(VkPhysicalDevice device)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        QueueFamilyIndices indices = FindQueueFamilies(device);

        if (indices.IsValid())
        {
            m_GraphicsFamilyIndex = indices.GraphicsIndex;
            m_PresentFamilyIndex = indices.PresentIndex;
        }
        else
        {
            LOG_WARNING_F("Can not find queue family supporting graphics for device: " +
                          deviceProperties.deviceName + "!\n");
        }

        // Check if selected physical device supports all required extensions
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, VK_NULL_HANDLE, &extensionCount,
                                             VK_NULL_HANDLE);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, VK_NULL_HANDLE, &extensionCount,
                                             availableExtensions.data());

        std::set<std::string> requiredExtensions(m_DeviceExtensions.begin(),
                                                 m_DeviceExtensions.end());

        for (const VkExtensionProperties &extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }

        bool swapChainAdequate = false;
        if (requiredExtensions.empty())
        {
            SwapChainSupportDetails swapChainSupport = FindSwapChainDetails(device);
            swapChainAdequate =
                !swapChainSupport.Formats.empty() && !swapChainSupport.PresentModes.empty();
        }

        return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
               indices.IsValid() && requiredExtensions.empty() && swapChainAdequate;
    }

    SwapChainSupportDetails VulkanRenderer::FindSwapChainDetails(VkPhysicalDevice device)
    {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_VulkanSurface,
                                                  &details.SurfaceCapabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_VulkanSurface, &formatCount, nullptr);

        if (formatCount != 0)
        {
            details.Formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_VulkanSurface, &formatCount,
                                                 details.Formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_VulkanSurface, &presentModeCount,
                                                  nullptr);

        if (presentModeCount != 0)
        {
            details.PresentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_VulkanSurface, &presentModeCount,
                                                      details.PresentModes.data());
        }

        return details;
    }

    VkSurfaceFormatKHR VulkanRenderer::_ChooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR> &availableFormats)
    {
        for (const VkSurfaceFormatKHR &availableFormat : availableFormats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR VulkanRenderer::_ChooseSwapPresentMode(
        const std::vector<VkPresentModeKHR> &availablePresentModes)
    {
        for (const VkPresentModeKHR &availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    Rect2D VulkanRenderer::_ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
    {
        if (capabilities.currentExtent.width != UINT32_MAX)
        {
            return Rect2D{0.0f, 0.0f, (float)capabilities.currentExtent.width,
                          (float)capabilities.currentExtent.height};
        }
        else
        {
            Vector2 actualExtent = m_PlatformWindow->GetCurrentWindowSize();

            actualExtent.Width =
                std::max((float)capabilities.minImageExtent.width,
                         std::min((float)capabilities.maxImageExtent.width, actualExtent.Width));
            actualExtent.Height =
                std::max((float)capabilities.minImageExtent.height,
                         std::min((float)capabilities.maxImageExtent.height, actualExtent.Height));

            return Rect2D{0.0f, 0.0f, actualExtent.Width, actualExtent.Height};
        }
    }

    void VulkanRenderer::_DestroyLogicalDevice()
    {
        if (m_VulkanDevice)
        {
            vkDestroyDevice(m_VulkanDevice, VK_NULL_HANDLE);
            m_VulkanDevice = VK_NULL_HANDLE;

            LOG_DEBUG_F("Vulkan Logical Device destroyed\n");
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

        CheckResult(createDebugReportCallbackEXTFunc(m_VulkanInstance, &m_DebugCallbackCreateInfo,
                                                     VK_NULL_HANDLE, &m_DebugReport),
                    "Can't create Vulkan debug report callback");

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

    bool VulkanRenderer::CreateCommandPool()
    {
        VkCommandPoolCreateInfo poolCreateInfo = {};
        poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolCreateInfo.queueFamilyIndex = m_GraphicsFamilyIndex;

        CheckResult(
            vkCreateCommandPool(m_VulkanDevice, &poolCreateInfo, VK_NULL_HANDLE, &m_CommandPool),
            "Error while creating Command Pool");

        return true;
    }

    uint32_t VulkanRenderer::_FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_VulkanPhysicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        CheckResult(VK_ERROR_UNKNOWN, "Failed to find suitable memory type!");

        return -1;
    }

    bool VulkanRenderer::CreateVertexBuffer()
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = sizeof(vertices[0]) * vertices.size();
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        CheckResult(vkCreateBuffer(m_VulkanDevice, &bufferInfo, nullptr, &m_VertexBuffer),
                    "Failed to create vertex buffer!");

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_VulkanDevice, m_VertexBuffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = _FindMemoryType(memRequirements.memoryTypeBits,
                                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        CheckResult(vkAllocateMemory(m_VulkanDevice, &allocInfo, nullptr, &m_VertexBufferMemory),
                    "Failed to allocate vertex buffer memory!");

        CheckResult(vkBindBufferMemory(m_VulkanDevice, m_VertexBuffer, m_VertexBufferMemory, 0),
                    "Can't bind memory for Vertex Buffer");

        void *data;
        vkMapMemory(m_VulkanDevice, m_VertexBufferMemory, 0, bufferInfo.size, 0, &data);
        memcpy(data, vertices.data(), (size_t)bufferInfo.size);
        vkUnmapMemory(m_VulkanDevice, m_VertexBufferMemory);

        LOG_DEBUG_F("Vulkan Vertex Buffer created\n");

        return true;
    }

    void VulkanRenderer::DestroyVertexBuffer()
    {
        vkDestroyBuffer(m_VulkanDevice, m_VertexBuffer, nullptr);
        vkFreeMemory(m_VulkanDevice, m_VertexBufferMemory, nullptr);

        LOG_DEBUG_F("Vulkan Vertex Buffer destroyed\n");
    }

    bool VulkanRenderer::CreateCommandBuffers()
    {
        m_CommandBuffers.resize(m_FrameBuffers.size());

        VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = m_CommandPool;
        commandBufferAllocateInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        CheckResult(vkAllocateCommandBuffers(m_VulkanDevice, &commandBufferAllocateInfo,
                                             m_CommandBuffers.data()),
                    "Error while creating Command Buffers");

        for (size_t i = 0; i < m_CommandBuffers.size(); ++i)
        {
            VkCommandBufferBeginInfo commandBufferBeginInfo = {};
            commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            CheckResult(vkBeginCommandBuffer(m_CommandBuffers[i], &commandBufferBeginInfo),
                        "Error while running vkBeginCommandBuffer");
            {
                std::array<VkClearValue, 1> clearValues = {};
                //   clearValues[0].depthStencil.depth = 0.0f;
                //    clearValues[0].depthStencil.stencil = 0;

                clearValues[0].color.float32[0] = 0.0f;
                clearValues[0].color.float32[1] = 0.0f;
                clearValues[0].color.float32[2] = 1.0f;
                clearValues[0].color.float32[3] = 0.0f;

                VkRenderPassBeginInfo renderPassBeginInfo = {};
                renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassBeginInfo.renderPass = m_RenderPass;
                renderPassBeginInfo.framebuffer = m_FrameBuffers[i];
                renderPassBeginInfo.renderArea.offset = {0, 0};
                renderPassBeginInfo.renderArea.extent.width = GetSurfaceSize().Size.Width;
                renderPassBeginInfo.renderArea.extent.height = GetSurfaceSize().Size.Height;
                renderPassBeginInfo.clearValueCount = clearValues.size();
                renderPassBeginInfo.pClearValues = clearValues.data();

                vkCmdBeginRenderPass(m_CommandBuffers[i], &renderPassBeginInfo,
                                     VK_SUBPASS_CONTENTS_INLINE);

                vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  m_GraphicsPipeline);

                VkBuffer vertexBuffers[] = {m_VertexBuffer};
                VkDeviceSize offsets[] = {0};
                vkCmdBindVertexBuffers(m_CommandBuffers[i], 0, 1, vertexBuffers, offsets);

                vkCmdDraw(m_CommandBuffers[i], static_cast<uint32_t>(vertices.size()), 1, 0, 0);

                vkCmdEndRenderPass(m_CommandBuffers[i]);
            }
            CheckResult(vkEndCommandBuffer(m_CommandBuffers[i]),
                        "Error while running vkEndCommandBuffer");
        }

        return true;
    }

    void VulkanRenderer::DestroyCommandPool()
    {
        vkDestroyCommandPool(m_VulkanDevice, m_CommandPool, VK_NULL_HANDLE);

        LOG_DEBUG_F("Vulkan Command Pool destroyed\n");
    }

    void VulkanRenderer::DestroySwapChain()
    {
        DestroyFrameBuffers();

        vkFreeCommandBuffers(m_VulkanDevice, m_CommandPool,
                             static_cast<uint32_t>(m_CommandBuffers.size()),
                             m_CommandBuffers.data());

        DestroyGraphicsPipeline();
        DestroyRenderPass();
        DestroySwapChainImages();

        vkDestroySwapchainKHR(m_VulkanDevice, m_SwapChain, VK_NULL_HANDLE);

        LOG_DEBUG_F("VulkanRenderer Vulkan SwapChain destroyed\n");
    }

    void VulkanRenderer::RecreateSwapChain()
    {
        Vector2 winSize = m_PlatformWindow->GetCurrentWindowSize();

        while (winSize.Width == 0 || winSize.Height == 0)
        {
            winSize = m_PlatformWindow->GetCurrentWindowSize();
            m_PlatformWindow->Update();
        }

        vkDeviceWaitIdle(m_VulkanDevice);

        DestroySwapChain();

        CreateSwapChain();
        CreateSwapChainImages();
        CreateRenderPass();
        CreateGraphicsPipeline();
        CreateFrameBuffers();
        CreateCommandBuffers();
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
        return m_GraphicsQueue;
    }

    void VulkanRenderer::CheckResult(VkResult result, const String &message)
    {
        if (result != VK_SUCCESS)
        {
            LOG_ERROR_F(message);

            std::exit(-1);
        }
    }

    void VulkanRenderer::SetFrameBufferResized(bool resized)
    {
        m_FramebufferResized = resized;
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