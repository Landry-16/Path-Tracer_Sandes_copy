/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** Ray
*/

#ifndef RT_RAY_HPP
    #define RT_RAY_HPP

    #include "rt/math/Vec3.hpp"

class Ray {
public:
    Vec3 origin;
    Vec3 direction;

    Ray();
    Ray(const Vec3  &origin, const Vec3   &direction);

    Vec3 at(double t) const;
};

#endif // RT_RAY_HPP
