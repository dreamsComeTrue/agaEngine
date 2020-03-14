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

        return true;
    }

    void MainLoop::DestroyRenderer()
    {
        m_Renderer->DestroySynchronizations();
        m_Renderer->DestroyFrameBuffers();
        m_Renderer->DestroyRenderPass();
        m_Renderer->DestroyDepthStencilImage();
        m_Renderer->DestroySwapChainImages();
        m_Renderer->DestroySwapChain();
        m_Renderer->DestroyVulkanSurface();
        m_Renderer->Destroy();
        SAFE_DELETE(m_Renderer);
    }

    bool MainLoop::InitializeWindow()
    {
        m_PlatformWindowBase = PlatformWindow::getInstance();
        m_PlatformWindowBase->SetRenderer(m_Renderer);

        return true;
    }

    bool MainLoop::Initialize(const char *title, size_t width, size_t height)
    {
        if (m_PlatformWindowBase->Initialize(title, width, height))
        {
            m_Renderer->SetPlatformWindow(m_PlatformWindowBase);
            m_Renderer->SetSurfaceSize(width, height);
            m_Renderer->Initialize();

            if (m_Renderer->CreateVulkanSurface())
            {
                if (m_Renderer->CreateSwapChain())
                {
                    if (m_Renderer->CreateSwapChainImages())
                    {
                        if (m_Renderer->CreateDepthStencilImage())
                        {
                            if (m_Renderer->CreateRenderPass())
                            {
                                if (m_Renderer->CreateFrameBuffers())
                                {
                                    return m_Renderer->CreateSynchronizations();
                                }
                            }
                        }
                    }
                }
            }
        }

        return false;
    }

    void MainLoop::DestroyWindow()
    {
        vkQueueWaitIdle(m_Renderer->GetVulkanQueue());
        SAFE_DELETE(m_PlatformWindowBase);
    }

    bool MainLoop::Iterate() const
    {
        //  TODO: Low CPU usage mode
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        m_Renderer->BeginRender();
        m_Renderer->RenderFrame();
        m_Renderer->EndRender();

        return m_PlatformWindowBase->Update();
    }
}  // namespace aga