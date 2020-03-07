// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#include "VulkanRenderer.h"
#include "core/BuildConfig.h"
#include "core/Common.h"
#include "core/Logger.h"
#include "core/Typedefs.h"
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
        m_DebugReport(VK_NULL_HANDLE)
    {
    }

    VulkanRenderer::~VulkanRenderer()
    {
    }

    bool VulkanRenderer::RenderFrame()
    {
        if (m_PlatformWindow)
        {
            return m_PlatformWindow->Update();
        }
        
        return false;
    }

    bool VulkanRenderer::Initialize(PlatformWindow *window)
    {
        m_PlatformWindow = window;

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

        return true;
    }

    void VulkanRenderer::Destroy()
    {
        _DestroyDevice();

#if BUILD_ENABLE_VULKAN_DEBUG
        _DestroyDebugging();
#endif
        _DestroyInstance();
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

        LOG_DEBUG_F("vkCreateDevice succeeded\n");

        return true;
    }

    void VulkanRenderer::_DestroyDevice()
    {
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

    void VulkanRenderer::CreateCommandPool()
    {
        VkCommandPoolCreateInfo poolCreateInfo = {};
        poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolCreateInfo.queueFamilyIndex = m_GraphicsFamilyIndex;
        poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VkCommandPool commandPool;
        vkCreateCommandPool(m_VulkanDevice, &poolCreateInfo, VK_NULL_HANDLE, &commandPool);

        VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = commandPool;
        commandBufferAllocateInfo.commandBufferCount = 1;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(m_VulkanDevice, &commandBufferAllocateInfo, &commandBuffer);

        VkCommandBufferBeginInfo commandBufferBeginInfo = {};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
        {
            VkViewport viewport = {};
            viewport.x = 0.f;
            viewport.y = 0.f;
            viewport.width = 512.f;
            viewport.height = 512.f;
            viewport.minDepth = 0.f;
            viewport.maxDepth = 1.f;

            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        }
        vkEndCommandBuffer(commandBuffer);

        VkSemaphoreCreateInfo semaphoreCreateInfo = {};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkSemaphore semaphore;
        vkCreateSemaphore(m_VulkanDevice, &semaphoreCreateInfo, VK_NULL_HANDLE, &semaphore);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        // submitInfo.waitSemaphoreCount = 1;
        // submitInfo.pWaitSemaphores = &semaphore;
        // submitInfo.pWaitDstStageMask = 0;

        vkQueueSubmit(m_Queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_Queue);

        vkDestroyCommandPool(m_VulkanDevice, commandPool, VK_NULL_HANDLE);
        vkDestroySemaphore(m_VulkanDevice, semaphore, VK_NULL_HANDLE);
    }

}  // namespace aga