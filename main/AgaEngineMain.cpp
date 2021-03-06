// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#include "MainLoop.h"
#include "core/Logger.h"
#include "core/Typedefs.h"

int main(int argc, char *argv[])
{
    aga::String title = "..:: agaEngine ::..";
    LOG_INFO(title + " [v " + ENGINE_VERSION_STRING + "]\n");

    {
        aga::MainLoop mainLoop;

        if (!mainLoop.InitializeRenderer())
        {
            exit(-1);
        }

        if (!mainLoop.InitializeWindow())
        {
            exit(-1);
        }
        
        if (!mainLoop.Initialize(title))
        {
            exit(-1);
        }        

        while (mainLoop.Iterate())
            ;

        mainLoop.DestroyWindow();
        mainLoop.DestroyRenderer();
    }

    LOG_DEBUG("Finishing agaEngine\n");

    return 0;
}