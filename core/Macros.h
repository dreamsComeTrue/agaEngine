// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#pragma once

#define SAFE_DELETE(x)                                                                             \
    {                                                                                              \
        if (x)                                                                                     \
        {                                                                                          \
            delete x;                                                                              \
            x = nullptr;                                                                           \
        }                                                                                          \
    }
