// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#include "MainLoop.h"
#include "core/Macros.h"
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
    MainLoop::MainLoop() : m_Renderer(nullptr), m_PlatformWindow(nullptr)
    {
    }

    MainLoop::~MainLoop()
    {
    }

    bool MainLoop::InitializeWindow(const char *title, size_t width, size_t height)
    {
#if defined(_WIN32)
        m_PlatformWindow = new WindowsPlatformWindow();
#elif defined(__linux)
        m_PlatformWindow = new X11PlatformWindow();
#endif

        return m_PlatformWindow->Initialize(title, width, height);
    }

    void MainLoop::DestroyWindow()
    {
        m_PlatformWindow->Destroy();
        SAFE_DELETE(m_PlatformWindow);
    }

    bool MainLoop::InitializeRenderer()
    {
        m_Renderer = new VulkanRenderer();
        m_Renderer->Initialize(m_PlatformWindow);

        return true;
    }

    void MainLoop::DestroyRenderer()
    {
        m_Renderer->Destroy();
        SAFE_DELETE(m_Renderer);
    }

    bool MainLoop::Iterate() const
    {
        return m_Renderer->RenderFrame();
    }
}  // namespace aga