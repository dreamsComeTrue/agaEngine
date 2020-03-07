// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#include "MainLoop.h"
#include "core/Macros.h"
#include "platform/PlatformWindow.h"
#include "render/VulkanRenderer.h"

#include <chrono>
#include <thread>

namespace aga
{
    MainLoop::MainLoop() : m_Renderer(nullptr), m_PlatformWindowBase(nullptr)
    {
    }

    MainLoop::~MainLoop()
    {
    }

    bool MainLoop::InitializeRenderer()
    {
        m_Renderer = new VulkanRenderer();
        m_Renderer->Initialize();

        return true;
    }

    void MainLoop::DestroyRenderer()
    {
        m_Renderer->Destroy();
        SAFE_DELETE(m_Renderer);
    }

    bool MainLoop::InitializeWindow(const char *title, size_t width, size_t height)
    {
        m_PlatformWindowBase = PlatformWindow::getInstance();
        bool result = m_PlatformWindowBase->Initialize(title, width, height);

        if (result)
        {
            m_PlatformWindowBase->SetRenderer(m_Renderer);
            m_Renderer->SetPlatformWindow(m_PlatformWindowBase);
            m_PlatformWindowBase->CreateVulkanSurface();
        }

        return result;
    }

    void MainLoop::DestroyWindow()
    {
        m_PlatformWindowBase->DestroyVulkanSurface();
        m_PlatformWindowBase->Destroy();
        SAFE_DELETE(m_PlatformWindowBase);
    }

    bool MainLoop::Iterate() const
    {
        //  TODO: Low CPU usage mode
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        return m_Renderer->RenderFrame();
    }
}  // namespace aga