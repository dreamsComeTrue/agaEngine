// Copyright (C) 2020 Dominik 'dreamsComeTrue' Jasiński

#include "Matrix.h"
#include "core/Macros.h"

namespace aga
{
    Matrix::Matrix()
    {
        SetIdentity();
    }

    Matrix::~Matrix()
    {
    }

    Matrix::Matrix(const Matrix &m)
    {
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                m_Data[i][j] = m.m_Data[i][j];
            }
        }
    }

    Matrix &Matrix::operator=(const Matrix &m)
    {
        if (this == &m)
        {
            return *this;
        }

        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                m_Data[i][j] = m.m_Data[i][j];
            }
        }

        return *this;
    }

    Matrix &Matrix::operator+=(const Matrix &m)
    {
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                m_Data[i][j] += m.m_Data[i][j];
            }
        }

        return *this;
    }

    Matrix &Matrix::operator-=(const Matrix &m)
    {
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                m_Data[i][j] -= m.m_Data[i][j];
            }
        }

        return *this;
    }

    Matrix &Matrix::operator*=(const Matrix &m)
    {
        Matrix temp;

        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                for (int k = 0; k < 4; ++k)
                {
                    temp.m_Data[i][j] += (m_Data[i][k] * m.m_Data[k][j]);
                }
            }
        }

        return (*this = temp);
    }

    Matrix &Matrix::operator*=(real_t num)
    {
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                m_Data[i][j] *= num;
            }
        }

        return *this;
    }

    Matrix &Matrix::operator/=(real_t num)
    {
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                m_Data[i][j] /= num;
            }
        }

        return *this;
    }

    Matrix Matrix::Transpose()
    {
        Matrix ret;

        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                ret.m_Data[j][i] = m_Data[i][j];
            }
        }

        return ret;
    }

    Matrix &Matrix::SetIdentity()
    {
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                if (i == j)
                {
                    m_Data[i][j] = 1;
                }
                else
                {
                    m_Data[i][j] = 0;
                }
            }
        }

        return *this;
    }

    Matrix &Matrix::SetRotationAxisRadians(const real_t &angle, const Vector3 &axis)
    {
        const real_t c = ::cos(angle);
        const real_t s = ::sin(angle);
        const real_t t = 1.0 - c;

        const real_t tx = t * axis.X;
        const real_t ty = t * axis.Y;
        const real_t tz = t * axis.Z;

        const real_t sx = s * axis.X;
        const real_t sy = s * axis.Y;
        const real_t sz = s * axis.Z;

        m_Data[0][0] = (real_t)(tx * axis.X + c);
        m_Data[0][1] = (real_t)(tx * axis.Y + sz);
        m_Data[0][2] = (real_t)(tx * axis.Z - sy);

        m_Data[1][0] = (real_t)(ty * axis.X - sz);
        m_Data[1][1] = (real_t)(ty * axis.Y + c);
        m_Data[1][2] = (real_t)(ty * axis.Z + sx);

        m_Data[2][0] = (real_t)(tz * axis.X + sy);
        m_Data[2][1] = (real_t)(tz * axis.Y - sx);
        m_Data[2][2] = (real_t)(tz * axis.Z + c);

        return *this;
    }

    Matrix &Matrix::LookAt(const Vector3 &position, const Vector3 &target, const Vector3 &upVector)
    {
        Vector3 zaxis = position - target;
        zaxis.Normalize();

        Vector3 xaxis = upVector.CrossProduct(zaxis);
        xaxis.Normalize();

        Vector3 yaxis = zaxis.CrossProduct(xaxis);

        m_Data[0][0] = xaxis.X;
        m_Data[0][1] = yaxis.X;
        m_Data[0][2] = zaxis.X;
        m_Data[0][3] = 0;

        m_Data[1][0] = xaxis.Y;
        m_Data[1][1] = yaxis.Y;
        m_Data[1][2] = zaxis.Y;
        m_Data[1][3] = 0;

        m_Data[2][0] = xaxis.Z;
        m_Data[2][1] = yaxis.Z;
        m_Data[2][2] = zaxis.Z;
        m_Data[2][3] = 0;

        m_Data[3][0] = -xaxis.DotProduct(position);
        m_Data[3][1] = -yaxis.DotProduct(position);
        m_Data[3][2] = -zaxis.DotProduct(position);
        m_Data[3][3] = 1;

        return *this;
    }

    Matrix &Matrix::ProjectionMatrixPerspectiveFov(real_t fieldOfViewRadians, real_t aspectRatio, real_t zNear,
                                                   real_t zFar)
    {
        const real_t h = Reciprocal(tan(fieldOfViewRadians * 0.5));
        const real_t w = static_cast<real_t>(h / aspectRatio);

        m_Data[0][0] = w;
        m_Data[0][1] = 0;
        m_Data[0][2] = 0;
        m_Data[0][3] = 0;

        m_Data[1][0] = 0;
        m_Data[1][1] = h;
        m_Data[1][2] = 0;
        m_Data[1][3] = 0;

        m_Data[2][0] = 0;
        m_Data[2][1] = 0;
        m_Data[2][2] = (real_t)(zFar / (zNear - zFar));
        m_Data[2][3] = -1;

        m_Data[3][0] = 0;
        m_Data[3][1] = 0;
        m_Data[3][2] = (real_t)(zNear * zFar / (zNear - zFar));
        m_Data[3][3] = 0;

        return *this;
    }

    Matrix operator+(const Matrix &m1, const Matrix &m2)
    {
        Matrix temp(m1);
        return (temp += m2);
    }

    Matrix operator-(const Matrix &m1, const Matrix &m2)
    {
        Matrix temp(m1);
        return (temp -= m2);
    }

    Matrix operator*(const Matrix &m1, const Matrix &m2)
    {
        Matrix temp(m1);
        return (temp *= m2);
    }

    Matrix operator*(const Matrix &m, real_t num)
    {
        Matrix temp(m);
        return (temp *= num);
    }

    Matrix operator*(real_t num, const Matrix &m)
    {
        return (m * num);
    }

    Matrix operator/(const Matrix &m, real_t num)
    {
        Matrix temp(m);
        return (temp /= num);
    }
}  // namespace aga