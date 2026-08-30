/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** Ray
*/

#include "rt/math/Ray.hpp"

Ray::Ray() : origin(), direction()
{
}

Ray::Ray(const Vec3 &origin, const Vec3 &direction)
    : origin(origin), direction(direction)
{
}

Vec3 Ray::at(double t) const
{
    return origin + direction * t;
}
