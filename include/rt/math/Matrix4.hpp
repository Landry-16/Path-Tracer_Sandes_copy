/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** Matrix4
*/

#ifndef RT_MATRIX4_HPP
    #define RT_MATRIX4_HPP

    #include "rt/math/Vec3.hpp"

class Matrix4 {
public:
    double m[4][4];

    Matrix4();
    
    static Matrix4 identity();
    static Matrix4 translation(double x, double y, double z);
    static Matrix4 scale(double x, double y, double z);
    static Matrix4 rotationX(double angle);
    static Matrix4 rotationY(double angle);
    static Matrix4 rotationZ(double angle);

    Matrix4 operator*(const Matrix4 &other) const;
    Vec3 transformPoint(const Vec3 &p) const;
    Vec3 transformDirection(const Vec3 &d) const;
    
    Matrix4 inverse() const;
    Matrix4 transpose() const;
};

#endif // RT_MATRIX4_HPP
