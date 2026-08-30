/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** AmbientLight
*/

#include "rt/lights/AmbientLight.hpp"

AmbientLight::AmbientLight(const Color &color, double intensity)
    : color(color), intensity(intensity)
{
}

Color AmbientLight::getIntensity() const
{
    return color * intensity;
}

Vec3 AmbientLight::getDirection(const Vec3 &point) const
{
    (void)point;
    return Vec3(0, 0, 0);
}

bool AmbientLight::isDirectional() const
{
    return false;
}
