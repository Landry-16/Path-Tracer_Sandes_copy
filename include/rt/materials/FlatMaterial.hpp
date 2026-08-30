/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** FlatMaterial
*/

#ifndef RT_FLATMATERIAL_HPP
    #define RT_FLATMATERIAL_HPP

    #include "rt/interfaces/IMaterial.hpp"
    #include "rt/interfaces/ITexture.hpp"
    #include "rt/math/Color.hpp"
    #include <memory>

class FlatMaterial : public IMaterial {
public:
    FlatMaterial(const Color &color);
    FlatMaterial(const Color &color, std::shared_ptr<ITexture> texture);

    Color getColor() const override;
    Color getDiffuse() const override;
    bool isReflective() const override;
    double getReflectivity() const override;
    std::shared_ptr<const ITexture> getTexture() const override;

    Color getSpecular() const override;
    double getShininess() const override;

    void setSpecular(const Color &spec, double shin);
    void setColor(const Color &c) override;

private:
    Color color;
    std::shared_ptr<ITexture> texture;
    Color specular;
    double shininess;
    /** @brief True when a custom texture was explicitly passed at construction. */
    bool hasExplicitTexture_ = false;
};

#endif // RT_FLATMATERIAL_HPP
