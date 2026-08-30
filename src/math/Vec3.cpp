/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** Vec3
*/

#include "rt/math/Vec3.hpp"

Vec3::Vec3() : x(0), y(0), z(0)
{
}

Vec3::Vec3(double x, double y, double z) : x(x), y(y), z(z)
{
}

Vec3 Vec3::operator+(const Vec3 &v) const
{
    return Vec3(x + v.x, y + v.y, z + v.z);
}

Vec3 Vec3::operator-(const Vec3 &v) const
{
    return Vec3(x - v.x, y - v.y, z - v.z);
}

Vec3 Vec3::operator*(double t) const
{
    return Vec3(x * t, y * t, z * t);
}

Vec3 Vec3::operator*(const Vec3 &v) const
{
    return Vec3(x * v.x, y * v.y, z * v.z);
}

Vec3 Vec3::operator/(double t) const
{
    return Vec3(x / t, y / t, z / t);
}

Vec3 Vec3::operator-() const
{
    return Vec3(-x, -y, -z);
}

Vec3 &Vec3::operator+=(const Vec3 &v)
{
    x += v.x;
    y += v.y;
    z += v.z;
    return *this;
}

Vec3 &Vec3::operator-=(const Vec3 &v)
{
    x -= v.x;
    y -= v.y;
    z -= v.z;
    return *this;
}

Vec3 &Vec3::operator*=(double t)
{
    x *= t;
    y *= t;
    z *= t;
    return *this;
}

double Vec3::length() const
{
    return std::sqrt(x * x + y * y + z * z);
}

double Vec3::lengthSquared() const
{
    return x * x + y * y + z * z;
}

Vec3 Vec3::normalized() const
{
    double len = length();
    if (len > 0)
        return *this / len;
    return *this;
}

double Vec3::dot(const Vec3 &v) const
{
    return x * v.x + y * v.y + z * v.z;
}

Vec3 Vec3::cross(const Vec3 &v) const
{
    return Vec3(
        y * v.z - z * v.y,
        z * v.x - x * v.z,
        x * v.y - y * v.x
    );
}

Vec3 operator*(double t, const Vec3 &v)
{
    return v * t;
}
