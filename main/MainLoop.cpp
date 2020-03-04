// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#include "MainLoop.h"
#include "core/Macros.h"
#include "render/VulkanRenderer.h"

namespace aga
{
    MainLoop::MainLoop() : m_Renderer(new VulkanRenderer())
    {
    }

    MainLoop::~MainLoop()
    {
        SAFE_DELETE(m_Renderer);
    }

}  // namespace aga