/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** CheckerTexture
*/

#include "rt/textures/CheckerTexture.hpp"
#include <cmath>

CheckerTexture::CheckerTexture(const Color &color1, const Color &color2, double scale)
    : color1(color1), color2(color2), scale(scale)
{
}

Color CheckerTexture::sample(double u, double v) const
{
    int sines = static_cast<int>(std::floor(scale * u) + std::floor(scale * v));
    return (sines % 2 == 0) ? color1 : color2;
}
