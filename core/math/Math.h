// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#pragma once

#include "core/Typedefs.h"

#include <math.h>

namespace aga
{
    const real_t PI = 3.14159265359f;
    const real_t RECIPROCAL_PI = 1.0 / PI;
    const real_t DEGTORAD = PI / 180.0f;
    const real_t RADTODEG = 180.0f / PI;
    const real_t DEGTORAD64 = PI / 180.0;
    const real_t RADTODEG64 = 180.0 / PI;

    real_t Reciprocal(const real_t f);
    real_t ReciprocalSquareRoot(const real_t x);

    real_t RadToDeg(real_t radians);
    real_t DegToRad(real_t degrees);

}  // namespace aga
