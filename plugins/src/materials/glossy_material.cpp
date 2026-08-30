/*
** EPITECH PROJECT, 2026
** RAYTRACER
** File description:
** glossy_material
*/

#include "rt/interfaces/IMaterial.hpp"
#include "rt/math/Color.hpp"
#include "rt/core/Factory.hpp"
#include <memory>

class GlossyMaterial : public IMaterial {
public:
    GlossyMaterial(const Color &color, double reflectivity, double roughness)
        : color(color), reflectivity(reflectivity), roughness(roughness)
    {}

    Color getColor() const override
    {
        return color;
    }

    Color getDiffuse() const override
    {
        return color * (1.0 - reflectivity);
    }

    bool isReflective() const override
    {
        return true;
    }

    double getReflectivity() const override
    {
        return reflectivity;
    }

    Color getSpecular() const override
    {
        return Color(1.0, 1.0, 1.0) * reflectivity;
    }

    double getShininess() const override
    {
        return 200.0 * (1.0 - roughness);
    }

    std::shared_ptr<const ITexture> getTexture() const override
    {
        return nullptr;
    }

    void setColor(const Color &c) override { color = c; }
    void setRoughness(double r) override { roughness = r; }
    void setReflectivity(double r) override { reflectivity = r; }

private:
    Color color;
    double reflectivity;
    double roughness;
};

extern "C" {
    void rt_plugin_register()
    {
        MaterialRegistry::instance().registerType("glossy",
            [](const MaterialParams &p) -> std::unique_ptr<IMaterial> {
                double roughness = 0.2;
                if (p.refractiveIndex > 1.0 && p.refractiveIndex < 2.0) {
                    roughness = p.refractiveIndex - 1.0;
                }
                return std::make_unique<GlossyMaterial>(p.color, p.reflectivity, roughness);
            }
        );
    }
}
