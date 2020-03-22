// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#pragma once

#include "Math.h"
namespace aga
{
    class Vector3
    {
    public:
        Vector3() : X(0.0f), Y(0.0f), Z(0.0f)
        {
        }

        Vector3(real_t x, real_t y, real_t z) : X(x), Y(y), Z(z)
        {
        }

        Vector3 &Normalize()
        {
            real_t length = X * X + Y * Y + Z * Z;

            if (length == 0)  // this check isn't an optimization but prevents getting NAN in the sqrt.
            {
                return *this;
            }

            length = ReciprocalSquareRoot(length);

            X = X * length;
            Y = Y * length;
            Z = Z * length;

            return *this;
        }

        real_t DotProduct(const Vector3 &other) const
        {
            return X * other.X + Y * other.Y + Z * other.Z;
        }

        Vector3 CrossProduct(const Vector3 &p) const
        {
            return Vector3(Y * p.Z - Z * p.Y, Z * p.X - X * p.Z, X * p.Y - Y * p.X);
        }

        Vector3 operator-() const
        {
            return Vector3(-X, -Y, -Z);
        }

        Vector3 &operator=(const Vector3 &other)
        {
            X = other.X;
            Y = other.Y;
            Z = other.Z;

            return *this;
        }

        Vector3 operator+(const Vector3 &other) const
        {
            return Vector3(X + other.X, Y + other.Y, Z + other.Z);
        }

        Vector3 &operator+=(const Vector3 &other)
        {
            X += other.X;
            Y += other.Y;
            Z += other.Z;
            return *this;
        }

        Vector3 operator+(const real_t val) const
        {
            return Vector3(X + val, Y + val, Z + val);
        }

        Vector3 &operator+=(const real_t val)
        {
            X += val;
            Y += val;
            Z += val;

            return *this;
        }

        Vector3 operator-(const Vector3 &other) const
        {
            return Vector3(X - other.X, Y - other.Y, Z - other.Z);
        }

        Vector3 &operator-=(const Vector3 &other)
        {
            X -= other.X;
            Y -= other.Y;
            Z -= other.Z;

            return *this;
        }

        Vector3 operator-(const real_t val) const
        {
            return Vector3(X - val, Y - val, Z - val);
        }

        Vector3 &operator-=(const real_t val)
        {
            X -= val;
            Y -= val;
            Z -= val;

            return *this;
        }

        Vector3 operator*(const Vector3 &other) const
        {
            return Vector3(X * other.X, Y * other.Y, Z * other.Z);
        }

        Vector3 &operator*=(const Vector3 &other)
        {
            X *= other.X;
            Y *= other.Y;
            Z *= other.Z;

            return *this;
        }

        Vector3 operator*(const real_t v) const
        {
            return Vector3(X * v, Y * v, Z * v);
        }

        Vector3 &operator*=(const real_t v)
        {
            X *= v;
            Y *= v;
            Z *= v;

            return *this;
        }

        Vector3 operator/(const Vector3 &other) const
        {
            return Vector3(X / other.X, Y / other.Y, Z / other.Z);
        }

        Vector3 &operator/=(const Vector3 &other)
        {
            X /= other.X;
            Y /= other.Y;
            Z /= other.Z;

            return *this;
        }

        Vector3 operator/(const real_t v) const
        {
            real_t i = (real_t)1.0 / v;
            return Vector3(X * i, Y * i, Z * i);
        }

        Vector3 &operator/=(const real_t v)
        {
            real_t i = (real_t)1.0 / v;
            X *= i;
            Y *= i;
            Z *= i;

            return *this;
        }

    public:
        union {
            struct
            {
                real_t X;
                real_t Y;
                real_t Z;
            };
            struct
            {
                real_t Width;
                real_t Height;
                real_t Depth;
            };
        };
    };

}  // namespace aga
