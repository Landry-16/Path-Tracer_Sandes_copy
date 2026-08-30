/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** FlatMaterial
*/

#include "rt/materials/FlatMaterial.hpp"
#include "rt/textures/SolidColorTexture.hpp"

FlatMaterial::FlatMaterial(const Color &color)
    : color(color), texture(std::make_shared<SolidColorTexture>(color)),
      specular(Color(1.0, 1.0, 1.0)), shininess(64.0), hasExplicitTexture_(false)
{
}

FlatMaterial::FlatMaterial(const Color &color, std::shared_ptr<ITexture> texture)
    : color(color), texture(std::move(texture)),
      specular(Color(1.0, 1.0, 1.0)), shininess(64.0), hasExplicitTexture_(true)
{
}

Color FlatMaterial::getColor() const
{
    return color;
}

Color FlatMaterial::getDiffuse() const
{
    return color;
}

bool FlatMaterial::isReflective() const
{
    return false;
}

double FlatMaterial::getReflectivity() const
{
    return 0.0;
}

std::shared_ptr<const ITexture> FlatMaterial::getTexture() const
{
    return texture;
}

Color FlatMaterial::getSpecular() const
{
    return specular;
}

double FlatMaterial::getShininess() const
{
    return shininess;
}

void FlatMaterial::setSpecular(const Color &spec, double shin)
{
    specular = spec;
    shininess = shin;
}

/**
 * @brief Sets the diffuse color and refreshes the solid-color texture
 * when no explicit texture was provided at construction.
 * @param c: the new diffuse color
 */
void FlatMaterial::setColor(const Color &c)
{
    color = c;
    if (!hasExplicitTexture_)
        texture = std::make_shared<SolidColorTexture>(c);
}
