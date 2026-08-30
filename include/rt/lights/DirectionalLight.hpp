/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** DirectionalLight
*/

#ifndef RT_DIRECTIONALLIGHT_HPP
    #define RT_DIRECTIONALLIGHT_HPP

    #include "rt/interfaces/ILight.hpp"

class DirectionalLight : public ILight {
public:
    DirectionalLight(const Vec3 &direction, const Color &color, double intensity);

    Color getIntensity() const override;
    Vec3 getDirection(const Vec3 &point) const override;
    bool isDirectional() const override;

    Color getColor() const override { return color; }
    double getBaseIntensity() const override { return intensity; }
    void setColor(const Color &c) override { color = c; }
    void setBaseIntensity(double i) override { intensity = i; }

private:
    Vec3 direction;
    Color color;
    double intensity;
};

#endif // RT_DIRECTIONALLIGHT_HPP
