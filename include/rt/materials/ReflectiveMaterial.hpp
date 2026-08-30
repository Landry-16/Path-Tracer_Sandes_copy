/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** ReflectiveMaterial
*/

#ifndef RT_REFLECTIVEMATERIAL_HPP
    #define RT_REFLECTIVEMATERIAL_HPP

    #include "rt/interfaces/IMaterial.hpp"
    #include "rt/interfaces/ITexture.hpp"
    #include <memory>

class ReflectiveMaterial : public IMaterial {
public:
    ReflectiveMaterial(const Color &color, double reflectivity);
    ReflectiveMaterial(const Color &color, double reflectivity, std::shared_ptr<ITexture> texture);

    Color getColor() const override;
    Color getDiffuse() const override;
    bool isReflective() const override;
    double getReflectivity() const override;
    std::shared_ptr<const ITexture> getTexture() const override;

    void setColor(const Color &c) override;
    void setReflectivity(double r) override { reflectivity = r; }

private:
    Color color;
    double reflectivity;
    std::shared_ptr<ITexture> texture;
    /** @brief True when a custom texture was explicitly passed at construction. */
    bool hasExplicitTexture_ = false;
};

#endif // RT_REFLECTIVEMATERIAL_HPP
