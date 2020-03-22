// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#include "VulkanRenderer.h"
#include "core/BuildConfig.h"
#include "core/Common.h"
#include "core/Logger.h"
#include "core/Typedefs.h"
#include "core/math/Matrix.h"
#include "core/math/Vector2.h"
#include "core/math/Vector3.h"
#include "platform/Platform.h"
#include "platform/PlatformFileSystem.h"
#include "platform/PlatformWindow.h"

#include <chrono>

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb/stb_image.h"

const int MAX_FRAMES_IN_PROCESS = 2;

namespace aga
{
    const std::vector<const char *> g_ValidationLayers = {"VK_LAYER_KHRONOS_validation"};

    VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugReportFlagsEXT flags,
                                                       VkDebugReportObjectTypeEXT objectType, uint64_t sourceObject,
                                                       size_t location, int32_t code, const char *layerPrefix,
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
        Vector3 Position;
        Vector3 Color;
        Vector2 TexCoord;

        static VkVertexInputBindingDescription getBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription = {};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
        {
            std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, Position);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, Color);

            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(Vertex, TexCoord);

            return attributeDescriptions;
        }
    };

    struct UniformBufferObject
    {
        Matrix Model;
        Matrix View;
        Matrix Projection;
    };

    const std::vector<Vertex> vertices = {{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
                                          {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
                                          {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                                          {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

                                          {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
                                          {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
                                          {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                                          {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}};

    const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4};

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
        m_DescriptorSetLayout(VK_NULL_HANDLE),
        m_DescriptorPool(VK_NULL_HANDLE),
        m_PipelineLayout(VK_NULL_HANDLE),
        m_GraphicsPipeline(VK_NULL_HANDLE),
        m_RenderPass(VK_NULL_HANDLE),
        m_CurrentFrame(0),
        m_FramebufferResized(false),
        m_VertexBuffer(VK_NULL_HANDLE),
        m_VertexBufferMemory(VK_NULL_HANDLE),
        m_IndexBuffer(VK_NULL_HANDLE),
        m_IndexBufferMemory(VK_NULL_HANDLE),
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
        CheckResult(vkWaitForFences(m_VulkanDevice, 1, &m_SyncFences[m_CurrentFrame], VK_TRUE, UINT64_MAX),
                    "Wait For Fences error\n");

        VkResult result =
            vkAcquireNextImageKHR(m_VulkanDevice, m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame],
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

        _UpdateUniformBuffer();

        if (m_ImagesInProcess[m_ActiveSwapChainImageID] != VK_NULL_HANDLE)
        {
            vkWaitForFences(m_VulkanDevice, 1, &m_ImagesInProcess[m_ActiveSwapChainImageID], VK_TRUE, UINT64_MAX);
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

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResized)
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

        CheckResult(vkResetFences(m_VulkanDevice, 1, &m_SyncFences[m_CurrentFrame]), "Reset Fences error\n");

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

        CheckResult(vkCreateSwapchainKHR(m_VulkanDevice, &swapChainCreateInfo, VK_NULL_HANDLE, &m_SwapChain),
                    "VulkanRenderer CreateSwapChain failed\n");

        CheckResult(vkGetSwapchainImagesKHR(m_VulkanDevice, m_SwapChain, &m_SwapChainImageCount, VK_NULL_HANDLE),
                    "VulkanRenderer GetSwapchainImages failed\n");

        m_SwapChainImages.resize(m_SwapChainImageCount);

        CheckResult(
            vkGetSwapchainImagesKHR(m_VulkanDevice, m_SwapChain, &m_SwapChainImageCount, m_SwapChainImages.data()),
            "VulkanRenderer GetSwapchainImages failed\n");

        LOG_DEBUG_F("VulkanRenderer Vulkan SwapChain created\n");

        return true;
    }

    bool VulkanRenderer::CreateSwapChainImages()
    {
        m_SwapChainImagesViews.resize(m_SwapChainImageCount);

        for (uint32_t i = 0; i < m_SwapChainImageCount; ++i)
        {
            m_SwapChainImagesViews[i] =
                _CreateImageView(m_SwapChainImages[i], m_SurfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
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

    VkFormat VulkanRenderer::_FindDepthFormat()
    {
        return _FindSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                                    VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    bool VulkanRenderer::CreateDepthStencilImage()
    {
        m_DepthStencilFormat = _FindDepthFormat();

        if (m_DepthStencilFormat == VK_FORMAT_UNDEFINED)
        {
            LOG_ERROR_F("VulkanRenderer DepthStencil format not selected\n");

            return false;
        }

        if (m_DepthStencilFormat == VK_FORMAT_D32_SFLOAT_S8_UINT ||
            m_DepthStencilFormat == VK_FORMAT_D24_UNORM_S8_UINT ||
            m_DepthStencilFormat == VK_FORMAT_D16_UNORM_S8_UINT || m_DepthStencilFormat == VK_FORMAT_S8_UINT)
        {
            m_IsStencilAvailable = true;
        }

        _CreateImage(m_SurfaceWidth, m_SurfaceHeight, m_DepthStencilFormat, VK_IMAGE_TILING_OPTIMAL,
                     VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     m_DepthStencilImage, m_DepthStencilImageMemory);

        m_DepthStencilImageView =
            _CreateImageView(m_DepthStencilImage, m_DepthStencilFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

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

    VkFormat VulkanRenderer::_FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                                                  VkFormatFeatureFlags features)
    {
        for (VkFormat format : candidates)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(m_VulkanPhysicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            {
                return format;
            }
        }

        LOG_ERROR_F("Failed to find supported format!");

        std::exit(-1);

        return VK_FORMAT_UNDEFINED;
    }

    bool VulkanRenderer::CreateGraphicsPipeline()
    {
        String vertShaderCode =
            PlatformFileSystem::getInstance()->ReadEntireFileBinaryMode("data/shaders/shader_base.vert.spv");
        String fragShaderCode =
            PlatformFileSystem::getInstance()->ReadEntireFileBinaryMode("data/shaders/shader_base.frag.spv");

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
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
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
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling = {};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencil = {};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
        colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
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
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0;

        CheckResult(vkCreatePipelineLayout(m_VulkanDevice, &pipelineLayoutInfo, VK_NULL_HANDLE, &m_PipelineLayout),
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
        pipelineCreateInfo.pDepthStencilState = &depthStencil;
        pipelineCreateInfo.pColorBlendState = &colorBlending;
        pipelineCreateInfo.layout = m_PipelineLayout;
        pipelineCreateInfo.renderPass = m_RenderPass;
        pipelineCreateInfo.subpass = 0;
        pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

        CheckResult(vkCreateGraphicsPipelines(m_VulkanDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr,
                                              &m_GraphicsPipeline),
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
        CheckResult(vkCreateShaderModule(m_VulkanDevice, &shaderModuleCreateInfo, VK_NULL_HANDLE, &shaderModule),
                    "Failed to create shader module!\n");

        return shaderModule;
    }

    bool VulkanRenderer::CreateRenderPass()
    {
        std::array<VkAttachmentDescription, 2> attachments = {};
        attachments[0].format = m_SurfaceFormat.format;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        attachments[1].format = _FindDepthFormat();
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef = {};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassCreateInfo = {};
        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassCreateInfo.pAttachments = attachments.data();
        renderPassCreateInfo.subpassCount = 1;
        renderPassCreateInfo.pSubpasses = &subpass;
        renderPassCreateInfo.dependencyCount = 1;
        renderPassCreateInfo.pDependencies = &dependency;

        CheckResult(vkCreateRenderPass(m_VulkanDevice, &renderPassCreateInfo, VK_NULL_HANDLE, &m_RenderPass),
                    "VulkanRenderer Error while creating RenderPass\n");

        LOG_DEBUG_F("VulkanRenderer RenderPass created\n");

        return true;
    }

    void VulkanRenderer::DestroyRenderPass()
    {
        vkDestroyRenderPass(m_VulkanDevice, m_RenderPass, VK_NULL_HANDLE);

        LOG_DEBUG_F("VulkanRenderer RenderPass destroyed\n");
    }

    bool VulkanRenderer::CreateDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding = {};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = VK_NULL_HANDLE;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        CheckResult(vkCreateDescriptorSetLayout(m_VulkanDevice, &layoutInfo, VK_NULL_HANDLE, &m_DescriptorSetLayout),
                    "Failed to create descriptor set layout!\n");

        return true;
    }

    void VulkanRenderer::DestroyDescriptorSetLayout()
    {
        vkDestroyDescriptorSetLayout(m_VulkanDevice, m_DescriptorSetLayout, VK_NULL_HANDLE);

        LOG_DEBUG_F("VulkanRenderer DescriptorSetLayout destroyed\n");
    }

    bool VulkanRenderer::CreateFrameBuffers()
    {
        m_FrameBuffers.resize(m_SwapChainImagesViews.size());

        for (uint32_t i = 0; i < m_SwapChainImagesViews.size(); ++i)
        {
            std::array<VkImageView, 2> attachments = {m_SwapChainImagesViews[i], m_DepthStencilImageView};

            VkFramebufferCreateInfo frameBufferCreateInfo = {};
            frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            frameBufferCreateInfo.renderPass = m_RenderPass;
            frameBufferCreateInfo.attachmentCount = attachments.size();
            frameBufferCreateInfo.pAttachments = attachments.data();
            frameBufferCreateInfo.width = m_SurfaceWidth;
            frameBufferCreateInfo.height = m_SurfaceHeight;
            frameBufferCreateInfo.layers = 1;

            CheckResult(vkCreateFramebuffer(m_VulkanDevice, &frameBufferCreateInfo, VK_NULL_HANDLE, &m_FrameBuffers[i]),
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
            CheckResult(
                vkCreateSemaphore(m_VulkanDevice, &semaphoreInfo, VK_NULL_HANDLE, &m_ImageAvailableSemaphores[i]),
                "Error while creating ImageAvailable semaphore");

            CheckResult(
                vkCreateSemaphore(m_VulkanDevice, &semaphoreInfo, VK_NULL_HANDLE, &m_RenderFinishedSemaphores[i]),
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

        if (!CreateRenderPass())
        {
            return false;
        }

        if (!CreateDescriptorSetLayout())
        {
            return false;
        }

        if (!CreateGraphicsPipeline())
        {
            return false;
        }

        if (!CreateCommandPool())
        {
            return false;
        }

        if (!CreateDepthStencilImage())
        {
            return false;
        }

        if (!CreateFrameBuffers())
        {
            return false;
        }

        if (!CreateTextureImage())
        {
            return false;
        }

        if (!CreateTextureImageView())
        {
            return false;
        }

        if (!CreateTextureSampler())
        {
            return false;
        }

        if (!CreateVertexBuffer())
        {
            return false;
        }

        if (!CreateIndexBuffer())
        {
            return false;
        }

        if (!CreateUniformBuffers())
        {
            return false;
        }

        if (!CreateDescriptorPool())
        {
            return false;
        }

        if (!CreateDescriptorSets())
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
        DestroyTextureSampler();
        DestroyTextureImageView();
        DestroyTextureImage();
        DestroyDescriptorSetLayout();
        DestroyIndexBuffer();
        DestroyVertexBuffer();
        DestroySynchronizations();
        DestroyCommandPool();
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
        m_DebugCallbackCreateInfo.flags = VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
                                          VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT |
                                          VK_DEBUG_REPORT_DEBUG_BIT_EXT;
#endif

        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        LOG_DEBUG("Available extensions:\n");

        for (const VkExtensionProperties &extension : extensions)
        {
            LOG_DEBUG(String("\t") + extension.extensionName + "\n");
        }

        m_InstanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        m_InstanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

        std::vector<const char *> requiredExtensions = Platform::getInstance()->GetRequiredExtensions();

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
        vkGetPhysicalDeviceMemoryProperties(m_VulkanPhysicalDevice, &m_PhysicalDeviceMemoryProperties);

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
        CheckResult(vkEnumerateInstanceLayerProperties(&instanceLayersCount, instanceLayerProperties.data()), "");

        LOG_DEBUG("Instance layers:\n");
        for (const VkLayerProperties &layer : instanceLayerProperties)
        {
            LOG_DEBUG(String("\t") + layer.layerName + " -> " + layer.description + "\n");
        }

        uint32_t deviceLayersCount = 0;
        CheckResult(vkEnumerateDeviceLayerProperties(m_VulkanPhysicalDevice, &deviceLayersCount, VK_NULL_HANDLE), "");
        std::vector<VkLayerProperties> deviceLayerProperties(deviceLayersCount);
        CheckResult(
            vkEnumerateDeviceLayerProperties(m_VulkanPhysicalDevice, &deviceLayersCount, deviceLayerProperties.data()),
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
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.enabledLayerCount = m_DeviceLayers.size();
        deviceCreateInfo.ppEnabledLayerNames = m_DeviceLayers.data();
        deviceCreateInfo.enabledExtensionCount = m_DeviceExtensions.size();
        deviceCreateInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();
        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

        CheckResult(vkCreateDevice(m_VulkanPhysicalDevice, &deviceCreateInfo, VK_NULL_HANDLE, &m_VulkanDevice),
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
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyProperyCount, queueFamilyProperties.data());

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
            LOG_WARNING_F("Can not find queue family supporting graphics for device: " + deviceProperties.deviceName +
                          "!\n");
        }

        // Check if selected physical device supports all required extensions
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, VK_NULL_HANDLE, &extensionCount, VK_NULL_HANDLE);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, VK_NULL_HANDLE, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());

        for (const VkExtensionProperties &extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }

        bool swapChainAdequate = false;
        if (requiredExtensions.empty())
        {
            SwapChainSupportDetails swapChainSupport = FindSwapChainDetails(device);
            swapChainAdequate = !swapChainSupport.Formats.empty() && !swapChainSupport.PresentModes.empty();
        }

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

        return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && indices.IsValid() &&
               requiredExtensions.empty() && swapChainAdequate && supportedFeatures.samplerAnisotropy;
    }

    SwapChainSupportDetails VulkanRenderer::FindSwapChainDetails(VkPhysicalDevice device)
    {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_VulkanSurface, &details.SurfaceCapabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_VulkanSurface, &formatCount, nullptr);

        if (formatCount != 0)
        {
            details.Formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_VulkanSurface, &formatCount, details.Formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_VulkanSurface, &presentModeCount, nullptr);

        if (presentModeCount != 0)
        {
            details.PresentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_VulkanSurface, &presentModeCount,
                                                      details.PresentModes.data());
        }

        return details;
    }

    VkSurfaceFormatKHR VulkanRenderer::_ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
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

    VkPresentModeKHR VulkanRenderer::_ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
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

            actualExtent.Width = std::max((float)capabilities.minImageExtent.width,
                                          std::min((float)capabilities.maxImageExtent.width, actualExtent.Width));
            actualExtent.Height = std::max((float)capabilities.minImageExtent.height,
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
        createDebugReportCallbackEXTFunc = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
            m_VulkanInstance, "vkCreateDebugReportCallbackEXT");
        destroDebugReportCallbackEXTFunc = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(
            m_VulkanInstance, "vkDestroyDebugReportCallbackEXT");

        if (createDebugReportCallbackEXTFunc == VK_NULL_HANDLE || destroDebugReportCallbackEXTFunc == VK_NULL_HANDLE)
        {
            LOG_ERROR_F("Can not acquire 'vkCreateDebugReportCallbackEXT' or "
                        "'vkDestroyDebugReportCallbackEXT' functions!\n");

            return false;
        }

        CheckResult(createDebugReportCallbackEXTFunc(m_VulkanInstance, &m_DebugCallbackCreateInfo, VK_NULL_HANDLE,
                                                     &m_DebugReport),
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

        CheckResult(vkCreateCommandPool(m_VulkanDevice, &poolCreateInfo, VK_NULL_HANDLE, &m_CommandPool),
                    "Error while creating Command Pool");

        return true;
    }

    uint32_t VulkanRenderer::_FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_VulkanPhysicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        CheckResult(VK_ERROR_UNKNOWN, "Failed to find suitable memory type!");

        return -1;
    }

    bool VulkanRenderer::CreateVertexBuffer()
    {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        _CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
                      stagingBufferMemory);

        void *data;
        vkMapMemory(m_VulkanDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t)bufferSize);
        vkUnmapMemory(m_VulkanDevice, stagingBufferMemory);

        _CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_IndexBuffer, m_IndexBufferMemory);

        _CopyBuffer(stagingBuffer, m_IndexBuffer, bufferSize);

        vkDestroyBuffer(m_VulkanDevice, stagingBuffer, nullptr);
        vkFreeMemory(m_VulkanDevice, stagingBufferMemory, nullptr);

        LOG_DEBUG_F("Vulkan Vertex Buffer created\n");

        return true;
    }

    void VulkanRenderer::DestroyVertexBuffer()
    {
        vkDestroyBuffer(m_VulkanDevice, m_VertexBuffer, nullptr);
        vkFreeMemory(m_VulkanDevice, m_VertexBufferMemory, nullptr);

        LOG_DEBUG_F("Vulkan Vertex Buffer destroyed\n");
    }

    bool VulkanRenderer::CreateIndexBuffer()
    {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        _CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
                      stagingBufferMemory);

        void *data;
        vkMapMemory(m_VulkanDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(m_VulkanDevice, stagingBufferMemory);

        _CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VertexBuffer, m_VertexBufferMemory);

        _CopyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);

        vkDestroyBuffer(m_VulkanDevice, stagingBuffer, nullptr);
        vkFreeMemory(m_VulkanDevice, stagingBufferMemory, nullptr);

        LOG_DEBUG_F("Vulkan Vertex Buffer created\n");

        return true;
    }

    void VulkanRenderer::DestroyIndexBuffer()
    {
        vkDestroyBuffer(m_VulkanDevice, m_IndexBuffer, nullptr);
        vkFreeMemory(m_VulkanDevice, m_IndexBufferMemory, nullptr);

        LOG_DEBUG_F("Vulkan Index Buffer destroyed\n");
    }

    bool VulkanRenderer::CreateUniformBuffers()
    {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        m_UniformBuffers.resize(m_SwapChainImages.size());
        m_UniformBuffersMemory.resize(m_SwapChainImages.size());

        for (size_t i = 0; i < m_SwapChainImages.size(); i++)
        {
            _CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          m_UniformBuffers[i], m_UniformBuffersMemory[i]);
        }

        LOG_DEBUG_F("Vulkan Uniforms Buffers created\n");

        return true;
    }

    void VulkanRenderer::DestroyUniformBuffers()
    {
        for (size_t i = 0; i < m_SwapChainImages.size(); i++)
        {
            vkDestroyBuffer(m_VulkanDevice, m_UniformBuffers[i], nullptr);
            vkFreeMemory(m_VulkanDevice, m_UniformBuffersMemory[i], nullptr);
        }

        LOG_DEBUG_F("Vulkan Uniforms Buffers destroyed\n");
    }

    bool VulkanRenderer::CreateDescriptorPool()
    {
        std::array<VkDescriptorPoolSize, 2> poolSizes = {};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(m_SwapChainImages.size());
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(m_SwapChainImages.size());

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(m_SwapChainImages.size());

        CheckResult(vkCreateDescriptorPool(m_VulkanDevice, &poolInfo, nullptr, &m_DescriptorPool),
                    "failed to create descriptor pool!");

        LOG_DEBUG_F("Vulkan Descriptor Pool created\n");

        return true;
    }

    void VulkanRenderer::DestroyDescriptorPool()
    {
        vkDestroyDescriptorPool(m_VulkanDevice, m_DescriptorPool, nullptr);

        LOG_DEBUG_F("Vulkan Descriptor Pool destroyed\n");
    }

    bool VulkanRenderer::CreateDescriptorSets()
    {
        std::vector<VkDescriptorSetLayout> layouts(m_SwapChainImages.size(), m_DescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_DescriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(m_SwapChainImages.size());
        allocInfo.pSetLayouts = layouts.data();

        m_DescriptorSets.resize(m_SwapChainImages.size());

        CheckResult(vkAllocateDescriptorSets(m_VulkanDevice, &allocInfo, m_DescriptorSets.data()),
                    "Failed to allocate descriptor sets!");

        for (size_t i = 0; i < m_SwapChainImages.size(); i++)
        {
            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = m_UniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo imageInfo = {};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = m_TextureImageView;
            imageInfo.sampler = m_TextureSampler;

            std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = m_DescriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = m_DescriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(m_VulkanDevice, static_cast<uint32_t>(descriptorWrites.size()),
                                   descriptorWrites.data(), 0, VK_NULL_HANDLE);
        }

        LOG_DEBUG_F("Vulkan Descriptor Sets created\n");

        return true;
    }

    void VulkanRenderer::DestroyDescriptorSets()
    {
        LOG_DEBUG_F("Vulkan Descriptor Sets destroyed\n");
    }

    bool VulkanRenderer::CreateTextureImage()
    {
        int texWidth, texHeight, texChannels;
        stbi_uc *pixels = stbi_load("data/textures/logo.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels)
        {
            LOG_ERROR_F("Failed to load texture image!");

            return false;
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        _CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
                      stagingBufferMemory);

        void *data;
        vkMapMemory(m_VulkanDevice, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(m_VulkanDevice, stagingBufferMemory);

        stbi_image_free(pixels);

        _CreateImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                     VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     m_TextureImage, m_TextureImageMemory);

        _TransitionImageLayout(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        _CopyBufferToImage(stagingBuffer, m_TextureImage, static_cast<uint32_t>(texWidth),
                           static_cast<uint32_t>(texHeight));
        _TransitionImageLayout(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(m_VulkanDevice, stagingBuffer, VK_NULL_HANDLE);
        vkFreeMemory(m_VulkanDevice, stagingBufferMemory, VK_NULL_HANDLE);

        return true;
    }

    void VulkanRenderer::DestroyTextureImage()
    {
        vkDestroyImage(m_VulkanDevice, m_TextureImage, VK_NULL_HANDLE);
        vkFreeMemory(m_VulkanDevice, m_TextureImageMemory, VK_NULL_HANDLE);
    }

    bool VulkanRenderer::CreateTextureImageView()
    {
        m_TextureImageView = _CreateImageView(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

        return true;
    }

    void VulkanRenderer::DestroyTextureImageView()
    {
        vkDestroyImageView(m_VulkanDevice, m_TextureImageView, VK_NULL_HANDLE);
    }

    bool VulkanRenderer::CreateTextureSampler()
    {
        VkSamplerCreateInfo samplerInfo = {};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = 16;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        CheckResult(vkCreateSampler(m_VulkanDevice, &samplerInfo, VK_NULL_HANDLE, &m_TextureSampler),
                    "Failed to create texture sampler!");

        return true;
    }

    void VulkanRenderer::DestroyTextureSampler()
    {
        vkDestroySampler(m_VulkanDevice, m_TextureSampler, VK_NULL_HANDLE);
    }

    VkImageView VulkanRenderer::_CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
    {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        CheckResult(vkCreateImageView(m_VulkanDevice, &viewInfo, nullptr, &imageView),
                    "Failed to create texture image view!");

        return imageView;
    }

    VkCommandBuffer VulkanRenderer::_BeginSingleTimeCommands()
    {
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_CommandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        CheckResult(vkAllocateCommandBuffers(m_VulkanDevice, &allocInfo, &commandBuffer),
                    "Failed to create Command Buffer for CopyBuffer operation");

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        CheckResult(vkBeginCommandBuffer(commandBuffer, &beginInfo), "Error during CopyBuffer operation");

        return commandBuffer;
    }

    void VulkanRenderer::_EndSingleTimeCommands(VkCommandBuffer commandBuffer)
    {
        CheckResult(vkEndCommandBuffer(commandBuffer), "Error during CopyBuffer operation");

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        CheckResult(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE),
                    "Error during CopyBuffer operation");
        CheckResult(vkQueueWaitIdle(m_GraphicsQueue), "Error during CopyBuffer operation");

        vkFreeCommandBuffers(m_VulkanDevice, m_CommandPool, 1, &commandBuffer);
    }

    void VulkanRenderer::_TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout,
                                                VkImageLayout newLayout)
    {
        VkCommandBuffer commandBuffer = _BeginSingleTimeCommands();

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                 newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
        {
            LOG_ERROR_F("Unsupported layout transition!");

            std::exit(-1);
        }

        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        _EndSingleTimeCommands(commandBuffer);
    }

    void VulkanRenderer::_CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
    {
        VkCommandBuffer commandBuffer = _BeginSingleTimeCommands();

        VkBufferImageCopy region = {};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        _EndSingleTimeCommands(commandBuffer);
    }

    void VulkanRenderer::_CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                                      VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image,
                                      VkDeviceMemory &imageMemory)
    {
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        CheckResult(vkCreateImage(m_VulkanDevice, &imageInfo, nullptr, &image), "Failed to create image!");

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(m_VulkanDevice, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = _FindMemoryType(memRequirements.memoryTypeBits, properties);

        CheckResult(vkAllocateMemory(m_VulkanDevice, &allocInfo, nullptr, &imageMemory),
                    "Failed to allocate image memory!");

        vkBindImageMemory(m_VulkanDevice, image, imageMemory, 0);
    }

    void VulkanRenderer::_CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                                       VkBuffer &buffer, VkDeviceMemory &bufferMemory)
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        CheckResult(vkCreateBuffer(m_VulkanDevice, &bufferInfo, nullptr, &buffer), "Failed to create vertex buffer!");

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_VulkanDevice, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = _FindMemoryType(memRequirements.memoryTypeBits, properties);

        CheckResult(vkAllocateMemory(m_VulkanDevice, &allocInfo, nullptr, &bufferMemory),
                    "Failed to allocate buffer memory!");

        CheckResult(vkBindBufferMemory(m_VulkanDevice, buffer, bufferMemory, 0), "Can't bind memory for a Buffer");
    }

    void VulkanRenderer::_CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
    {
        VkCommandBuffer commandBuffer = _BeginSingleTimeCommands();
        {
            VkBufferCopy copyRegion = {};
            copyRegion.size = size;
            vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
        }
        _EndSingleTimeCommands(commandBuffer);
    }

    void VulkanRenderer::_UpdateUniformBuffer()
    {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo = {};
        ubo.Model = ubo.Model.SetRotationAxisRadians(time * DegToRad(30.0f), Vector3(0.0f, 0.0f, 1.0f));
        ubo.View = ubo.View.LookAt(Vector3(2.0f, 2.0f, 2.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f));
        ubo.Projection = ubo.Projection.ProjectionMatrixPerspectiveFov(
            DegToRad(45.0f), m_SurfaceWidth / (real_t)m_SurfaceHeight, 0.1f, 10.0f);
        ubo.Projection[1][1] *= -1;

        void *data;
        vkMapMemory(m_VulkanDevice, m_UniformBuffersMemory[m_ActiveSwapChainImageID], 0, sizeof(ubo), 0, &data);
        memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(m_VulkanDevice, m_UniformBuffersMemory[m_ActiveSwapChainImageID]);
    }

    bool VulkanRenderer::CreateCommandBuffers()
    {
        m_CommandBuffers.resize(m_FrameBuffers.size());

        VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = m_CommandPool;
        commandBufferAllocateInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        CheckResult(vkAllocateCommandBuffers(m_VulkanDevice, &commandBufferAllocateInfo, m_CommandBuffers.data()),
                    "Error while creating Command Buffers");

        for (size_t i = 0; i < m_CommandBuffers.size(); ++i)
        {
            VkCommandBufferBeginInfo commandBufferBeginInfo = {};
            commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            CheckResult(vkBeginCommandBuffer(m_CommandBuffers[i], &commandBufferBeginInfo),
                        "Error while running vkBeginCommandBuffer");
            {
                std::array<VkClearValue, 2> clearValues = {};
                clearValues[0].color = {0.01f, 0.01f, 0.01f, 1.0f};
                clearValues[1].depthStencil = {1.0f, 0};

                VkRenderPassBeginInfo renderPassBeginInfo = {};
                renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassBeginInfo.renderPass = m_RenderPass;
                renderPassBeginInfo.framebuffer = m_FrameBuffers[i];
                renderPassBeginInfo.renderArea.offset = {0, 0};
                renderPassBeginInfo.renderArea.extent.width = GetSurfaceSize().Size.Width;
                renderPassBeginInfo.renderArea.extent.height = GetSurfaceSize().Size.Height;
                renderPassBeginInfo.clearValueCount = clearValues.size();
                renderPassBeginInfo.pClearValues = clearValues.data();

                vkCmdBeginRenderPass(m_CommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

                vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

                VkBuffer vertexBuffers[] = {m_VertexBuffer};
                VkDeviceSize offsets[] = {0};
                vkCmdBindVertexBuffers(m_CommandBuffers[i], 0, 1, vertexBuffers, offsets);
                vkCmdBindIndexBuffer(m_CommandBuffers[i], m_IndexBuffer, 0, VK_INDEX_TYPE_UINT16);
                vkCmdBindDescriptorSets(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1,
                                        &m_DescriptorSets[i], 0, nullptr);

                vkCmdDrawIndexed(m_CommandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

                vkCmdEndRenderPass(m_CommandBuffers[i]);
            }
            CheckResult(vkEndCommandBuffer(m_CommandBuffers[i]), "Error while running vkEndCommandBuffer");
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
        DestroyDepthStencilImage();
        DestroyFrameBuffers();

        vkFreeCommandBuffers(m_VulkanDevice, m_CommandPool, static_cast<uint32_t>(m_CommandBuffers.size()),
                             m_CommandBuffers.data());

        DestroyGraphicsPipeline();
        DestroyRenderPass();
        DestroySwapChainImages();

        vkDestroySwapchainKHR(m_VulkanDevice, m_SwapChain, VK_NULL_HANDLE);

        DestroyUniformBuffers();
        DestroyDescriptorPool();

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
        CreateDepthStencilImage();
        CreateFrameBuffers();
        CreateUniformBuffers();
        CreateDescriptorPool();
        CreateDescriptorSets();
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

    const VkPhysicalDeviceMemoryProperties &VulkanRenderer::GetVulkanPhysicalDeviceMemoryProperties() const
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

    uint32_t VulkanRenderer::FindMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties *memoryProperties,
                                                 const VkMemoryRequirements *memoryRequirements,
                                                 const VkMemoryPropertyFlags requiredPropertyFlags)
    {
        for (uint32_t i = 0; i < memoryProperties->memoryTypeCount; ++i)
        {
            if (memoryRequirements->memoryTypeBits & (1 << i))
            {
                if ((memoryProperties->memoryTypes[i].propertyFlags & requiredPropertyFlags) == requiredPropertyFlags)
                {
                    return i;
                }
            }
        }

        return UINT32_MAX;
    }
}  // namespace aga