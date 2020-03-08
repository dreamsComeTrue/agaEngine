// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#include "PlatformWindow.h"
#include "core/Logger.h"
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
    PlatformWindowBase::PlatformWindowBase() :
        m_Renderer(nullptr),
        m_ShouldRun(true)
    {
    }

    PlatformWindowBase::~PlatformWindowBase()
    {
    }

    void PlatformWindowBase::Close()
    {
        m_ShouldRun = false;
    }

    void PlatformWindowBase::SetRenderer(VulkanRenderer *renderer)
    {
        m_Renderer = renderer;
    }

#if defined(_WIN32)
    PlatformWindowBase *PlatformWindow::s_PLatformWindowBase = new X11PlatformWindow();
#elif defined(__linux)
    PlatformWindowBase *PlatformWindow::s_PLatformWindowBase = new X11PlatformWindow();
#endif

    PlatformWindowBase *PlatformWindow::getInstance()
    {
        return s_PLatformWindowBase;
    }
}  // namespace aga