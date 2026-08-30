/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** ITexture
*/

#ifndef RT_ITEXTURE_HPP
    #define RT_ITEXTURE_HPP

    #include "rt/math/Color.hpp"

class ITexture {
public:
    virtual ~ITexture() = default;
    virtual Color sample(double u, double v) const = 0;
};

#endif // RT_ITEXTURE_HPP
