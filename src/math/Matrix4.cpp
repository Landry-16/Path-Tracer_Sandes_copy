/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** Matrix4
*/

#include "rt/math/Matrix4.hpp"
#include <cmath>
#include <cstring>

Matrix4::Matrix4()
{
    std::memset(m, 0, sizeof(m));
}

Matrix4 Matrix4::identity()
{
    Matrix4 result;
    for (int i = 0; i < 4; ++i)
        result.m[i][i] = 1.0;
    return result;
}

Matrix4 Matrix4::translation(double x, double y, double z)
{
    Matrix4 result = identity();
    result.m[0][3] = x;
    result.m[1][3] = y;
    result.m[2][3] = z;
    return result;
}

Matrix4 Matrix4::scale(double x, double y, double z)
{
    Matrix4 result = identity();
    result.m[0][0] = x;
    result.m[1][1] = y;
    result.m[2][2] = z;
    return result;
}

Matrix4 Matrix4::rotationX(double angle)
{
    Matrix4 result = identity();
    double c = std::cos(angle);
    double s = std::sin(angle);
    result.m[1][1] = c;
    result.m[1][2] = -s;
    result.m[2][1] = s;
    result.m[2][2] = c;
    return result;
}

Matrix4 Matrix4::rotationY(double angle)
{
    Matrix4 result = identity();
    double c = std::cos(angle);
    double s = std::sin(angle);
    result.m[0][0] = c;
    result.m[0][2] = s;
    result.m[2][0] = -s;
    result.m[2][2] = c;
    return result;
}

Matrix4 Matrix4::rotationZ(double angle)
{
    Matrix4 result = identity();
    double c = std::cos(angle);
    double s = std::sin(angle);
    result.m[0][0] = c;
    result.m[0][1] = -s;
    result.m[1][0] = s;
    result.m[1][1] = c;
    return result;
}

Matrix4 Matrix4::operator*(const Matrix4 &other) const
{
    Matrix4 result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result.m[i][j] = 0;
            for (int k = 0; k < 4; ++k) {
                result.m[i][j] += m[i][k] * other.m[k][j];
            }
        }
    }
    return result;
}

Vec3 Matrix4::transformPoint(const Vec3 &p) const
{
    double x = m[0][0] * p.x + m[0][1] * p.y + m[0][2] * p.z + m[0][3];
    double y = m[1][0] * p.x + m[1][1] * p.y + m[1][2] * p.z + m[1][3];
    double z = m[2][0] * p.x + m[2][1] * p.y + m[2][2] * p.z + m[2][3];
    return Vec3(x, y, z);
}

Vec3 Matrix4::transformDirection(const Vec3 &d) const
{
    double x = m[0][0] * d.x + m[0][1] * d.y + m[0][2] * d.z;
    double y = m[1][0] * d.x + m[1][1] * d.y + m[1][2] * d.z;
    double z = m[2][0] * d.x + m[2][1] * d.y + m[2][2] * d.z;
    return Vec3(x, y, z);
}

Matrix4 Matrix4::transpose() const
{
    Matrix4 result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result.m[i][j] = m[j][i];
        }
    }
    return result;
}

static void fillInverseColumn0(const double m[4][4], Matrix4 &inv)
{
    inv.m[0][0] = m[1][1] * m[2][2] * m[3][3] - m[1][1] * m[2][3] * m[3][2] -
                  m[2][1] * m[1][2] * m[3][3] + m[2][1] * m[1][3] * m[3][2] +
                  m[3][1] * m[1][2] * m[2][3] - m[3][1] * m[1][3] * m[2][2];
    inv.m[1][0] = -m[1][0] * m[2][2] * m[3][3] + m[1][0] * m[2][3] * m[3][2] +
                   m[2][0] * m[1][2] * m[3][3] - m[2][0] * m[1][3] * m[3][2] -
                   m[3][0] * m[1][2] * m[2][3] + m[3][0] * m[1][3] * m[2][2];
    inv.m[2][0] = m[1][0] * m[2][1] * m[3][3] - m[1][0] * m[2][3] * m[3][1] -
                  m[2][0] * m[1][1] * m[3][3] + m[2][0] * m[1][3] * m[3][1] +
                  m[3][0] * m[1][1] * m[2][3] - m[3][0] * m[1][3] * m[2][1];
    inv.m[3][0] = -m[1][0] * m[2][1] * m[3][2] + m[1][0] * m[2][2] * m[3][1] +
                   m[2][0] * m[1][1] * m[3][2] - m[2][0] * m[1][2] * m[3][1] -
                   m[3][0] * m[1][1] * m[2][2] + m[3][0] * m[1][2] * m[2][1];
}

static void fillInverseColumn1(const double m[4][4], Matrix4 &inv)
{
    inv.m[0][1] = -m[0][1] * m[2][2] * m[3][3] + m[0][1] * m[2][3] * m[3][2] +
                   m[2][1] * m[0][2] * m[3][3] - m[2][1] * m[0][3] * m[3][2] -
                   m[3][1] * m[0][2] * m[2][3] + m[3][1] * m[0][3] * m[2][2];
    inv.m[1][1] = m[0][0] * m[2][2] * m[3][3] - m[0][0] * m[2][3] * m[3][2] -
                  m[2][0] * m[0][2] * m[3][3] + m[2][0] * m[0][3] * m[3][2] +
                  m[3][0] * m[0][2] * m[2][3] - m[3][0] * m[0][3] * m[2][2];
    inv.m[2][1] = -m[0][0] * m[2][1] * m[3][3] + m[0][0] * m[2][3] * m[3][1] +
                   m[2][0] * m[0][1] * m[3][3] - m[2][0] * m[0][3] * m[3][1] -
                   m[3][0] * m[0][1] * m[2][3] + m[3][0] * m[0][3] * m[2][1];
    inv.m[3][1] = m[0][0] * m[2][1] * m[3][2] - m[0][0] * m[2][2] * m[3][1] -
                  m[2][0] * m[0][1] * m[3][2] + m[2][0] * m[0][2] * m[3][1] +
                  m[3][0] * m[0][1] * m[2][2] - m[3][0] * m[0][2] * m[2][1];
}

static void fillInverseColumn2(const double m[4][4], Matrix4 &inv)
{
    inv.m[0][2] = m[0][1] * m[1][2] * m[3][3] - m[0][1] * m[1][3] * m[3][2] -
                  m[1][1] * m[0][2] * m[3][3] + m[1][1] * m[0][3] * m[3][2] +
                  m[3][1] * m[0][2] * m[1][3] - m[3][1] * m[0][3] * m[1][2];
    inv.m[1][2] = -m[0][0] * m[1][2] * m[3][3] + m[0][0] * m[1][3] * m[3][2] +
                   m[1][0] * m[0][2] * m[3][3] - m[1][0] * m[0][3] * m[3][2] -
                   m[3][0] * m[0][2] * m[1][3] + m[3][0] * m[0][3] * m[1][2];
    inv.m[2][2] = m[0][0] * m[1][1] * m[3][3] - m[0][0] * m[1][3] * m[3][1] -
                  m[1][0] * m[0][1] * m[3][3] + m[1][0] * m[0][3] * m[3][1] +
                  m[3][0] * m[0][1] * m[1][3] - m[3][0] * m[0][3] * m[1][1];
    inv.m[3][2] = -m[0][0] * m[1][1] * m[3][2] + m[0][0] * m[1][2] * m[3][1] +
                   m[1][0] * m[0][1] * m[3][2] - m[1][0] * m[0][2] * m[3][1] -
                   m[3][0] * m[0][1] * m[1][2] + m[3][0] * m[0][2] * m[1][1];
}

static void fillInverseColumn3(const double m[4][4], Matrix4 &inv)
{
    inv.m[0][3] = -m[0][1] * m[1][2] * m[2][3] + m[0][1] * m[1][3] * m[2][2] +
                   m[1][1] * m[0][2] * m[2][3] - m[1][1] * m[0][3] * m[2][2] -
                   m[2][1] * m[0][2] * m[1][3] + m[2][1] * m[0][3] * m[1][2];
    inv.m[1][3] = m[0][0] * m[1][2] * m[2][3] - m[0][0] * m[1][3] * m[2][2] -
                  m[1][0] * m[0][2] * m[2][3] + m[1][0] * m[0][3] * m[2][2] +
                  m[2][0] * m[0][2] * m[1][3] - m[2][0] * m[0][3] * m[1][2];
    inv.m[2][3] = -m[0][0] * m[1][1] * m[2][3] + m[0][0] * m[1][3] * m[2][1] +
                   m[1][0] * m[0][1] * m[2][3] - m[1][0] * m[0][3] * m[2][1] -
                   m[2][0] * m[0][1] * m[1][3] + m[2][0] * m[0][3] * m[1][1];
    inv.m[3][3] = m[0][0] * m[1][1] * m[2][2] - m[0][0] * m[1][2] * m[2][1] -
                  m[1][0] * m[0][1] * m[2][2] + m[1][0] * m[0][2] * m[2][1] +
                  m[2][0] * m[0][1] * m[1][2] - m[2][0] * m[0][2] * m[1][1];
}

Matrix4 Matrix4::inverse() const
{
    Matrix4 inv;
    fillInverseColumn0(m, inv);
    double det = m[0][0] * inv.m[0][0] + m[0][1] * inv.m[1][0] +
                 m[0][2] * inv.m[2][0] + m[0][3] * inv.m[3][0];
    fillInverseColumn1(m, inv);
    fillInverseColumn2(m, inv);
    fillInverseColumn3(m, inv);
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            inv.m[i][j] /= det;
    return inv;
}
