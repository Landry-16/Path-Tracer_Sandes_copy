/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** AmbientLight
*/

#ifndef RT_AMBIENTLIGHT_HPP
    #define RT_AMBIENTLIGHT_HPP

    #include "rt/interfaces/ILight.hpp"

class AmbientLight : public ILight {
public:
    AmbientLight(const Color &color, double intensity);

    Color getIntensity() const override;
    Vec3 getDirection(const Vec3 &point) const override;
    bool isDirectional() const override;

    Color getColor() const override { return color; }
    double getBaseIntensity() const override { return intensity; }
    void setColor(const Color &c) override { color = c; }
    void setBaseIntensity(double i) override { intensity = i; }

private:
    Color color;
    double intensity;
};

#endif // RT_AMBIENTLIGHT_HPP
