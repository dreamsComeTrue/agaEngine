// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#pragma once

#include "core/String.h"
#include "core/Typedefs.h"
#include "platform/Platform.h"

namespace aga
{
    class VulkanRenderer;

    class PlatformWindowBase
    {
    public:
        PlatformWindowBase();
        virtual ~PlatformWindowBase();

        virtual bool Initialize(const char *title, uint32_t width = 1280,
                                uint32_t height = 800) = 0;
        virtual void Destroy() = 0;
        virtual bool Update() = 0;

        virtual VkExtent2D GetCurrentWindowSize() = 0;

        void Close();

        void SetRenderer(VulkanRenderer *renderer);

    public:
        virtual VkSurfaceKHR CreateVulkanSurface() = 0;

    protected:
        VulkanRenderer *m_Renderer;
        String m_Name;
        uint32_t m_Width;
        uint32_t m_Height;
        bool m_ShouldRun;
    };

    class PlatformWindow
    {
    public:
        static PlatformWindowBase *getInstance();

    private:
        PlatformWindow()
        {
        }

    public:
        PlatformWindow(PlatformWindow const &) = delete;
        void operator=(PlatformWindow const &) = delete;

    private:
        static PlatformWindowBase *s_PLatformWindowBase;
    };
}  // namespace aga
