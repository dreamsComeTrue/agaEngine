// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#pragma once

#include "core/Typedefs.h"

namespace aga
{
    class VulkanRenderer;
    class PlatformWindowBase;

    class MainLoop
    {
    public:
        MainLoop();
        ~MainLoop();

        bool InitializeRenderer();
        void DestroyRenderer();

        bool InitializeWindow(const char* title, size_t width = 1024, size_t height = 768);
        void DestroyWindow();

        bool Iterate() const;

    private:
        VulkanRenderer *m_Renderer;
        PlatformWindowBase *m_PlatformWindowBase;
    };
}  // namespace aga
