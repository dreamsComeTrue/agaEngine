// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#pragma once

#include "core/Typedefs.h"

namespace aga
{
    class VulkanRenderer;
    class PlatformWindow;

    class MainLoop
    {
    public:
        MainLoop();
        ~MainLoop();

        bool InitializeWindow(const char* title, size_t width = 1024, size_t height = 768);
        void DestroyWindow();

        bool InitializeRenderer();
        void DestroyRenderer();
        
        bool Iterate() const;

    private:
        VulkanRenderer *m_Renderer;
        PlatformWindow *m_PlatformWindow;
    };
}  // namespace aga
