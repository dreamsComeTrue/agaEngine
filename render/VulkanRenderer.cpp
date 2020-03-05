// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#include "VulkanRenderer.h"
#include "core/Logger.h"
#include "core/Typedefs.h"

#include <vector>

namespace aga
{
    VulkanRenderer::VulkanRenderer() :
        m_VulkanInstance(nullptr),
        m_VulkanDevice(nullptr),
        m_VulkanPhysicalDevice(nullptr)
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
        if (!_InitInstance())
        {
            return;
        }

        if (!_InitDevice())
        {
            return;
        }
    }

    void VulkanRenderer::_Destroy()
    {
        _DestroyDevice();
        _DestroyInstance();
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
        applicationInfo.pApplicationName = ENGINE_NAME;

        VkInstanceCreateInfo instanceCreateInfo = {};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pApplicationInfo = &applicationInfo;

        VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &m_VulkanInstance);

        if (result != VK_SUCCESS)
        {
            LOG_ERROR_F("vkCreateInstance failed!\n");

            return false;
        }

        LOG_DEBUG_F("vkCreateInstance succeeded!\n");

        return true;
    }

    void VulkanRenderer::_DestroyInstance()
    {
        if (m_VulkanInstance)
        {
            vkDestroyInstance(m_VulkanInstance, nullptr);
            m_VulkanInstance = nullptr;

            LOG_DEBUG_F("vkDestroyInstance succeeded!\n");
        }
    }

    bool VulkanRenderer::_InitDevice()
    {
        uint32_t physicalDevicesCount = 0;
        vkEnumeratePhysicalDevices(m_VulkanInstance, &physicalDevicesCount, nullptr);
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
                                                 nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyProperyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_VulkanPhysicalDevice, &queueFamilyProperyCount,
                                                 queueFamilyProperties.data());

        bool foundGraphicsBit = false;
        uint32_t graphicsFamilyIndex = -1;
        for (uint32_t i = 0; i < queueFamilyProperyCount; ++i)
        {
            if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                foundGraphicsBit = true;
                graphicsFamilyIndex = i;
                break;
            }
        }

        if (!foundGraphicsBit)
        {
            LOG_ERROR_F("Can not find queue family supporting graphics!\n");

            return false;
        }

        uint32_t instanceLayersCount = 0;
        vkEnumerateInstanceLayerProperties(&instanceLayersCount, nullptr);
        std::vector<VkLayerProperties> instanceLayerProperties(instanceLayersCount);
        vkEnumerateInstanceLayerProperties(&instanceLayersCount, instanceLayerProperties.data());

        LOG_DEBUG("Instance layers:\n");
        for (const VkLayerProperties &layer : instanceLayerProperties)
        {
            LOG_DEBUG(String("\t") + layer.layerName + " -> " + layer.description + "\n");
        }

        uint32_t deviceLayersCount = 0;
        vkEnumerateDeviceLayerProperties(m_VulkanPhysicalDevice, &deviceLayersCount, nullptr);
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
        queueCreateInfo.queueFamilyIndex = graphicsFamilyIndex;
        queueCreateInfo.pQueuePriorities = queuePriorities;

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;

        VkResult result =
            vkCreateDevice(m_VulkanPhysicalDevice, &deviceCreateInfo, nullptr, &m_VulkanDevice);

        if (result != VK_SUCCESS)
        {
            LOG_ERROR_F("vkCreateDevice failed!\n");

            return false;
        }

        LOG_DEBUG_F("vkCreateDevice succeeded!\n");

        return true;
    }

    void VulkanRenderer::_DestroyDevice()
    {
        if (m_VulkanDevice)
        {
            vkDestroyDevice(m_VulkanDevice, nullptr);
            m_VulkanDevice = nullptr;

            LOG_DEBUG_F("m_VulkanDevice succeeded!\n");
        }
    }

}  // namespace aga