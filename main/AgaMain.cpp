// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#include "core/Logger.h"
#include "render/VulkanRenderer.h"

int main(int argc, char *argv[])
{
    LOG_DEBUG("Starting agaEngine\n");

    {
        aga::VulkanRenderer renderer;
        renderer.RenderFrame();
    }

    LOG_DEBUG("Finishing agaEngine\n");

    return 0;
}