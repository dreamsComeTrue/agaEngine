// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#pragma once

#include "Vector3.h"
#include "core/Typedefs.h"

namespace aga
{
    class Matrix
    {
    public:
        Matrix();
        ~Matrix();
        Matrix(const Matrix &);
        Matrix &operator=(const Matrix &);

        real_t &operator()(int x, int y)
        {
            return m_Data[x][y];
        }

        real_t *operator[](int index)
        {
            return m_Data[index];
        }

        Matrix &operator+=(const Matrix &);
        Matrix &operator-=(const Matrix &);
        Matrix &operator*=(const Matrix &);
        Matrix &operator*=(real_t);
        Matrix &operator/=(real_t);

        Matrix Transpose();

        Matrix &SetIdentity();

        Matrix &SetRotationAxisRadians(const real_t &angle, const Vector3 &axis);

        Matrix &LookAt(const Vector3 &position, const Vector3 &target, const Vector3 &upVector);
        Matrix &ProjectionMatrixPerspectiveFov(real_t fieldOfViewRadians, real_t aspectRatio, real_t zNear,
                                               real_t zFar);

    private:
        real_t m_Data[4][4];
    };

    Matrix operator+(const Matrix &, const Matrix &);
    Matrix operator-(const Matrix &, const Matrix &);
    Matrix operator*(const Matrix &, const Matrix &);
    Matrix operator*(const Matrix &, double);
    Matrix operator*(double, const Matrix &);
    Matrix operator/(const Matrix &, double);

}  // namespace aga
