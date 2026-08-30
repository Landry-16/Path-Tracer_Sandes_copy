/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** PointLight
*/

#include "rt/lights/PointLight.hpp"

PointLight::PointLight(const Vec3 &position, const Color &intensity)
    : position(position), intensity(intensity)
{
}

Color PointLight::getIntensity() const
{
    return intensity;
}

Vec3 PointLight::getDirection(const Vec3 &point) const
{
    return position - point;
}

bool PointLight::isDirectional() const
{
    return false;
}

Vec3 PointLight::getPosition() const
{
    return position;
}

double PointLight::getDistance(const Vec3 &point) const
{
    return (position - point).length();
}
