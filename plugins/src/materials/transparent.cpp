/*
** EPITECH PROJECT, 2026
** RAYTRACER
** File description:
** transparent
*/

#include <memory>
#include <cmath>
#include <map>
#include "rt/interfaces/IMaterial.hpp"
#include "rt/interfaces/ITexture.hpp"
#include "rt/textures/SolidColorTexture.hpp"
#include "rt/core/Factory.hpp"
#include "rt/math/Color.hpp"
#include "rt/math/Vec3.hpp"

class TransparentMaterial : public IMaterial {
public:
    TransparentMaterial(const Color &color, double refractiveIndex, double reflectivity)
        : color(color), refractiveIndex(refractiveIndex), reflectivity(reflectivity),
          texture(std::make_shared<SolidColorTexture>(color))
    {}

    Color getColor() const override
    {
        return color;
    }

    Color getDiffuse() const override
    {
        return color;
    }

    bool isReflective() const override
    {
        return true;
    }

    double getReflectivity() const override
    {
        return reflectivity;
    }

    bool isTransparent() const override
    {
        return true;
    }

    double getRefractiveIndex() const override
    {
        return refractiveIndex;
    }

    std::shared_ptr<const ITexture> getTexture() const override
    {
        return texture;
    }

    void setColor(const Color &c) override { color = c; }
    void setRefractiveIndex(double r) override { refractiveIndex = r; }
    void setReflectivity(double r) override { reflectivity = r; }

private:
    Color color;
    double refractiveIndex;
    double reflectivity;
    std::shared_ptr<ITexture> texture;
};

extern "C" {
    void rt_plugin_register()
    {
        MaterialRegistry::instance().registerType("transparent",
            [](const MaterialParams &p) -> std::unique_ptr<IMaterial> {
                double ior = (p.refractiveIndex > 0.0) ? p.refractiveIndex : 1.5;
                return std::make_unique<TransparentMaterial>(p.color, ior, p.reflectivity);
            }
        );
        struct TransPreset { double ior; double reflectivity; };
        static const std::map<std::string, TransPreset> PRESETS = {
            {"glass",   {1.50, 0.90}},
            {"water",   {1.33, 0.02}},
            {"diamond", {2.42, 0.95}},
        };
        for (const auto &entry : PRESETS) {
            const TransPreset tp = entry.second;
            MaterialRegistry::instance().registerType(entry.first,
                [tp](const MaterialParams &p) -> std::unique_ptr<IMaterial> {
                    return std::make_unique<TransparentMaterial>(p.color, tp.ior, tp.reflectivity);
                }
            );
        }
    }
}
