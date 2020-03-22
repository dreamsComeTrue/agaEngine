// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#pragma once

#include "core/Typedefs.h"

namespace aga
{
    class Vector2
    {
    public:
        Vector2() : X(0.0f), Y(0.0f)
        {
        }

        Vector2(real_t x, real_t y) : X(x), Y(y)
        {
        }

    public:
        union {
            struct
            {
                real_t X;
                real_t Y;
            };
            struct
            {
                real_t Width;
                real_t Height;
            };
        };
    };

}  // namespace aga
