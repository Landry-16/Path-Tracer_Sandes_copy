/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** SolidColorTexture
*/

#ifndef RT_SOLIDCOLORTEXTURE_HPP
    #define RT_SOLIDCOLORTEXTURE_HPP

    #include "rt/interfaces/ITexture.hpp"
    #include "rt/math/Color.hpp"

class SolidColorTexture : public ITexture {
public:
    SolidColorTexture(const Color &color);
    Color sample(double u, double v) const override;

private:
    Color color;
};

#endif // RT_SOLIDCOLORTEXTURE_HPP
