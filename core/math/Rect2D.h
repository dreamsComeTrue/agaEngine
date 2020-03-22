// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#pragma once

#include "Vector2.h"

namespace aga
{
    class Rect2D
    {
    public:
        Rect2D() = default;

        Rect2D(Vector2 pos, Vector2 size) : Position(pos), Size(size)
        {
        }

        Rect2D(float x, float y, float width, float height) : Position(x, y), Size(width, height)
        {
        }

    public:
        Vector2 Position;
        Vector2 Size;
    };

}  // namespace aga
