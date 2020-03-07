// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#include "X11PlatformWindow.h"
#include "core/Logger.h"
#include "platform/Platform.h"

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
        Destroy();
    }

    bool X11PlatformWindow::Initialize(const char *title, uint32_t width, uint32_t height)
    {
        m_Name = title;
        m_Width = width;
        m_Height = height;

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
        value_list[1] = XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE;

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

        // xcb_generic_event_t *e;
        // while ((e = xcb_wait_for_event(m_XCBConnection)))
        // {
        //     if ((e->response_type & ~0x80) == XCB_EXPOSE)
        //         break;
        // }

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

}  // namespace aga