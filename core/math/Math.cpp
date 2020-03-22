// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#include "Math.h"

namespace aga
{
    real_t Reciprocal(const real_t f)
    {
        return 1.0 / f;
    }

    real_t ReciprocalSquareRoot(const real_t x)
    {
        return 1.0 / ::sqrt(x);
    }

    real_t RadToDeg(real_t radians)
    {
        return RADTODEG * radians;
    }

    real_t DegToRad(real_t degrees)
    {
        return DEGTORAD * degrees;
    }
}  // namespace aga