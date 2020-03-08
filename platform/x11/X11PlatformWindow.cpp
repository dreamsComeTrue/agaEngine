// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#include "X11PlatformWindow.h"
#include "core/Logger.h"
#include "platform/Platform.h"
#include "render/VulkanRenderer.h"

namespace aga
{
    X11PlatformWindow::X11PlatformWindow() :
        m_XCBConnection(nullptr),
        m_XCBScreen(nullptr),
        m_XCBWindow(0),
        m_XCBAtomWindowReply(nullptr)
    {
    }

    X11PlatformWindow::~X11PlatformWindow()
    {
    }

    bool X11PlatformWindow::Initialize(const char *title, uint32_t width, uint32_t height)
    {
        m_Name = title;
        m_Width = width;
        m_Height = height;
        m_SurfaceWidth = m_Width;
        m_SurfaceHeight = m_Height;

        if (m_Width <= 0 || m_Height <= 0)
        {
            LOG_ERROR_F("Window dimensions must be positive integer value.\n");

            return false;
        }

        /* open the connection to the X server */
        int screen = 0;

        m_XCBConnection = xcb_connect(nullptr, &screen);

        if (m_XCBConnection == nullptr)
        {
            LOG_ERROR_F("Cannot find a compatible Vulkan ICD.\n");

            return false;
        }

        /* get the first screen */
        const xcb_setup_t *setup = xcb_get_setup(m_XCBConnection);
        xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);

        while (screen-- > 0)
        {
            xcb_screen_next(&iter);
        }

        m_XCBScreen = iter.data;

        // create window
        VkRect2D dimensions = {{0, 0}, {m_Width, m_Height}};

        /* create the window */
        m_XCBWindow = xcb_generate_id(m_XCBConnection);

        uint32_t value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;

        uint32_t value_list[32];
        value_list[0] = m_XCBScreen->black_pixel;
        value_list[1] = XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_KEY_PRESS |
                        XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY |
                        XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_PRESS |
                        XCB_EVENT_MASK_BUTTON_RELEASE;

        xcb_create_window(m_XCBConnection, XCB_COPY_FROM_PARENT, m_XCBWindow, m_XCBScreen->root,
                          dimensions.offset.x, dimensions.offset.y, dimensions.extent.width,
                          dimensions.extent.height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                          m_XCBScreen->root_visual, value_mask, value_list);

        xcb_change_property(m_XCBConnection, XCB_PROP_MODE_REPLACE, m_XCBWindow, XCB_ATOM_WM_NAME,
                            XCB_ATOM_STRING, 8, strlen(title), title);
        xcb_change_property(m_XCBConnection, XCB_PROP_MODE_REPLACE, m_XCBWindow,
                            XCB_ATOM_WM_ICON_NAME, XCB_ATOM_STRING, 8, strlen(title), title);

        /* Magic code that will send notification when window is destroyed */
        xcb_intern_atom_cookie_t cookie = xcb_intern_atom(m_XCBConnection, 1, 12, "WM_PROTOCOLS");
        xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(m_XCBConnection, cookie, 0);

        xcb_intern_atom_cookie_t cookie2 =
            xcb_intern_atom(m_XCBConnection, 0, 16, "WM_DELETE_WINDOW");
        m_XCBAtomWindowReply = xcb_intern_atom_reply(m_XCBConnection, cookie2, 0);

        xcb_change_property(m_XCBConnection, XCB_PROP_MODE_REPLACE, m_XCBWindow, (*reply).atom, 4,
                            32, 1, &(*m_XCBAtomWindowReply).atom);
        free(reply);

        /* map the window on the screen */
        xcb_map_window(m_XCBConnection, m_XCBWindow);

        // Force the x/y coordinates to 100,100 results are identical in consecutive runs
        const uint32_t coords[] = {100, 100};
        xcb_configure_window(m_XCBConnection, m_XCBWindow,
                             XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords);
        xcb_flush(m_XCBConnection);

        LOG_INFO("Initialized X11PlatformWindow [" + String(width) + "x" + String(height) + "]\n");

        return true;
    }

    void X11PlatformWindow::Destroy()
    {
        xcb_destroy_window(m_XCBConnection, m_XCBWindow);
        xcb_disconnect(m_XCBConnection);
        m_XCBWindow = 0;
        m_XCBConnection = nullptr;

        LOG_INFO("X11PlatformWindow destroyed\n");
    }

    bool X11PlatformWindow::Update()
    {
        xcb_generic_event_t *event;
        while ((event = xcb_poll_for_event(m_XCBConnection)))
        {
            _HandleEvent(event);
            free(event);
        }

        return m_ShouldRun;
    }

    void X11PlatformWindow::_HandleEvent(const xcb_generic_event_t *event)
    {
        switch (event->response_type & 0x7f)
        {
            case XCB_CLIENT_MESSAGE:
                if ((*(xcb_client_message_event_t *)event).data.data32[0] ==
                    (*m_XCBAtomWindowReply).atom)
                {
                    m_ShouldRun = false;
                }
                break;
            case XCB_MOTION_NOTIFY:
            {
                xcb_motion_notify_event_t *motion = (xcb_motion_notify_event_t *)event;
                //    handleMouseMove((int32_t)motion->event_x, (int32_t)motion->event_y);
                break;
            }
            break;
            case XCB_BUTTON_PRESS:
            {
                xcb_button_press_event_t *press = (xcb_button_press_event_t *)event;
                // if (press->detail == XCB_BUTTON_INDEX_1)
                //     mouseButtons.left = true;
                // if (press->detail == XCB_BUTTON_INDEX_2)
                //     mouseButtons.middle = true;
                // if (press->detail == XCB_BUTTON_INDEX_3)
                //     mouseButtons.right = true;
            }
            break;
            case XCB_BUTTON_RELEASE:
            {
                xcb_button_press_event_t *press = (xcb_button_press_event_t *)event;
                // if (press->detail == XCB_BUTTON_INDEX_1)
                //     mouseButtons.left = false;
                // if (press->detail == XCB_BUTTON_INDEX_2)
                //     mouseButtons.middle = false;
                // if (press->detail == XCB_BUTTON_INDEX_3)
                //     mouseButtons.right = false;
            }
            break;
            case XCB_KEY_PRESS:
            {
                const xcb_key_release_event_t *keyEvent = (const xcb_key_release_event_t *)event;
                switch (keyEvent->detail)
                {
                    // case KEY_W:
                    //     camera.keys.up = true;
                    //     break;
                    // case KEY_S:
                    //     camera.keys.down = true;
                    //     break;
                    // case KEY_A:
                    //     camera.keys.left = true;
                    //     break;
                    // case KEY_D:
                    //     camera.keys.right = true;
                    //     break;
                    // case KEY_P:
                    //     paused = !paused;
                    //     break;
                    // case KEY_F1:
                    //     if (settings.overlay)
                    //     {
                    //         settings.overlay = !settings.overlay;
                    //     }
                    //     break;
                }
            }
            break;
            case XCB_KEY_RELEASE:
            {
                const xcb_key_release_event_t *keyEvent = (const xcb_key_release_event_t *)event;
                switch (keyEvent->detail)
                {
                    // case KEY_W:
                    //     camera.keys.up = false;
                    //     break;
                    case 0x9:
                        m_ShouldRun = false;
                        break;
                }
                // keyPressed(keyEvent->detail);
            }
            break;
            case XCB_DESTROY_NOTIFY:
                m_ShouldRun = false;
                break;
            case XCB_CONFIGURE_NOTIFY:
            {
                const xcb_configure_notify_event_t *cfgEvent =
                    (const xcb_configure_notify_event_t *)event;
                // if ((prepared) && ((cfgEvent->width != width) || (cfgEvent->height != height)))
                // {
                //     destWidth = cfgEvent->width;
                //     destHeight = cfgEvent->height;
                //     if ((destWidth > 0) && (destHeight > 0))
                //     {
                //         windowResize();
                //     }
                // }
            }
            break;
            default:
                break;
        }
    }

    bool X11PlatformWindow::CreateVulkanSurface()
    {
        VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {};
        surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
        surfaceCreateInfo.connection = m_XCBConnection;
        surfaceCreateInfo.window = m_XCBWindow;

        vkCreateXcbSurfaceKHR(m_Renderer->GetVulkanInstance(), &surfaceCreateInfo, VK_NULL_HANDLE,
                              &m_VulkanSurface);

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

    void X11PlatformWindow::DestroyVulkanSurface()
    {
        vkDestroySurfaceKHR(m_Renderer->GetVulkanInstance(), m_VulkanSurface, VK_NULL_HANDLE);
        m_VulkanSurface = VK_NULL_HANDLE;

        LOG_DEBUG_F("X11PlatformWindow Vulkan Surface destroyed\n");
    }

    bool X11PlatformWindow::CreateSwapChain()
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

    void X11PlatformWindow::DestroySwapChain()
    {
        vkDestroySwapchainKHR(m_Renderer->GetVulkanDevice(), m_SwapChain, VK_NULL_HANDLE);

        LOG_DEBUG_F("X11PlatformWindow Vulkan SwapChain destroyed\n");
    }

    bool X11PlatformWindow::CreateSwapChainImages()
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

    void X11PlatformWindow::DestroySwapChainImages()
    {
        for (uint32_t i = 0; i < m_SwapChainImagesViews.size(); ++i)
        {
            vkDestroyImageView(m_Renderer->GetVulkanDevice(), m_SwapChainImagesViews[i],
                               VK_NULL_HANDLE);
        }

        LOG_DEBUG_F("X11PlatformWindow Vulkan SwapChain Images destroyed\n");
    }

}  // namespace aga