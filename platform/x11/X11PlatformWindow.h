// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#pragma once

#include "platform/Platform.h"
#include "platform/PlatformWindow.h"

#define VK_USE_PLATFORM_XCB_KHR 1
#include <xcb/xcb.h>

namespace aga
{
    class X11PlatformWindow : public PlatformWindowBase
    {
    public:
        X11PlatformWindow();
        ~X11PlatformWindow();

        bool Initialize(const char *title, uint32_t width = 1024, uint32_t height = 768) override;
        void Destroy() override;

        bool Update() override;

        bool CreateVulkanSurface() override;
        void DestroyVulkanSurface() override;

        void _HandleEvent(const xcb_generic_event_t *event);

    private:
        xcb_connection_t *m_XCBConnection;
        xcb_screen_t *m_XCBScreen;
        xcb_window_t m_XCBWindow;
        xcb_intern_atom_reply_t *m_XCBAtomWindowReply;
    };
}  // namespace aga
