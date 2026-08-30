/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** BuiltinRegistration
*/

#include "rt/core/Factory.hpp"
#include "rt/primitives/Sphere.hpp"
#include "rt/primitives/Plane.hpp"
#include "rt/materials/FlatMaterial.hpp"
#include "rt/materials/ReflectiveMaterial.hpp"
#include "rt/lights/AmbientLight.hpp"
#include "rt/lights/DirectionalLight.hpp"
#include "rt/lights/PointLight.hpp"
#include "rt/textures/SolidColorTexture.hpp"
#include "rt/textures/CheckerTexture.hpp"

static std::unique_ptr<ITexture> buildTexture(const MaterialParams &m)
{
    if (!m.hasTexture || m.textureType.empty())
        return nullptr;
    if (!TextureRegistry::instance().isRegistered(m.textureType))
        return nullptr;
    return TextureRegistry::instance().create(m.textureType, m.textureParams);
}

static std::unique_ptr<IMaterial> buildFlatMaterial(const MaterialParams &m)
{
    auto texture = buildTexture(m);
    auto flat = texture
        ? std::make_unique<FlatMaterial>(m.color, std::move(texture))
        : std::make_unique<FlatMaterial>(m.color);
    if (m.hasSpecular && m.shininess > 0.0)
        flat->setSpecular(m.specular, m.shininess);
    return flat;
}

static std::unique_ptr<IMaterial> buildReflectiveMaterial(const MaterialParams &m)
{
    auto texture = buildTexture(m);
    if (texture)
        return std::make_unique<ReflectiveMaterial>(m.color, m.reflectivity, std::move(texture));
    return std::make_unique<ReflectiveMaterial>(m.color, m.reflectivity);
}

static void registerPrimitives()
{
    auto &reg = PrimitiveRegistry::instance();
    reg.registerType("sphere", [](const PrimitiveParams &p) -> std::unique_ptr<IPrimitive>
    {
        return std::make_unique<Sphere>(p.position, p.radius, p.material, p.transform);
    });
    reg.registerType("plane", [](const PrimitiveParams &p) -> std::unique_ptr<IPrimitive>
    {
        return std::make_unique<Plane>(p.position, p.direction, p.material, p.transform);
    });
}

static void registerMaterials()
{
    auto &reg = MaterialRegistry::instance();
    reg.registerType("flat", [](const MaterialParams &m) -> std::unique_ptr<IMaterial>
    {
        return buildFlatMaterial(m);
    });
    reg.registerType("reflective", [](const MaterialParams &m) -> std::unique_ptr<IMaterial>
    {
        return buildReflectiveMaterial(m);
    });
}

static void registerLights()
{
    auto &reg = LightRegistry::instance();
    reg.registerType("ambient", [](const LightParams &l) -> std::unique_ptr<ILight>
    {
        return std::make_unique<AmbientLight>(l.color, l.intensity);
    });
    reg.registerType("directional", [](const LightParams &l) -> std::unique_ptr<ILight>
    {
        return std::make_unique<DirectionalLight>(l.direction, l.color, l.intensity);
    });
    reg.registerType("point", [](const LightParams &l) -> std::unique_ptr<ILight>
    {
        return std::make_unique<PointLight>(l.position, l.color * l.intensity);
    });
}

static void registerTextures()
{
    auto &reg = TextureRegistry::instance();
    reg.registerType("solid", [](const TextureParams &t) -> std::unique_ptr<ITexture>
    {
        return std::make_unique<SolidColorTexture>(t.color1);
    });
    reg.registerType("checker", [](const TextureParams &t) -> std::unique_ptr<ITexture>
    {
        return std::make_unique<CheckerTexture>(t.color1, t.color2, t.scale);
    });
}

namespace
{
    bool registerBuiltinTypes()
    {
        registerPrimitives();
        registerMaterials();
        registerLights();
        registerTextures();
        return true;
    }

    static bool registered = registerBuiltinTypes();
}
