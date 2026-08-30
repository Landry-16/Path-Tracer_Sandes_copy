/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** IMaterial
*/

#ifndef RT_IMATERIAL_HPP
    #define RT_IMATERIAL_HPP

    #include "rt/math/Color.hpp"
    #include "rt/math/Vec3.hpp"
    #include <memory>

class ITexture;

struct ScatterResult {
    Vec3 direction;
    Color attenuation;
    double pdf;
    bool valid;
};

class IMaterial {
public:
    virtual ~IMaterial() = default;

    virtual Color getColor() const = 0;
    virtual Color getDiffuse() const = 0;
    virtual bool isReflective() const = 0;
    virtual double getReflectivity() const = 0;
    virtual bool isTransparent() const { return false; }
    virtual double getRefractiveIndex() const { return 1.0; }
    virtual std::shared_ptr<const ITexture> getTexture() const { return nullptr; }

    virtual bool isPBR() const { return false; }
    virtual double getMetallic() const { return 0.0; }
    virtual double getRoughness() const { return 1.0; }
    virtual Color getEmission() const { return Color(0, 0, 0); }

    virtual Color getSpecular() const { return Color(0, 0, 0); }
    virtual double getShininess() const { return 0.0; }

    virtual void setColor(const Color &) {}
    virtual void setRoughness(double) {}
    virtual void setMetallic(double) {}
    virtual void setEmission(const Color &) {}
    virtual void setReflectivity(double) {}
    virtual void setRefractiveIndex(double) {}

    virtual Color evaluateBRDF(const Vec3&, const Vec3 &wi, const Vec3 &normal,
                               double = 0.0, double = 0.0) const {
        double cosTheta = std::max(0.0, normal.dot(wi));
        return getDiffuse() * (1.0 / M_PI) * cosTheta;
    }

    virtual ScatterResult scatter(const Vec3&, const Vec3&,
                                 double = 0.0, double = 0.0,
                                 double = 0.0, double = 0.0) const {
        return ScatterResult{Vec3(0, 0, 0), Color(0, 0, 0), 0.0, false};
    }
};

#endif // RT_IMATERIAL_HPP

