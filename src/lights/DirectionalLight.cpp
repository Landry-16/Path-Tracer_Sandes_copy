/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** DirectionalLight
*/

#include "rt/lights/DirectionalLight.hpp"

DirectionalLight::DirectionalLight(const Vec3 &direction, const Color &color, double intensity)
    : direction(direction.normalized()), color(color), intensity(intensity) {}

Color DirectionalLight::getIntensity() const {
    return color * intensity;
}

Vec3 DirectionalLight::getDirection(const Vec3 &point) const {
    (void)point;
    return direction * -1.0;
}

bool DirectionalLight::isDirectional() const {
    return true;
}
