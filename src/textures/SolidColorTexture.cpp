/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** SolidColorTexture
*/

#include "rt/textures/SolidColorTexture.hpp"

SolidColorTexture::SolidColorTexture(const Color &color) : color(color)
{
}

Color SolidColorTexture::sample(double u, double v) const
{
    (void)u;
    (void)v;
    return color;
}
