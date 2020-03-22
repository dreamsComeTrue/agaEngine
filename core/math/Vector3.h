// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#pragma once

namespace aga
{
    class Vector3
    {
    public:
        Vector3() : X(0.0f), Y(0.0f), Z(0.0f)
        {
        }

        Vector3(float x, float y, float z) : X(x), Y(y), Z(z)
        {
        }

    public:
        union {
            struct
            {
                float X;
                float Y;
                float Z;
            };
            struct
            {
                float Width;
                float Height;
                float Depth;
            };
        };
    };

}  // namespace aga
