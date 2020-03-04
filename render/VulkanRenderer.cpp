// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#include "VulkanRenderer.h"
#include "core/Logger.h"

namespace aga
{
    VulkanRenderer::VulkanRenderer() : m_VulkanInstance(nullptr)
    {
        _Initialize();
    }

    VulkanRenderer::~VulkanRenderer()
    {
        _Destroy();
    }

    void VulkanRenderer::RenderFrame()
    {
    }

    void VulkanRenderer::_Initialize()
    {
        _InitInstance();
    }

    void VulkanRenderer::_Destroy()
    {
        if (m_VulkanInstance)
        {
            vkDestroyInstance(m_VulkanInstance, nullptr);
            m_VulkanInstance = nullptr;

            LOG_DEBUG_F("vkDestroyInstance succeeded!\n");
        }
    }

    bool VulkanRenderer::_InitInstance()
    {
        VkInstanceCreateInfo instanceCreateInfo = {};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

        VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &m_VulkanInstance);

        if (result != VK_SUCCESS)
        {
            LOG_ERROR_F("vkCreateInstance failed!\n");

            return false;
        }

        LOG_DEBUG_F("vkCreateInstance succeeded!\n");

        return true;
    }

}  // namespace aga