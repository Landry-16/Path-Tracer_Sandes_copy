/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** CheckerTexture
*/

#ifndef RT_CHECKERTEXTURE_HPP
    #define RT_CHECKERTEXTURE_HPP

    #include "rt/interfaces/ITexture.hpp"
    #include "rt/math/Color.hpp"

class CheckerTexture : public ITexture {
public:
    CheckerTexture(const Color &color1, const Color &color2, double scale);
    Color sample(double u, double v) const override;

private:
    Color color1;
    Color color2;
    double scale;
};

#endif // RT_CHECKERTEXTURE_HPP
