/*
** EPITECH PROJECT, 2026
** RAYTRACER
** File description:
** emissive_material
*/

#include <memory>
#include <cmath>
#include <map>
#include "rt/interfaces/IMaterial.hpp"
#include "rt/math/Color.hpp"
#include "rt/math/Vec3.hpp"
#include "rt/math/Ray.hpp"
#include "rt/core/Factory.hpp"

class EmissiveMaterial : public IMaterial {
private:
    Color baseColor;
    Color emissionColor;
    double emissionStrength;

public:
    EmissiveMaterial(const Color &base, const Color &emission, double strength)
        : baseColor(base), emissionColor(emission), emissionStrength(strength)
    {}

    Color getColor() const override
    {
        return baseColor;
    }

    Color getDiffuse() const override
    {
        return baseColor;
    }

    bool isReflective() const override
    {
        return false;
    }

    double getReflectivity() const override
    {
        return 0.0;
    }

    bool isPBR() const override
    {
        return false;
    }

    Color getEmission() const override
    {
        return emissionColor * emissionStrength;
    }

    void setColor(const Color &c) override { baseColor = c; }

    void setEmission(const Color &e) override { emissionColor = e; }

    Color evaluateBRDF(const Vec3&, const Vec3&, const Vec3&, double, double) const override
    {
        return Color(0, 0, 0);
    }

    ScatterResult scatter(const Vec3&, const Vec3&, double, double, double, double) const override
    {
        ScatterResult result;
        result.valid = false;
        return result;
    }
};

class EmissiveDiffuseMaterial : public IMaterial {
private:
    Color baseColor;
    Color emissionColor;
    double emissionStrength;
    double roughness;

public:
    EmissiveDiffuseMaterial(const Color &base, const Color &emission, double strength, double rough)
        : baseColor(base), emissionColor(emission), emissionStrength(strength), roughness(rough)
    {}

    Color getColor() const override
    {
        return baseColor;
    }

    Color getDiffuse() const override
    {
        return baseColor;
    }

    bool isReflective() const override
    {
        return false;
    }

    double getReflectivity() const override
    {
        return 0.0;
    }

    bool isPBR() const override
    {
        return false;
    }

    Color getEmission() const override
    {
        return emissionColor * emissionStrength;
    }

    Color evaluateBRDF(const Vec3&, const Vec3 &wi, const Vec3 &normal, double, double) const override
    {
        double NdotL = std::max(0.0, normal.dot(wi));
        return baseColor * (NdotL / M_PI);
    }

    ScatterResult scatter(const Vec3&, const Vec3 &normal, double, double, double r1, double r2) const override
    {
        double phi = 2.0 * M_PI * r1;
        double cosTheta = std::sqrt(1.0 - r2);
        double sinTheta = std::sqrt(r2);

        Vec3 w = normal;
        Vec3 u = ((std::abs(w.x) > 0.1 ? Vec3(0, 1, 0) : Vec3(1, 0, 0)).cross(w)).normalized();
        Vec3 v = w.cross(u);

        Vec3 direction = (u * std::cos(phi) * sinTheta +
                         v * std::sin(phi) * sinTheta +
                         w * cosTheta).normalized();

        ScatterResult result;
        result.direction = direction;
        result.attenuation = baseColor * 2.0;
        result.pdf = cosTheta / M_PI;
        result.valid = true;

        return result;
    }

    double getRoughness() const override
    {
        return roughness;
    }
};

extern "C" {
    void rt_plugin_register()
    {
        MaterialRegistry::instance().registerType("emissive",
            [](const MaterialParams &params) -> std::unique_ptr<IMaterial> {
                return std::make_unique<EmissiveMaterial>(params.color, params.color, params.emissionStrength);
            }
        );
        MaterialRegistry::instance().registerType("emissive_diffuse",
            [](const MaterialParams &params) -> std::unique_ptr<IMaterial> {
                return std::make_unique<EmissiveDiffuseMaterial>(params.color, params.color, params.emissionStrength, 0.8);
            }
        );
        static const std::map<std::string, Color> LIGHT_PRESETS = {
            {"neon_red",    {1.0, 0.1,  0.15}},
            {"neon_blue",   {0.1, 0.5,  1.0}},
            {"neon_cyan",   {0.0, 1.0,  1.0}},
            {"neon_pink",   {1.0, 0.1,  0.6}},
            {"neon_green",  {0.2, 1.0,  0.3}},
            {"neon_orange", {1.0, 0.4,  0.0}},
            {"warm_light",  {1.0, 0.9,  0.7}},
            {"cool_light",  {0.7, 0.8,  1.0}},
        };
        for (const auto &entry : LIGHT_PRESETS) {
            const Color c = entry.second;
            MaterialRegistry::instance().registerType(entry.first,
                [c](const MaterialParams &p) -> std::unique_ptr<IMaterial> {
                    return std::make_unique<EmissiveMaterial>(c, c, p.emissionStrength);
                }
            );
        }
    }
}

