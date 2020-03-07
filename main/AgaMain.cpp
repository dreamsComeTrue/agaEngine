// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#include "MainLoop.h"
#include "core/Logger.h"
#include "core/Memory.h"
#include "core/Typedefs.h"

int main(int argc, char *argv[])
{
    LOG_INFO("..:: agaEngine ::.. [v " + ENGINE_VERSION_STRING + "]\n");

    {
        aga::MainLoop mainLoop;
        mainLoop.Iterate();
    }

    LOG_DEBUG("Finishing agaEngine\n");

    return 0;
}