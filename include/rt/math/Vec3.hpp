/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** Vec3
*/

#ifndef RT_VEC3_HPP
    #define RT_VEC3_HPP

    #include <cmath>

class Vec3 {
public:
    double x, y, z;

    Vec3();
    Vec3(double x, double y, double z);

    Vec3 operator+(const Vec3 &v) const;
    Vec3 operator-(const Vec3 &v) const;
    Vec3 operator*(double t) const;
    Vec3 operator*(const Vec3 &v) const;
    Vec3 operator/(double t) const;
    Vec3 operator-() const;

    Vec3 &operator+=(const Vec3 &v);
    Vec3 &operator-=(const Vec3 &v);
    Vec3 &operator*=(double t);

    double length() const;
    double lengthSquared() const;
    Vec3 normalized() const;
    double dot(const Vec3 &v) const;
    Vec3 cross(const Vec3 &v) const;
};

Vec3 operator*(double t, const Vec3 &v);

#endif // RT_VEC3_HPP
