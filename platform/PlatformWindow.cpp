// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#include "PlatformWindow.h"

namespace aga
{
    PlatformWindow::PlatformWindow()
    {
    }

    PlatformWindow::~PlatformWindow()
    {
    }

    void PlatformWindow::Close()
    {
        m_ShouldRun = false;
    }
}  // namespace aga