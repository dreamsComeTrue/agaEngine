// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#pragma once

#include "core/Typedefs.h"
#include "core/String.h"

namespace aga
{
    class PlatformWindow
    {
    public:
        PlatformWindow();
        virtual ~PlatformWindow();

        virtual bool Initialize(const char* title, uint32_t width = 1024, uint32_t height = 768) = 0;
        virtual void Destroy() = 0;
        
        virtual bool Update() = 0;

        void Close();

    protected:
        String m_Name;
        uint32_t m_Width;
        uint32_t m_Height;
        bool m_ShouldRun;
    };
}  // namespace aga
