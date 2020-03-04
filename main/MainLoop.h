// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#pragma once

namespace aga
{
    class VulkanRenderer;
    
    class MainLoop
    {
    public:
        MainLoop();
        ~MainLoop();

        bool Iterate();

    private:
        VulkanRenderer *m_Renderer;
    };
}  // namespace aga
