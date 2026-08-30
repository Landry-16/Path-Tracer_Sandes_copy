/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** ILight
*/

#ifndef RT_ILIGHT_HPP
    #define RT_ILIGHT_HPP

    #include "rt/math/Vec3.hpp"
    #include "rt/math/Color.hpp"

class ILight {
public:
    virtual ~ILight() = default;
    virtual Color getIntensity() const = 0;
    virtual Vec3 getDirection(const Vec3 &point) const = 0;
    virtual bool isDirectional() const = 0;

    virtual Color getColor() const { return Color(0, 0, 0); }
    virtual double getBaseIntensity() const { return 0.0; }
    virtual void setColor(const Color &) {}
    virtual void setBaseIntensity(double) {}
};

#endif // RT_ILIGHT_HPP
